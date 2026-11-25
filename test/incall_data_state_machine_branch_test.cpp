/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define private public
#define protected public

#include <map>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cellular_data_constant.h"
#include "cellular_data_settings_rdb_helper.h"
#include "cellular_data_utils.h"
#include "core_manager_inner.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "incall_data_state_machine.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "mock/mock_network_search.h"
#include "mock/mock_sim_manager.h"
#include "network_search_types.h"
#include "parameters.h"
#include "radio_event.h"
#include "sim_state_type.h"
#include "telephony_types.h"
#include "utils/scoped_setter.h"
#include "utils/test_async_utils.h"
#include "uri.h"

namespace OHOS {
namespace Telephony {
using namespace testing;
using namespace testing::ext;

namespace {
const std::string BACKUP_SIM_LIST_VALUE_VALID = "0,1,0;1,2,1";
const std::string BACKUP_SIM_LIST_VALUE_SINGLE = "0,1,0";
const std::string BACKUP_SIM_LIST_VALUE_INVALID = "-1,x,0;abc;1,2";
}

class IncallDataTestHandler : public TelEventHandler {
public:
    IncallDataTestHandler() : TelEventHandler("IncallDataTestHandler") {}
    ~IncallDataTestHandler() = default;
};

class IncallDataStateMachineBranchTest : public testing::Test {
public:
    IncallDataStateMachineBranchTest() = default;
    ~IncallDataStateMachineBranchTest() = default;
    static void SetUpTestCase() {}
    static void TearDownTestCase()
    {
        UnmockManager();
    }
    void SetUp()
    {
        InitMockManager();
        SetUpMockSettingsHelper();
        CreateIncallDataStateMachine();
        // Init() 末尾 StateMachine::Start() 异步投递 MSG_STATE_MACHINE_INIT，FFRT 线程延迟
        // 处理时会在用例中途执行 InitCmdEnter/StateBegin（额外消费 SimManager mock 的
        // GetDefaultCellularDataSlotId/GetPrimarySlotId 并重置 destState_，DT 实测
        // SecondaryActiveState_ProcessCallEnded_001 因过饱和失败）。本文件 11 个用例均直接
        // 驱动状态方法、不依赖状态机事件循环，SetUp 统一清空待处理事件消除该竞态。
        TestAsyncUtils::DrainTelEventHandler(stateMachine_->stateMachineEventHandler_.get());
    }
    void TearDown()
    {
        UnmockManager();
        system::SetParameter(PERSIST_TSTS_MODE, "0");
        // 用例结束先停机排空 FFRT 队列再析构成员，规避 TelEventQueue 生命周期竞态（DT Signal 6）
        if (stateMachine_ != nullptr) {
            TestAsyncUtils::DrainStateMachine(stateMachine_.get());
            stateMachine_ = nullptr;
        }
        if (handler_ != nullptr) {
            TestAsyncUtils::DrainTelEventHandler(handler_.get());
            handler_ = nullptr;
        }
        // 状态机/Handler 析构完成后还原单例的 settingsHelper_ 原值再销毁 mock，
        // 避免后续用例读到悬垂指针（mock 注入守卫，RAII 还原，不跨用例泄漏）
        settingsHelperGuard_.reset();
        mockResultSet_.reset();
        resultSet_.reset();
        mockDataShareHelper_.reset();
    }

    /*
     * Mock DataShareHelper 注入：真机上测试进程（hdcd token）无 ohos.permission.
     * MANAGE_SETTINGS，DataShare 服务侧 Update/Insert 恒被拒（hilog ExecuteEx
     * Permission denied），WriteBackupSimList/SetIncallDataSwitchOn 真库路径全部
     * 失败导致 14 个用例 GTEST_SKIP。改为向 CellularDataSettingsRdbHelper 单例
     * 的 settingsHelper_ 缓存成员塞 MockDataShareHelper + 内存假库（URI→值），
     * 读写全部走 mock，不触真库。
     */
    void SetUpMockSettingsHelper()
    {
        mockResultSet_ = std::make_shared<DataShareResultSetMock>();
        resultSet_ = mockResultSet_;
        mockDataShareHelper_ = std::make_shared<MockDataShareHelper>();
        ON_CALL(*mockDataShareHelper_, Release()).WillByDefault(Return(true));
        ON_CALL(*mockDataShareHelper_, Insert(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockDataShareHelper_, Update(_, _, _)).WillByDefault(Return(0));
        ON_CALL(*mockDataShareHelper_, RegisterObserver(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockDataShareHelper_, UnregisterObserver(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockDataShareHelper_, NotifyChange(_)).WillByDefault(Return());
        ON_CALL(*mockDataShareHelper_, Query(_, _, _, _))
            .WillByDefault(Invoke([this](Uri &uri, const DataShare::DataSharePredicates &,
                std::vector<std::string> &, DataShare::DatashareBusinessError *) {
                lastQueryUri_ = uri.ToString();
                return resultSet_;
            }));
        ON_CALL(*mockResultSet_, GoToFirstRow()).WillByDefault(Return(0));
        ON_CALL(*mockResultSet_, Close()).WillByDefault(Return(0));
        ON_CALL(*mockResultSet_, GetColumnIndex(_, _))
            .WillByDefault(DoAll(SetArgReferee<1>(0), Return(0)));
        ON_CALL(*mockResultSet_, GetString(_, _))
            .WillByDefault(Invoke([this](int, std::string &value) {
                auto it = fakeSettingsDb_.find(lastQueryUri_);
                value = (it != fakeSettingsDb_.end()) ? it->second : "";
                return DataShare::E_OK;
            }));
        settingsHelperGuard_ = std::make_unique<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>>(
            CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockDataShareHelper_);
    }

    // 假库 key 与 Query 侧统一走 Uri::ToString()，规避 URI 字符串归一化差异
    static std::string SettingsKey(const std::string &uri)
    {
        return Uri(uri).ToString();
    }


    void InitMockManager()
    {
        mockSimManager = new MockSimManager();
        std::shared_ptr<MockSimManager> mockSimManagerPtr(mockSimManager);
        CoreManagerInner::GetInstance().simManager_ = mockSimManagerPtr;
        mockNetworkSearchManager = new MockNetworkSearchManager();
        std::shared_ptr<MockNetworkSearchManager> mockNetworkSearchManagerPtr(mockNetworkSearchManager);
        CoreManagerInner::GetInstance().networkSearchManager_ = mockNetworkSearchManagerPtr;
    }

    static void UnmockManager()
    {
        if (CoreManagerInner::GetInstance().simManager_ != nullptr) {
            CoreManagerInner::GetInstance().simManager_ = nullptr;
        }
        if (CoreManagerInner::GetInstance().networkSearchManager_ != nullptr) {
            CoreManagerInner::GetInstance().networkSearchManager_ = nullptr;
        }
    }

    void CreateIncallDataStateMachine(int32_t slotId = 1)
    {
        handler_ = std::make_shared<IncallDataTestHandler>();
        stateMachine_ = std::make_shared<IncallDataStateMachine>();
        sptr<ApnManager> apnManager = nullptr;
        stateMachine_->Init(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DIALING), slotId, handler_, apnManager);
    }

    // 内存假库写入（不再触真库；真机 DataShare 权限拒绝史见 SetUpMockSettingsHelper 注释）
    bool WriteBackupSimList(const std::string &value)
    {
        fakeSettingsDb_[SettingsKey(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI)] = value;
        return true;
    }

    bool SetIncallDataSwitchOn()
    {
        fakeSettingsDb_[SettingsKey(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI)] = "1";
        fakeSettingsDb_[SettingsKey(CELLULAR_DATA_SETTING_INTELLIGENCE_SWITCH_URI)] = "1";
        return true;
    }

    bool SetIncallDataSwitchOff()
    {
        fakeSettingsDb_[SettingsKey(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI)] = "0";
        fakeSettingsDb_[SettingsKey(CELLULAR_DATA_SETTING_INTELLIGENCE_SWITCH_URI)] = "0";
        return true;
    }

    std::shared_ptr<IncallDataStateMachine> stateMachine_ = nullptr;
    std::shared_ptr<IncallDataTestHandler> handler_ = nullptr;
    MockSimManager *mockSimManager = nullptr;
    MockNetworkSearchManager *mockNetworkSearchManager = nullptr;
    // settings DataShare mock 注入（见 SetUpMockSettingsHelper 注释）
    std::shared_ptr<MockDataShareHelper> mockDataShareHelper_ = nullptr;
    std::shared_ptr<DataShareResultSetMock> mockResultSet_ = nullptr;
    std::shared_ptr<DataShare::DataShareResultSet> resultSet_ = nullptr;
    std::unique_ptr<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>> settingsHelperGuard_ = nullptr;
    std::map<std::string, std::string> fakeSettingsDb_;
    std::string lastQueryUri_;
};

/**
 * @tc.number: IncallDataStateMachine_CanActiveDataByRadioTech_001
 * @tc.name: Test CanActiveDataByRadioTech with different radio tech
 * @tc.desc: Verify CanActiveDataByRadioTech returns true for LTE/NR/WCDMA and false for GSM/INVALID
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_CanActiveDataByRadioTech_001,
    Function | MediumTest | Level3)
{
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    ASSERT_TRUE(stateMachine_->CanActiveDataByRadioTech());

    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_NR)), Return(0)));
    ASSERT_TRUE(stateMachine_->CanActiveDataByRadioTech());

    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_WCDMA)), Return(0)));
    ASSERT_TRUE(stateMachine_->CanActiveDataByRadioTech());

    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_GSM)), Return(0)));
    ASSERT_FALSE(stateMachine_->CanActiveDataByRadioTech());

    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_INVALID)), Return(0)));
    ASSERT_FALSE(stateMachine_->CanActiveDataByRadioTech());
}

/**
 * @tc.number: IncallDataStateMachine_CheckBackupNetworkIfAllow_001
 * @tc.name: Test CheckBackupNetworkIfAllow with empty backup sim list
 * @tc.desc: Verify CheckBackupNetworkIfAllow returns true when backup sim list is not configured
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_CheckBackupNetworkIfAllow_001,
    Function | MediumTest | Level3)
{
    if (!WriteBackupSimList("")) {
        GTEST_SKIP() << "datashare unavailable, skip empty backup list case";
    }
    ASSERT_TRUE(stateMachine_->CheckBackupNetworkIfAllow(0));
}

/**
 * @tc.number: IncallDataStateMachine_CheckBackupNetworkIfAllow_002
 * @tc.name: Test CheckBackupNetworkIfAllow with single backup card
 * @tc.desc: Verify CheckBackupNetworkIfAllow returns true when backup card num is less than VALID_MIN_BACKUP_NUM
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_CheckBackupNetworkIfAllow_002,
    Function | MediumTest | Level3)
{
    if (!WriteBackupSimList(BACKUP_SIM_LIST_VALUE_SINGLE)) {
        GTEST_SKIP() << "datashare unavailable, skip single backup card case";
    }
    ASSERT_TRUE(stateMachine_->CheckBackupNetworkIfAllow(0));
}

/**
 * @tc.number: IncallDataStateMachine_CheckBackupNetworkIfAllow_003
 * @tc.name: Test CheckBackupNetworkIfAllow with matched and unmatched sim label
 * @tc.desc: Verify CheckBackupNetworkIfAllow returns true when target slot is in backup list, false when not
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_CheckBackupNetworkIfAllow_003,
    Function | MediumTest | Level3)
{
    if (!WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip matched sim label case";
    }
    // sim label index 2 esim matches the second backup card entry "1,2,1"
    SimLabel esimLabel;
    esimLabel.simType = SimType::ESIM;
    esimLabel.index = 2;
    EXPECT_CALL(*mockSimManager, GetSimLabel(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(esimLabel), Return(0)));
    ASSERT_TRUE(stateMachine_->CheckBackupNetworkIfAllow(0));

    // sim label not in backup list
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    ASSERT_FALSE(stateMachine_->CheckBackupNetworkIfAllow(0));
}

/**
 * @tc.number: IncallDataStateMachine_CheckBackupNetworkIfAllow_004
 * @tc.name: Test CheckBackupNetworkIfAllow with invalid backup sim list
 * @tc.desc: Verify CheckBackupNetworkIfAllow returns true when backup list content is malformed
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_CheckBackupNetworkIfAllow_004,
    Function | MediumTest | Level3)
{
    if (!WriteBackupSimList(BACKUP_SIM_LIST_VALUE_INVALID)) {
        GTEST_SKIP() << "datashare unavailable, skip invalid backup list case";
    }
    // 非法列表（"-1,x,0;abc;1,2"）各段全部被 ParseSingleBackupCardInfo 丢弃 →
    // cardInfos 数量 < VALID_MIN_BACKUP_NUM → 在 CheckIfBackupNetwork 之前提前返回 true，
    // 不会触达 GetSimLabel（原用例带 GetSimLabel WillOnce 预期，真库写失败被 skip 时
    // 从未暴露该错误预期，mock 化后按实际路径修正）
    ASSERT_TRUE(stateMachine_->CheckBackupNetworkIfAllow(0));
}

/**
 * @tc.number: IdleState_ProcessCallStarted_001
 * @tc.name: Test ProcessCallStarted when backup network is not allowed and slot3 is a backup card
 * @tc.desc: Verify ProcessCallStarted falls back to slot3 itself when target slot is not allowed
 *           and slot3 is in the backup sim list in TSTS mode
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessCallStarted_001, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip backup not allowed case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    // target slot 1 is not in the backup list
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_1, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    // slot3 itself is a backup card (esim label index 2 matches "1,2,1")
    SimLabel esimLabel;
    esimLabel.simType = SimType::ESIM;
    esimLabel.index = 2;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_3, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(esimLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_3))
        .Times(1)
        .WillOnce(Return(0));

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STARTED);
    ASSERT_TRUE(idleState->ProcessCallStarted(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->activatingSecondaryState_);
    // delayed self-check is scheduled in case the call ended event is lost
    ASSERT_TRUE(stateMachine_->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STATE_CHECK));
}

/**
 * @tc.number: IdleState_ProcessCallStarted_002
 * @tc.name: Test ProcessCallStarted when backup network is allowed
 * @tc.desc: Verify ProcessCallStarted switches default slot and transitions to activating state in TSTS mode
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessCallStarted_002, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip backup allowed case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    SimLabel esimLabel;
    esimLabel.simType = SimType::ESIM;
    esimLabel.index = 2;
    EXPECT_CALL(*mockSimManager, GetSimLabel(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(esimLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_1))
        .Times(1)
        .WillOnce(Return(0));

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STARTED);
    ASSERT_TRUE(idleState->ProcessCallStarted(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->activatingSecondaryState_);
}


/**
 * @tc.number: IdleState_ProcessCallStarted_003
 * @tc.name: Test ProcessCallStarted when neither target slot nor slot3 is a backup card
 * @tc.desc: Verify ProcessCallStarted keeps idle state when target slot is not allowed and slot3 is
 *           not in the backup sim list in TSTS mode
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessCallStarted_003, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip slot3 not backup card case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(_)).Times(0);

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STARTED);
    ASSERT_TRUE(idleState->ProcessCallStarted(event));
    ASSERT_FALSE(stateMachine_->IsSecondaryActiveState());
}

/**
 * @tc.number: IdleState_ProcessCallStarted_004
 * @tc.name: Test ProcessCallStarted when backup network is not allowed on non-TSTS secondary slot
 * @tc.desc: Verify ProcessCallStarted keeps idle state when target slot is not allowed on a normal slot
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessCallStarted_004, Function | MediumTest | Level3)
{
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_1);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip non-TSTS not allowed case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(_)).Times(0);

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STARTED);
    ASSERT_TRUE(idleState->ProcessCallStarted(event));
    ASSERT_FALSE(stateMachine_->IsSecondaryActiveState());
}

/**
 * @tc.number: IdleState_ProcessSettingsOn_001
 * @tc.name: Test ProcessSettingsOn falls back to slot3 when target slot is not allowed
 * @tc.desc: Verify ProcessSettingsOn switches data to slot3 when target slot is not allowed
 *           and slot3 is a backup card in TSTS mode
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessSettingsOn_001, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip settings on fallback case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_1, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    SimLabel esimLabel;
    esimLabel.simType = SimType::ESIM;
    esimLabel.index = 2;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_3, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(esimLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_3))
        .Times(1)
        .WillOnce(Return(0));

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_SETTINGS_ON);
    ASSERT_TRUE(idleState->ProcessSettingsOn(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->activatingSecondaryState_);
}

/**
 * @tc.number: IdleState_ProcessDsdsChanged_001
 * @tc.name: Test ProcessDsdsChanged falls back to slot3 when target slot is not allowed
 * @tc.desc: Verify ProcessDsdsChanged switches data to slot3 when target slot is not allowed
 *           and slot3 is a backup card in TSTS mode
 */
HWTEST_F(IncallDataStateMachineBranchTest, IdleState_ProcessDsdsChanged_001, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOn() || !WriteBackupSimList(BACKUP_SIM_LIST_VALUE_VALID)) {
        GTEST_SKIP() << "datashare unavailable, skip dsds changed fallback case";
    }
    EXPECT_CALL(*mockSimManager, GetActiveSimAccountInfoList(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::vector<IccAccountInfo>()), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(static_cast<int32_t>(DSDS_MODE_V2)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(true), Return(0)));
    ImsRegInfo registered;
    registered.imsRegState = ImsRegState::IMS_REGISTERED;
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<2>(registered), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId())
        .WillRepeatedly(Return(CELLDATA_SLOT_ID_0));
    SimLabel unmatchedLabel;
    unmatchedLabel.simType = SimType::PSIM;
    unmatchedLabel.index = 9;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_1, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(unmatchedLabel), Return(0)));
    SimLabel esimLabel;
    esimLabel.simType = SimType::ESIM;
    esimLabel.index = 2;
    EXPECT_CALL(*mockSimManager, GetSimLabel(CELLDATA_SLOT_ID_3, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(esimLabel), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_3))
        .Times(1)
        .WillOnce(Return(0));

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DSDS_CHANGED);
    ASSERT_TRUE(idleState->ProcessDsdsChanged(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->activatingSecondaryState_);
}

/**
 * @tc.number: SecondaryActiveState_ProcessCallEnded_001
 * @tc.name: Test ProcessCallEnded transitions in secondary active state
 * @tc.desc: Verify ProcessCallEnded transitions to deactivating state when default slot differs from primary slot,
 *           and to idle state when they are the same
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_ProcessCallEnded_001, Function | MediumTest | Level3)
{
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    auto secondaryActiveState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_ENDED);

    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_2));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    ASSERT_TRUE(secondaryActiveState->ProcessCallEnded(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);

    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_0));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    ASSERT_TRUE(secondaryActiveState->ProcessCallEnded(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->idleState_);
}

/**
 * @tc.number: SecondaryActiveState_ProcessCallEnded_002
 * @tc.name: Test ProcessCallEnded restores data slot after slot3 fallback
 * @tc.desc: Verify ProcessCallEnded transitions to deactivating state when the default slot
 *           falls back to slot3 and differs from the primary slot, so the deactivating state
 *           restores the primary slot by the default/primary comparison
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_ProcessCallEnded_002, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    auto secondaryActiveState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_ENDED);

    // default slot falls back to slot3 and differs from the primary slot, restore is required
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_3));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    ASSERT_TRUE(secondaryActiveState->ProcessCallEnded(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);
}

/**
 * @tc.number: SecondaryActiveState_ProcessSettingsOff_001
 * @tc.name: Test ProcessSettingsOff restores data slot when the incall data switch is off
 * @tc.desc: Verify ProcessSettingsOff transitions to deactivating state when the default slot
 *           differs from the primary slot after the fallback switch to slot3
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_ProcessSettingsOff_001, Function | MediumTest | Level3)
{
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    if (!SetIncallDataSwitchOff()) {
        GTEST_SKIP() << "datashare unavailable, skip settings off restore case";
    }
    auto secondaryActiveState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_SETTINGS_OFF);

    // default slot falls back to slot3 and differs from the primary slot, restore is required
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_3));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    ASSERT_TRUE(secondaryActiveState->ProcessSettingsOff(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);
}

/**
 * @tc.number: SecondaryActiveState_ProcessDsdsChanged_001
 * @tc.name: Test ProcessDsdsChanged re-runs the restore when the call already ended
 * @tc.desc: Verify ProcessDsdsChanged does nothing while still in call, and re-runs the
 *           primary/default slot comparison restore when the call state is already idle
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_ProcessDsdsChanged_001,
    Function | MediumTest | Level3)
{
    auto secondaryActiveState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DSDS_CHANGED);

    // still in call, no transition
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DIALING));
    ASSERT_TRUE(secondaryActiveState->ProcessDsdsChanged(event));
    ASSERT_NE(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);

    // call already ended, re-run the restore in case the call ended event was lost
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    ASSERT_TRUE(secondaryActiveState->ProcessDsdsChanged(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);
}

/**
 * @tc.number: SecondaryActiveState_ProcessCallStateCheck_001
 * @tc.name: Test ProcessCallStateCheck delayed self-check after slot3 fallback
 * @tc.desc: Verify ProcessCallStateCheck renews the delayed check while still in call, and re-runs
 *           the primary/default slot comparison restore when the call already ended
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_ProcessCallStateCheck_001,
    Function | MediumTest | Level3)
{
    auto secondaryActiveState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STATE_CHECK);

    // still in call, renew the delayed self-check
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DIALING));
    ASSERT_TRUE(secondaryActiveState->ProcessCallStateCheck(event));
    ASSERT_TRUE(stateMachine_->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STATE_CHECK));
    ASSERT_NE(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);

    // call already ended, re-run the restore in case the call ended event was lost
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    stateMachine_->stateMachineEventHandler_->RemoveEvent(
        CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STATE_CHECK);
    ASSERT_TRUE(secondaryActiveState->ProcessCallStateCheck(event));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->deactivatingSecondaryState_);
}

/**
 * @tc.number: DeactivatingSecondaryState_StateBegin_001
 * @tc.name: Test DeactivatingSecondaryState StateBegin branches
 * @tc.desc: Verify StateBegin restores primary slot when default slot differs and transitions to idle state,
 *           and transitions to idle state directly when they are the same
 */
HWTEST_F(IncallDataStateMachineBranchTest, DeactivatingSecondaryState_StateBegin_001, Function | MediumTest | Level3)
{
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(stateMachine_->deactivatingSecondaryState_);

    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_2));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_0))
        .Times(1)
        .WillOnce(Return(0));
    deactivatingSecondaryState->StateBegin();
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->idleState_);

    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_0));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    deactivatingSecondaryState->StateBegin();
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->idleState_);
}

/**
 * @tc.number: DeactivatingSecondaryState_StateBegin_002
 * @tc.name: Test DeactivatingSecondaryState StateBegin restores the primary slot after slot3 fallback
 * @tc.desc: Verify StateBegin restores the primary slot by the default/primary comparison after
 *           the fallback switch to slot3, and the pending delayed self-check is cleared back in idle state
 */
HWTEST_F(IncallDataStateMachineBranchTest, DeactivatingSecondaryState_StateBegin_002, Function | MediumTest | Level3)
{
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    CreateIncallDataStateMachine(CELLDATA_SLOT_ID_3);
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(stateMachine_->deactivatingSecondaryState_);

    // default slot falls back to slot3 and differs from the primary slot
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillOnce(Return(CELLDATA_SLOT_ID_3));
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    EXPECT_CALL(*mockSimManager, SetDefaultCellularDataSlotId(CELLDATA_SLOT_ID_0))
        .Times(1)
        .WillOnce(Return(0));
    deactivatingSecondaryState->StateBegin();
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->idleState_);

    auto idleState = std::static_pointer_cast<IdleState>(stateMachine_->idleState_);
    idleState->StateBegin();
    // pending delayed self-check is cleared when back to idle state
    ASSERT_FALSE(stateMachine_->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STATE_CHECK));
}

/**
 * @tc.number: DeactivatingSecondaryState_StateProcess_001
 * @tc.name: Test DeactivatingSecondaryState StateProcess branches
 * @tc.desc: Verify StateProcess handles data disconnected and settings on events correctly
 */
HWTEST_F(IncallDataStateMachineBranchTest, DeactivatingSecondaryState_StateProcess_001, Function | MediumTest | Level3)
{
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(stateMachine_->deactivatingSecondaryState_);
    AppExecFwk::InnerEvent::Pointer event(nullptr, nullptr);
    ASSERT_FALSE(deactivatingSecondaryState->StateProcess(event));

    // current slot differs from primary slot, transition to idle state
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(CELLDATA_SLOT_ID_0), Return(0)));
    auto disconnectedEvent =
        AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    ASSERT_TRUE(deactivatingSecondaryState->StateProcess(disconnectedEvent));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->idleState_);

    // current slot equals primary slot, no transition
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(stateMachine_->GetSlotId()), Return(0)));
    ASSERT_TRUE(deactivatingSecondaryState->StateProcess(disconnectedEvent));

    auto settingsOnEvent = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_SETTINGS_ON);
    ASSERT_TRUE(deactivatingSecondaryState->StateProcess(settingsOnEvent));

    auto unknownEvent = AppExecFwk::InnerEvent::Get(0);
    ASSERT_FALSE(deactivatingSecondaryState->StateProcess(unknownEvent));
}

/**
 * @tc.number: ActivatingSecondaryState_StateProcess_001
 * @tc.name: Test ActivatingSecondaryState StateProcess branches
 * @tc.desc: Verify StateProcess transitions to activated state on data connected event
 */
HWTEST_F(IncallDataStateMachineBranchTest, ActivatingSecondaryState_StateProcess_001, Function | MediumTest | Level3)
{
    auto activatingSecondaryState =
        std::static_pointer_cast<ActivatingSecondaryState>(stateMachine_->activatingSecondaryState_);
    AppExecFwk::InnerEvent::Pointer event(nullptr, nullptr);
    ASSERT_FALSE(activatingSecondaryState->StateProcess(event));

    auto connectedEvent = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    ASSERT_TRUE(activatingSecondaryState->StateProcess(connectedEvent));
    ASSERT_EQ(stateMachine_->stateMachineEventHandler_->destState_, stateMachine_->activatedSecondaryState_);

    auto unknownEvent = AppExecFwk::InnerEvent::Get(0);
    ASSERT_FALSE(activatingSecondaryState->StateProcess(unknownEvent));
}

/**
 * @tc.number: IncallDataState_StandaloneStates_NullGuards_001
 * @tc.name: Test standalone-constructed incall states with empty weak_ptr
 * @tc.desc: 独立构造各 State 子类（空 weak_ptr）→ StateProcess/各 Process 方法/StateBegin
 *           的空机守卫全部早退（NOT_PROCESSED/直接返回），不依赖状态机事件循环。
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataState_StandaloneStates_NullGuards_001,
    Function | MediumTest | Level3)
{
    auto event = AppExecFwk::InnerEvent::Get(0);
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    // IdleState：StateProcess 空事件/空机两守卫 + 各 Process 方法自身守卫
    IdleState idleState(std::weak_ptr<IncallDataStateMachine>(), "IdleState");
    EXPECT_FALSE(idleState.StateProcess(nullEvent));
    EXPECT_FALSE(idleState.StateProcess(event));
    EXPECT_FALSE(idleState.ProcessCallStarted(event));
    EXPECT_FALSE(idleState.ProcessCallEnded(event));
    EXPECT_FALSE(idleState.ProcessSettingsOn(event));
    EXPECT_FALSE(idleState.ProcessDsdsChanged(event));
    idleState.StateBegin();
    idleState.StateEnd();

    // SecondaryActiveState：StateProcess 空事件/空机 + 各 Process 方法守卫
    SecondaryActiveState secondaryState(std::weak_ptr<IncallDataStateMachine>(), "SecondaryActiveState");
    EXPECT_FALSE(secondaryState.StateProcess(nullEvent));
    EXPECT_FALSE(secondaryState.StateProcess(event));
    EXPECT_FALSE(secondaryState.ProcessCallEnded(event));
    EXPECT_FALSE(secondaryState.ProcessSettingsOff(event));
    EXPECT_FALSE(secondaryState.ProcessDsdsChanged(event));
    EXPECT_FALSE(secondaryState.ProcessCallStateCheck(event));
    secondaryState.ProcessSettingsOn(event);
    secondaryState.StateBegin();
    secondaryState.StateEnd();

    // 其余三态：StateProcess 空机守卫 + StateBegin/StateEnd 空机早退
    ActivatingSecondaryState activatingState(std::weak_ptr<IncallDataStateMachine>(), "ActivatingSecondaryState");
    EXPECT_FALSE(activatingState.StateProcess(event));
    activatingState.StateBegin();
    activatingState.StateEnd();

    ActivatedSecondaryState activatedState(std::weak_ptr<IncallDataStateMachine>(), "ActivatedSecondaryState");
    EXPECT_FALSE(activatedState.StateProcess(event));
    activatedState.StateBegin();
    activatedState.StateEnd();

    DeactivatingSecondaryState deactivatingState(std::weak_ptr<IncallDataStateMachine>(), "DeactivatingSecondaryState");
    EXPECT_FALSE(deactivatingState.StateProcess(event));
    deactivatingState.StateBegin();
    deactivatingState.StateEnd();
}

/**
 * @tc.number: SecondaryActiveState_CallStateVariants_001
 * @tc.name: Test ProcessDsdsChanged / ProcessCallStateCheck / ProcessCallEnded call state matrix
 * @tc.desc   UpdateCallState 三态（IDLE/DISCONNECTED/ACTIVE）驱动 ProcessDsdsChanged 的 ||
 *            矩阵与 TransitionTo；ProcessCallStateCheck 矩阵 + 空事件 + DATA_CONNECTED 事件 +
 *            未知事件；ProcessCallEnded 开关两态（&& 首操作数矩阵）。
 */
HWTEST_F(IncallDataStateMachineBranchTest, SecondaryActiveState_CallStateVariants_001,
    Function | MediumTest | Level3)
{
    auto secondaryState = std::static_pointer_cast<SecondaryActiveState>(stateMachine_->secondaryActiveState_);
    auto event = AppExecFwk::InnerEvent::Get(0);
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    // A) ProcessDsdsChanged：IDLE（首操作数真）/ DISCONNECTED（次操作数真）/ ACTIVE（全假）
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    EXPECT_TRUE(secondaryState->ProcessDsdsChanged(event));
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DISCONNECTED));
    EXPECT_TRUE(secondaryState->ProcessDsdsChanged(event));
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_ACTIVE));
    EXPECT_TRUE(secondaryState->ProcessDsdsChanged(event));
    // B) ProcessCallStateCheck：IDLE/DISCONNECTED → 转 deactivating + PROCESSED；
    //    ACTIVE → 续约延迟自检 + PROCESSED（事件本身不检查，含 eventHandler 非空弧）
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    EXPECT_TRUE(secondaryState->ProcessCallStateCheck(event));
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DISCONNECTED));
    EXPECT_TRUE(secondaryState->ProcessCallStateCheck(event));
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_ACTIVE));
    EXPECT_TRUE(secondaryState->ProcessCallStateCheck(event));
    EXPECT_TRUE(secondaryState->ProcessCallStateCheck(nullEvent));
    // C) ProcessCallEnded：智能通话开关 关/开 两态（IsIncallDataSwitchOn && 矩阵首操作数）
    stateMachine_->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_ACTIVE));
    SetIncallDataSwitchOff();
    secondaryState->ProcessCallEnded(event);
    SetIncallDataSwitchOn();
    secondaryState->ProcessCallEnded(event);
}

/**
 * @tc.number: IncallDataStateMachine_GetValueFailVariants_001
 * @tc.name: Test settings rdb GetValue failure variants
 * @tc.desc  Query mock 返回 nullptr → GetValue 非 SUCCESS：CheckBackupNetworkIfAllow
 *           备份列表未配置早退 true；IsIncallDataSwitchOn 首/第二个 GetValue 失败均 false。
 */
HWTEST_F(IncallDataStateMachineBranchTest, IncallDataStateMachine_GetValueFailVariants_001,
    Function | MediumTest | Level3)
{
    // A) Query 恒失败 → CheckBackupNetworkIfAllow 走"未配置"早退 true
    ON_CALL(*mockDataShareHelper_, Query(_, _, _, _))
        .WillByDefault(Invoke([this](Uri &uri, const DataShare::DataSharePredicates &,
            std::vector<std::string> &, DataShare::DatashareBusinessError *) {
            lastQueryUri_ = uri.ToString();
            return std::shared_ptr<DataShare::DataShareResultSet>();
        }));
    EXPECT_TRUE(stateMachine_->CheckBackupNetworkIfAllow(0));
    // B) IsIncallDataSwitchOn：首个 GetValue（智能上网）失败 → false
    EXPECT_FALSE(stateMachine_->IsIncallDataSwitchOn());
    // C) IsIncallDataSwitchOn：首个成功、第二个（双卡智能开关）失败 → false
    int32_t queryCalls = 0;
    ON_CALL(*mockDataShareHelper_, Query(_, _, _, _))
        .WillByDefault(Invoke([this, &queryCalls](Uri &uri, const DataShare::DataSharePredicates &,
            std::vector<std::string> &, DataShare::DatashareBusinessError *) {
            lastQueryUri_ = uri.ToString();
            ++queryCalls;
            return (queryCalls == 1) ? resultSet_
                                     : std::shared_ptr<DataShare::DataShareResultSet>();
        }));
    EXPECT_FALSE(stateMachine_->IsIncallDataSwitchOn());
    // D) 两查询均成功且都为开 → true（复核 mock 还原后正常路径仍可达）
    ON_CALL(*mockDataShareHelper_, Query(_, _, _, _))
        .WillByDefault(Invoke([this](Uri &uri, const DataShare::DataSharePredicates &,
            std::vector<std::string> &, DataShare::DatashareBusinessError *) {
            lastQueryUri_ = uri.ToString();
            return resultSet_;
        }));
    SetIncallDataSwitchOn();
    EXPECT_TRUE(stateMachine_->IsIncallDataSwitchOn());
}
} // namespace Telephony
} // namespace OHOS
