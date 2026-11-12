/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "mock/mock_sim_manager.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "utils/scoped_setter.h"
#include "cellular_data_constant.h"
#include "cellular_data_types.h"
#include "data_switch_settings.h"
#include "cellular_data_settings_rdb_helper.h"
#include "core_manager_inner.h"
#include "telephony_errors.h"
#include "telephony_ext_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class DataSwitchSettingTest : public testing::Test {
public:
    DataSwitchSettingTest()
    {
        mockSimManager = new MockSimManager();
        std::shared_ptr<MockSimManager> mockSimManagerPtr(mockSimManager);
        CoreManagerInner::GetInstance().simManager_ = mockSimManagerPtr;
    }
    ~DataSwitchSettingTest() = default;

    static void TearDownTestCase()
    {
        if (CoreManagerInner::GetInstance().simManager_ != nullptr) {
            CoreManagerInner::GetInstance().simManager_ = nullptr;
            std::cout << "DataSwitchSettingTest set simManager_ nullptr" << std::endl;
        }
    }

    MockSimManager *mockSimManager;
};

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_01, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_01 slotId: " << sets.slotId_ << std::endl;
    ASSERT_TRUE(sets.SetUserDataOn(true) == TELEPHONY_ERR_SUCCESS);

    DataSwitchSettings sets1(0);
    ASSERT_TRUE(sets.SetUserDataOn(true) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_02, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_02 slotId: " << sets.slotId_ << std::endl;
    bool dataEnabled;
    ASSERT_TRUE(sets.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataEnabled);

    DataSwitchSettings sets1(0);
    ASSERT_TRUE(sets1.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataEnabled);
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_03, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_03 slotId: " << sets.slotId_ << std::endl;
    ASSERT_TRUE(sets.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SUCCESS);

    DataSwitchSettings sets1(0);
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(0));
    ASSERT_TRUE(sets1.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SLOTID_INVALID);

    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    ASSERT_TRUE(sets1.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_04, Function | MediumTest | Level1)
{
    bool dataRoamingEnabled;
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_04 slotId: " << sets.slotId_ << std::endl;
    ASSERT_TRUE(sets.QueryUserDataRoamingStatus(dataRoamingEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataRoamingEnabled);

    DataSwitchSettings sets1(0);
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(0));
    ASSERT_TRUE(sets1.QueryUserDataRoamingStatus(dataRoamingEnabled) == TELEPHONY_ERR_LOCAL_PTR_NULL);

    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    sets1.QueryUserDataRoamingStatus(dataRoamingEnabled);
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_05, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(0);
    sets.userDataOn_ = false;
    std::cout << "DataSwitchSetting_05 userDataOn_: " << sets.userDataOn_ << ", policyDataOn_:" << sets.policyDataOn_
              << ", internalDataOn_:" << sets.internalDataOn_ << std::endl;
    ASSERT_FALSE(sets.IsAllowActiveData());
    sets.userDataOn_ = true;
    sets.policyDataOn_ = true;
    sets.internalDataOn_ = true;
    ASSERT_TRUE(sets.IsAllowActiveData());
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_06, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_06 slotId: " << sets.slotId_ << std::endl;
    int32_t simDetected = 1;
    ASSERT_TRUE(sets.SetAnySimDetected(simDetected) == TELEPHONY_ERR_SUCCESS);

    DataSwitchSettings sets1(0);
    ASSERT_TRUE(sets1.SetAnySimDetected(simDetected) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_07, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(2);
    std::cout << "DataSwitchSetting_07 slotId: " << sets.slotId_ << std::endl;
    ASSERT_TRUE(sets.QueryAnySimDetectedStatus() == TELEPHONY_ERR_SUCCESS);

    DataSwitchSettings sets1(0);
    ASSERT_TRUE(sets1.QueryAnySimDetectedStatus() == TELEPHONY_ERR_SUCCESS);
}

static bool IsVirtualModemSlot(int32_t slotId)
{
    return true;
}

static bool IsDcCellularDataAllowed()
{
    return true;
}

HWTEST_F(DataSwitchSettingTest, DataSwitchSetting_08, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(0);
    std::cout << "DataSwitchSetting_08 slotId: " << sets.slotId_ << std::endl;
    TELEPHONY_EXT_WRAPPER.isVirtualModemSlot_ = IsVirtualModemSlot;
    EXPECT_EQ(sets.IsAllowActiveData(), false);

    TELEPHONY_EXT_WRAPPER.isDcCellularDataAllowed_ = IsDcCellularDataAllowed;
    EXPECT_EQ(sets.IsAllowActiveData(), true);

    TELEPHONY_EXT_WRAPPER.isVirtualModemSlot_ = nullptr;
    TELEPHONY_EXT_WRAPPER.isDcCellularDataAllowed_ = nullptr;
}

/*
 * Mock 注入 fixture：每用例自建 MockSimManager/MockDataShareHelper/结果集 mock，
 * 经 ScopedSetter 守卫注入 CellularDataSettingsRdbHelper 单例的 settingsHelper_
 * 缓存成员与 CoreManagerInner::simManager_，在 UT 中模拟 DataShare 环境，
 * 覆盖 data_switch_settings.cpp 的成功/失败/回滚等分支；
 * 守卫析构自动还原进入用例前的原值，用例间互不影响。
 */
class DataSwitchSettingMockTest : public testing::Test {
public:
    DataSwitchSettingMockTest() = default;
    ~DataSwitchSettingMockTest() = default;

    void SetUp()
    {
        mockSimManager = std::make_shared<MockSimManager>();
        mockResultSet_ = std::make_shared<DataShareResultSetMock>();
        result_ = mockResultSet_;
        mockHelper_ = std::make_shared<MockDataShareHelper>();
        ON_CALL(*mockHelper_, Release()).WillByDefault(Return(true));
        ON_CALL(*mockHelper_, Query(_, _, _, _)).WillByDefault(Return(nullptr));
        ON_CALL(*mockHelper_, Insert(_, _)).WillByDefault(Return(1));
        ON_CALL(*mockHelper_, Update(_, _, _)).WillByDefault(Return(1));
        ON_CALL(*mockHelper_, NotifyChange(_)).WillByDefault(Return());
        ON_CALL(*mockResultSet_, GoToFirstRow()).WillByDefault(Return(0));
        ON_CALL(*mockResultSet_, Close()).WillByDefault(Return(0));
        // 经守卫注入：TearDown 还原进入用例前的原值，不向其他用例泄漏 mock 或空值
        simManagerGuard_ = std::make_unique<ScopedSetter<std::shared_ptr<ISimManager>>>(
            CoreManagerInner::GetInstance().simManager_, mockSimManager);
        settingsHelperGuard_ = std::make_unique<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>>(
            CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper_);
    }

    void TearDown()
    {
        // 先还原单例成员再销毁 mock，避免后续用例读到悬垂指针
        settingsHelperGuard_.reset();
        simManagerGuard_.reset();
        mockSimManager.reset();
        mockHelper_.reset();
        mockResultSet_.reset();
        result_.reset();
    }

protected:
    // 配置一次 GetValue 查询：结果集返回字符串 resultValue
    void SetupQueryResult(const std::string &resultValue)
    {
        EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
        EXPECT_CALL(*mockResultSet_, GetColumnIndex(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(0), Return(0)));
        EXPECT_CALL(*mockResultSet_, GetString(0, _))
            .WillOnce(DoAll(SetArgReferee<1>(resultValue), Return(0)));
    }

    std::shared_ptr<MockSimManager> mockSimManager;
    std::shared_ptr<MockDataShareHelper> mockHelper_;
    std::shared_ptr<DataShareResultSetMock> mockResultSet_;
    std::shared_ptr<DataShare::DataShareResultSet> result_;
    std::unique_ptr<ScopedSetter<std::shared_ptr<ISimManager>>> simManagerGuard_;
    std::unique_ptr<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>> settingsHelperGuard_;
};

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataOn_001, Function | MediumTest | Level1)
{
    // VSIM 卡直接返回成功，不落库
    DataSwitchSettings sets(CELLULAR_DATA_VSIM_SLOT_ID);
    ASSERT_TRUE(sets.SetUserDataOn(true) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataOn_002, Function | MediumTest | Level1)
{
    // PutValue 走 Insert 路径成功
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetUserDataOn(true) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(sets.userDataOn_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataOn_003, Function | MediumTest | Level1)
{
    // PutValue 失败时回滚 userDataOn_
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(-1));
    DataSwitchSettings sets(0);
    sets.userDataOn_ = true;
    ASSERT_TRUE(sets.SetUserDataOn(false) != TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(sets.userDataOn_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetAnySimDetected_001, Function | MediumTest | Level1)
{
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetAnySimDetected(1) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetAnySimDetected_002, Function | MediumTest | Level1)
{
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(-1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetAnySimDetected(1) != TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetIntelliSwitchOn_001, Function | MediumTest | Level1)
{
    // 成功时更新 intelliSwitchOn_
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetIntelliSwitchOn(true) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(sets.intelliSwitchOn_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetIntelliSwitchOn_002, Function | MediumTest | Level1)
{
    // 失败时不更新 intelliSwitchOn_
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(-1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetIntelliSwitchOn(true) != TELEPHONY_ERR_SUCCESS);
    ASSERT_FALSE(sets.intelliSwitchOn_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataStatus_001, Function | MediumTest | Level1)
{
    // VSIM 卡开关恒开
    DataSwitchSettings sets(CELLULAR_DATA_VSIM_SLOT_ID);
    bool dataEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataStatus_002, Function | MediumTest | Level1)
{
    // 查询失败（结果集为空导致转换失败）
    SetupQueryResult("");
    DataSwitchSettings sets(0);
    bool dataEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_LOCAL_PTR_NULL);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataStatus_003, Function | MediumTest | Level1)
{
    SetupQueryResult(std::to_string(static_cast<int>(DataSwitchCode::CELLULAR_DATA_ENABLED)));
    DataSwitchSettings sets(0);
    bool dataEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataStatus_004, Function | MediumTest | Level1)
{
    SetupQueryResult(std::to_string(static_cast<int>(DataSwitchCode::CELLULAR_DATA_DISABLED)));
    DataSwitchSettings sets(0);
    bool dataEnabled = true;
    ASSERT_TRUE(sets.QueryUserDataStatus(dataEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_FALSE(dataEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryAnySimDetectedStatus_001, Function | MediumTest | Level1)
{
    SetupQueryResult("");
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.QueryAnySimDetectedStatus() != TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryAnySimDetectedStatus_002, Function | MediumTest | Level1)
{
    SetupQueryResult("1");
    DataSwitchSettings sets(0);
    // 返回码承载查询结果：SUCCESS = 库中已有 any_sim_detected 记录（查询值无调用方消费，参数已移除）
    ASSERT_TRUE(sets.QueryAnySimDetectedStatus() == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryIntelligenceSwitchStatus_001, Function | MediumTest | Level1)
{
    // GetValue 失败时仍返回 SUCCESS（内部容错）
    SetupQueryResult("");
    DataSwitchSettings sets(0);
    bool switchEnabled = true;
    ASSERT_TRUE(sets.QueryIntelligenceSwitchStatus(switchEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_FALSE(switchEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryIntelligenceSwitchStatus_002, Function | MediumTest | Level1)
{
    SetupQueryResult(std::to_string(static_cast<int>(IntelligenceSwitchCode::INTELLIGENCE_SWITCH_ENABLED)));
    DataSwitchSettings sets(0);
    bool switchEnabled = false;
    ASSERT_TRUE(sets.QueryIntelligenceSwitchStatus(switchEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(switchEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataRoamingOn_001, Function | MediumTest | Level1)
{
    // VSIM 卡直接返回成功
    DataSwitchSettings sets(CELLULAR_DATA_VSIM_SLOT_ID);
    ASSERT_TRUE(sets.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataRoamingOn_002, Function | MediumTest | Level1)
{
    // simId 无效
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(0));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SLOTID_INVALID);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataRoamingOn_003, Function | MediumTest | Level1)
{
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetUserDataRoamingOn(true) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(sets.userDataRoaming_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_SetUserDataRoamingOn_004, Function | MediumTest | Level1)
{
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(-1));
    DataSwitchSettings sets(0);
    ASSERT_TRUE(sets.SetUserDataRoamingOn(true) != TELEPHONY_ERR_SUCCESS);
    ASSERT_FALSE(sets.userDataRoaming_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataRoamingStatus_001, Function | MediumTest | Level1)
{
    // VSIM 卡漫游开关恒开
    DataSwitchSettings sets(CELLULAR_DATA_VSIM_SLOT_ID);
    bool dataRoamingEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataRoamingStatus(dataRoamingEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataRoamingEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataRoamingStatus_002, Function | MediumTest | Level1)
{
    // simId 无效
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(0));
    DataSwitchSettings sets(0);
    bool dataRoamingEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataRoamingStatus(dataRoamingEnabled) == TELEPHONY_ERR_LOCAL_PTR_NULL);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataRoamingStatus_003, Function | MediumTest | Level1)
{
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    SetupQueryResult("");
    DataSwitchSettings sets(0);
    bool dataRoamingEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataRoamingStatus(dataRoamingEnabled) != TELEPHONY_ERR_SUCCESS);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_QueryUserDataRoamingStatus_004, Function | MediumTest | Level1)
{
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    SetupQueryResult(std::to_string(static_cast<int>(RoamingSwitchCode::CELLULAR_DATA_ROAMING_ENABLED)));
    DataSwitchSettings sets(0);
    bool dataRoamingEnabled = false;
    ASSERT_TRUE(sets.QueryUserDataRoamingStatus(dataRoamingEnabled) == TELEPHONY_ERR_SUCCESS);
    ASSERT_TRUE(dataRoamingEnabled);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_LoadSwitchValue_001, Function | MediumTest | Level1)
{
    // LoadSwitchValue 内部完成数据开关与漫游开关两次查询
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).Times(2).WillRepeatedly(Return(result_));
    EXPECT_CALL(*mockResultSet_, GetColumnIndex(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(0)));
    EXPECT_CALL(*mockResultSet_, GetString(0, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>("1"), Return(0)));
    DataSwitchSettings sets(0);
    sets.LoadSwitchValue();
    ASSERT_TRUE(sets.userDataOn_);
    ASSERT_TRUE(sets.userDataRoaming_);
}

HWTEST_F(DataSwitchSettingMockTest, DataSwitchSettingMock_PolicyCarrierSwitch_001, Function | MediumTest | Level1)
{
    DataSwitchSettings sets(0);
    // 生产默认值：policyDataOn_/internalDataOn_ 为 true，carrierDataOn_ 为 false
    ASSERT_TRUE(sets.IsPolicyDataOn());
    sets.SetPolicyDataOn(false);
    ASSERT_FALSE(sets.IsPolicyDataOn());
    ASSERT_FALSE(sets.IsCarrierDataOn());
    sets.SetCarrierDataOn(true);
    ASSERT_TRUE(sets.IsCarrierDataOn());
    ASSERT_TRUE(sets.IsInternalDataOn());
    sets.SetInternalDataOn(false);
    ASSERT_FALSE(sets.IsInternalDataOn());
}
}  // namespace Telephony
}  // namespace OHOS
