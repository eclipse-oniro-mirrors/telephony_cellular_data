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

#include "common_event_manager.h"
#include "common_event_support.h"
#include "cellular_data_constant.h"
#include "cellular_data_handler.h"
#include "core_manager_inner.h"
#include "mock/mock_sim_manager.h"
#include "mock/mock_network_search.h"
#include "utils/test_async_utils.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;

class CellularDataHandlerBranchTest : public testing::Test {
public:
    CellularDataHandlerBranchTest() = default;
    ~CellularDataHandlerBranchTest() = default;
    static void TearDownTestCase()
    {
        UnmockManager();
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
            std::cout << "CellularDataHandlerBranchTest set simManager_ nullptr" << std::endl;
        }
        if (CoreManagerInner::GetInstance().networkSearchManager_ != nullptr) {
            CoreManagerInner::GetInstance().networkSearchManager_ = nullptr;
            std::cout << "CellularDataHandlerBranchTest set networkSearchManager_ nullptr" << std::endl;
        }
    }

    void InitCellularDataHandler()
    {
        cellularDataHandler_ = TestAsyncUtils::MakeDrainedHandler(0);
        cellularDataHandler_->Init();
    }

    std::shared_ptr<CellularDataHandler> cellularDataHandler_;
    MockSimManager *mockSimManager;
    MockNetworkSearchManager *mockNetworkSearchManager;
};

HWTEST_F(CellularDataHandlerBranchTest, RoamingStateOn_001, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
    // GetPsRoamingState return 1, dataRoamingEnabled is false
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(1));
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(0));
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler_->RoamingStateOn(event);

    // GetPsRoamingState return 1, dataRoamingEnabled is false
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(1));
    EXPECT_CALL(*mockSimManager, GetSimId(_)).Times(AtLeast(0)).WillRepeatedly(Return(1));
    cellularDataHandler_->defaultDataRoamingEnable_ = true;
    auto event1 = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler_->RoamingStateOn(event1);
    EXPECT_EQ(cellularDataHandler_->apnManager_->GetOverallApnState(), PROFILE_STATE_IDLE);
    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, CheckRoamingState_001, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
    // roamingState true, dataRoamingEnabled true, isMmsApn true, isEmergencyApn false, IsRestrictedMode false
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(1));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _)).WillOnce(Return(0));
    sptr<ApnHolder> apnHolder = new ApnHolder("mms", 0);
    ASSERT_FALSE(cellularDataHandler_ == nullptr);
    ASSERT_FALSE(cellularDataHandler_->dataSwitchSettings_ == nullptr);
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(true);
    ASSERT_TRUE(cellularDataHandler_->CheckRoamingState(apnHolder));

    // roamingState false, dataRoamingEnabled true, isMmsApn false, isEmergencyApn true, IsRestrictedMode false
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(0));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _)).WillOnce(Return(0));
    apnHolder = new ApnHolder("emergency", 0);
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(true);
    ASSERT_TRUE(cellularDataHandler_->CheckRoamingState(apnHolder));

    // roamingState true, dataRoamingEnabled false, isMmsApn true, isEmergencyApn false, IsRestrictedMode false
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(1));
    apnHolder = new ApnHolder("mms", 0);
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(false);
    ASSERT_FALSE(cellularDataHandler_->CheckRoamingState(apnHolder));

    // roamingState false, dataRoamingEnabled false, isMmsApn false, isEmergencyApn true, IsRestrictedMode true
    int32_t tech = 1;
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillOnce(Return(0));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _)).WillOnce(DoAll(SetArgReferee<1>(tech), Return(0)));
    cellularDataHandler_->lastCallState_ = 1;
    apnHolder = new ApnHolder("emergency", 0);
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(false);
    ASSERT_FALSE(cellularDataHandler_->CheckRoamingState(apnHolder));
    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, IsCellularDataRoamingEnabled_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);

    EXPECT_EQ(cellularDataHandler->dataSwitchSettings_, nullptr);
    cellularDataHandler->defaultDataRoamingEnable_ = false;
    bool result = false;
    cellularDataHandler->IsCellularDataRoamingEnabled(result);
    ASSERT_FALSE(result);

    cellularDataHandler->Init();
    cellularDataHandler->IsCellularDataRoamingEnabled(result);
    ASSERT_TRUE(result);
}

HWTEST_F(CellularDataHandlerBranchTest, SetDataPermittedForMms_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);

    cellularDataHandler->incallDataStateMachine_ =
        cellularDataHandler->CreateIncallDataStateMachine(CALL_STATUS_DIALING);
    ASSERT_FALSE(cellularDataHandler->SetDataPermittedForMms(false));
}

HWTEST_F(CellularDataHandlerBranchTest, CheckApnState_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();

    sptr<ApnHolder> apnHolder = new ApnHolder("mms", 0);
    apnHolder->SetApnState(PROFILE_STATE_DISCONNECTING);
    ASSERT_FALSE(cellularDataHandler->CheckApnState(apnHolder));

    apnHolder->SetApnState(PROFILE_STATE_FAILED);
    ASSERT_FALSE(cellularDataHandler->CheckApnState(apnHolder));

    apnHolder->SetApnState(PROFILE_STATE_CONNECTING);
    ASSERT_FALSE(cellularDataHandler->CheckApnState(apnHolder));
}

HWTEST_F(CellularDataHandlerBranchTest, EstablishDataConnection_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();

    sptr<ApnHolder> apnHolder = new ApnHolder("mms", 0);
    ApnItem apnItem;
    sptr<ApnItem> item = apnItem.MakeDefaultApn("mms");
    std::vector<sptr<ApnItem>> matchedApns;
    matchedApns.push_back(item);
    apnHolder->SetAllMatchedApns(matchedApns);
    cellularDataHandler->EstablishDataConnection(apnHolder, 1);
    ASSERT_EQ(apnHolder->GetApnState(), PROFILE_STATE_CONNECTING);
}

HWTEST_F(CellularDataHandlerBranchTest, DisconnectDataComplete_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();

    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->DisconnectDataComplete(event);
    ASSERT_EQ(event->GetSharedObject<SetupDataCallResultInfo>(), nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, DisconnectDataComplete_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();

    std::shared_ptr<SetupDataCallResultInfo> resultInfo = std::make_shared<SetupDataCallResultInfo>();
    resultInfo->flag = 2;
    auto event = AppExecFwk::InnerEvent::Get(0, resultInfo);
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    if (connectionManager == nullptr) {
        return;
    }
    connectionManager->Init();
    std::shared_ptr<CellularDataStateMachine> cellularDataStateMachine =
        std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    apnHolder->SetCellularDataStateMachine(cellularDataStateMachine);
    cellularDataHandler->DisconnectDataComplete(event);
    ASSERT_EQ(event->GetSharedObject<SetupDataCallResultInfo>(), resultInfo);
}

HWTEST_F(CellularDataHandlerBranchTest, UpdatePhysicalConnectionState_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();

    cellularDataHandler->physicalConnectionActiveState_ = true;
    cellularDataHandler->UpdatePhysicalConnectionState(true);
    ASSERT_FALSE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->physicalConnectionActiveState_ = false;
    cellularDataHandler->UpdatePhysicalConnectionState(true);
    ASSERT_FALSE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->physicalConnectionActiveState_ = true;
    cellularDataHandler->UpdatePhysicalConnectionState(false);
    ASSERT_TRUE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->physicalConnectionActiveState_ = false;
    cellularDataHandler->UpdatePhysicalConnectionState(false);
    ASSERT_TRUE(cellularDataHandler->physicalConnectionActiveState_);
}

HWTEST_F(CellularDataHandlerBranchTest, HandleScreenStateChanged_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->HandleScreenStateChanged(true);
    ASSERT_EQ(cellularDataHandler->connectionManager_, nullptr);
    cellularDataHandler->Init();
    cellularDataHandler->HandleScreenStateChanged(true);
    ASSERT_NE(cellularDataHandler->connectionManager_, nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, UpdateCellularDataConnectState_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    cellularDataHandler->UpdateCellularDataConnectState("default");
    ASSERT_NE(cellularDataHandler->apnManager_, nullptr);

    cellularDataHandler->UpdateCellularDataConnectState("internal_default");
    ASSERT_NE(cellularDataHandler->apnManager_, nullptr);

    cellularDataHandler->UpdateCellularDataConnectState("mss");
    ASSERT_NE(cellularDataHandler->apnManager_, nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, HandleDBSettingIncallChanged_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    cellularDataHandler->HandleImsCallChanged(TelCallStatus::CALL_STATUS_DIALING);

    auto event = AppExecFwk::InnerEvent::Get(0, static_cast<int64_t>(DataSwitchCode::CELLULAR_DATA_ENABLED));
    cellularDataHandler->HandleDBSettingIncallChanged(event);
    ASSERT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);

    event = AppExecFwk::InnerEvent::Get(0, static_cast<int64_t>(DataSwitchCode::CELLULAR_DATA_DISABLED));
    cellularDataHandler->HandleDBSettingIncallChanged(event);
    ASSERT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);

    cellularDataHandler->IncallDataComplete(event);
    ASSERT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, HandleImsCallChanged, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    cellularDataHandler->HandleImsCallChanged(TelCallStatus::CALL_STATUS_DIALING);
    cellularDataHandler->HandleImsCallChanged(TelCallStatus::CALL_STATUS_INCOMING);
    ASSERT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);
    cellularDataHandler->HandleVoiceCallChanged(TelCallStatus::CALL_STATUS_DISCONNECTED);
    cellularDataHandler->HandleVoiceCallChanged(TelCallStatus::CALL_STATUS_IDLE);
    ASSERT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, ReleaseAllNetworkRequest, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    cellularDataHandler->ReleaseAllNetworkRequest();
    ASSERT_NE(cellularDataHandler->apnManager_, nullptr);
}

HWTEST_F(CellularDataHandlerBranchTest, HandleSimStateChanged, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    SimState simState = SimState::SIM_STATE_READY;
    std::u16string iccId = u"iccId";
    EXPECT_CALL(*mockSimManager, GetSimState(_, _)).WillOnce(DoAll(SetArgReferee<1>(simState), Return(0)));
    EXPECT_CALL(*mockSimManager, GetSimIccId(_, _)).WillOnce(DoAll(SetArgReferee<1>(iccId), Return(0)));
    cellularDataHandler_->HandleSimStateChanged();
    ASSERT_NE(cellularDataHandler_->lastIccId_, iccId);

    cellularDataHandler_->lastIccId_ = u"iccId";
    EXPECT_CALL(*mockSimManager, GetSimState(_, _)).WillOnce(DoAll(SetArgReferee<1>(simState), Return(0)));
    EXPECT_CALL(*mockSimManager, GetSimIccId(_, _)).WillOnce(DoAll(SetArgReferee<1>(iccId), Return(0)));
    cellularDataHandler_->HandleSimStateChanged();
    ASSERT_EQ(cellularDataHandler_->lastIccId_, iccId);

    cellularDataHandler_->isRilApnAttached_ = true;
    simState = SimState::SIM_STATE_NOT_PRESENT;
    EXPECT_CALL(*mockSimManager, GetSimState(_, _)).WillOnce(DoAll(SetArgReferee<1>(simState), Return(0)));
    cellularDataHandler_->HandleSimStateChanged();

    simState = SimState::SIM_STATE_NOT_READY;
    EXPECT_CALL(*mockSimManager, GetSimState(_, _)).WillOnce(DoAll(SetArgReferee<1>(simState), Return(0)));
    cellularDataHandler_->HandleSimStateChanged();
    ASSERT_FALSE(cellularDataHandler_->isRilApnAttached_);

    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, HandleRecordsChanged, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    cellularDataHandler_->lastIccId_ = u"";
    std::u16string iccId = u"iccId";
    EXPECT_CALL(*mockSimManager, GetSimIccId(_, _)).WillOnce(DoAll(SetArgReferee<1>(iccId), Return(0)));
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockSimManager, GetSimId(_)).Times(AtLeast(0));
    EXPECT_CALL(*mockSimManager, GetSimOperatorNumeric(_, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockSimManager, IsCTSimCard(_, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillRepeatedly(Return(0));
    cellularDataHandler_->HandleSimAccountLoaded();
    cellularDataHandler_->HandleRecordsChanged();
    ASSERT_EQ(cellularDataHandler_->lastIccId_, iccId);

    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, SetPolicyDataOn, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    cellularDataHandler->dataSwitchSettings_->SetPolicyDataOn(true);
    cellularDataHandler->SetPolicyDataOn(false);
    ASSERT_FALSE(cellularDataHandler->dataSwitchSettings_->IsPolicyDataOn());

    cellularDataHandler->SetPolicyDataOn(true);
    ASSERT_TRUE(cellularDataHandler->dataSwitchSettings_->IsPolicyDataOn());
}

HWTEST_F(CellularDataHandlerBranchTest, ChangeConnectionForDsds, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    cellularDataHandler->ChangeConnectionForDsds(false);
    ASSERT_TRUE(cellularDataHandler->ChangeConnectionForDsds(false));
    ASSERT_TRUE(cellularDataHandler->ChangeConnectionForDsds(true));
}

HWTEST_F(CellularDataHandlerBranchTest, HandleDBSettingRoamingChanged, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    auto event = AppExecFwk::InnerEvent::Get(0, static_cast<int64_t>(RoamingSwitchCode::CELLULAR_DATA_ROAMING_ENABLED));
    cellularDataHandler->HandleDBSettingRoamingChanged(event);

    event = AppExecFwk::InnerEvent::Get(0, static_cast<int64_t>(RoamingSwitchCode::CELLULAR_DATA_ROAMING_DISABLED));
    cellularDataHandler->HandleDBSettingRoamingChanged(event);
    ASSERT_EQ(event->GetParam(), 0);
}

HWTEST_F(CellularDataHandlerBranchTest, UnRegisterDataSettingObserver, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    cellularDataHandler_->UnRegisterDataSettingObserver();
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    cellularDataHandler_->RegisterDataSettingObserver();
    ASSERT_NE(cellularDataHandler_->settingObserver_, nullptr);
    ASSERT_NE(cellularDataHandler_->roamingObserver_, nullptr);
    ASSERT_NE(cellularDataHandler_->incallObserver_, nullptr);
    ASSERT_NE(cellularDataHandler_->cellularDataRdbObserver_, nullptr);

    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, HandleSimAccountLoaded, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(1));
    cellularDataHandler_->HandleSimAccountLoaded();
    EXPECT_CALL(*mockSimManager, GetSimId(_)).WillOnce(Return(-1));
    cellularDataHandler_->HandleSimAccountLoaded();
    EXPECT_TRUE(cellularDataHandler_->isSimAccountLoaded_);
    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, CheckForCompatibleDataConnection, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    sptr<ApnHolder> apnHolder = new ApnHolder("dun", 0);
    cellularDataHandler->CheckForCompatibleDataConnection(apnHolder);
    ASSERT_EQ(apnHolder->GetApnType(), DATA_CONTEXT_ROLE_DUN);
}

HWTEST_F(CellularDataHandlerBranchTest, CheckCellularDataSlotId, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2); // 2: vsim slot id
    cellularDataHandler->Init();
    sptr<ApnHolder> apnHolder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    cellularDataHandler->CheckCellularDataSlotId(apnHolder);

    cellularDataHandler.reset();
    cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0); // 0: sim0 slot id
    cellularDataHandler->Init();
    apnHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    cellularDataHandler->CheckCellularDataSlotId(apnHolder);
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_DEFAULT;
    bool ret = cellularDataHandler->CheckCellularDataSlotId(apnHolder);
    EXPECT_FALSE(ret);

    cellularDataHandler.reset();
    cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1); // 1: sim1 slot id
    cellularDataHandler->Init();
    apnHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    cellularDataHandler->CheckCellularDataSlotId(apnHolder);
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_DEFAULT;
    ret = cellularDataHandler->CheckCellularDataSlotId(apnHolder);
    EXPECT_FALSE(ret);
}

HWTEST_F(CellularDataHandlerBranchTest, ResumeDataPermittedTimerOut_001, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    auto event = AppExecFwk::InnerEvent::Get(0);
    auto apnHolder =
        cellularDataHandler_->apnManager_->FindApnHolderById(DataContextRolesId::DATA_CONTEXT_ROLE_MMS_ID);
    apnHolder->SetApnState(PROFILE_STATE_IDLE);
    cellularDataHandler_->ResumeDataPermittedTimerOut(event);
    EXPECT_TRUE(cellularDataHandler_->dataSwitchSettings_->IsInternalDataOn());

    cellularDataHandler_->apnManager_ = nullptr;
    cellularDataHandler_->ResumeDataPermittedTimerOut(event);
    EXPECT_TRUE(cellularDataHandler_->dataSwitchSettings_->IsInternalDataOn());
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_002, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: event object is nullptr
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "");
 
    UnmockManager();
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_003, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: PLMN length is less than 3
    std::string plmn = "46";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "");
 
    UnmockManager();
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_004, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: MCC changes from "460" to "454"
    cellularDataHandler_->lastMcc_ = "460";
    std::string plmn = "45401";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_TRUE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "454");
 
    UnmockManager();
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_005, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: MCC changes from abroad to China
    cellularDataHandler_->lastMcc_ = "454";
    std::string plmn = "46001";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "460");
 
    UnmockManager();
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_006, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: Same MCC
    cellularDataHandler_->lastMcc_ = "460";
    std::string plmn = "46001";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "460");
 
    UnmockManager();
}
 
HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_007, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
 
    // Test case: One abroad MCC to another abroad MCC
    cellularDataHandler_->lastMcc_ = "454";
    std::string plmn = "44001";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "440");
 
    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_008, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    // Test case: User data roaming is on
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(true);
    cellularDataHandler_->lastMcc_ = "460";
    std::string plmn = "45401";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_FALSE(cellularDataHandler_->isMccChanged_);
    EXPECT_EQ(cellularDataHandler_->lastMcc_, "454");

    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, HandleResidentNetworkChanged_009, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();

    // Test case: User data roaming is off
    cellularDataHandler_->dataSwitchSettings_->UpdateUserDataRoamingOn(false);
    cellularDataHandler_->lastMcc_ = "460";
    std::string plmn = "45401";
    auto event = AppExecFwk::InnerEvent::Get(0, std::make_shared<std::string>(plmn));
    cellularDataHandler_->HandleResidentNetworkChanged(event);
    ASSERT_TRUE(cellularDataHandler_->isMccChanged_);
    ASSERT_EQ(cellularDataHandler_->lastMcc_, "454");

    UnmockManager();
}

HWTEST_F(CellularDataHandlerBranchTest, CheckAttachAndSimState_001, Function | MediumTest | Level3)
{
    InitCellularDataHandler();
    InitMockManager();
    cellularDataHandler_->isMccChanged_ = false;

    auto apnHolder =
        cellularDataHandler_->apnManager_->FindApnHolderById(DataContextRolesId::DATA_CONTEXT_ROLE_MMS_ID);
    SimState simState = SimState::SIM_STATE_READY;
    EXPECT_CALL(*mockSimManager, GetSimState(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(simState), Return(0)));
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRegState(_)).WillOnce(Return(0));
    EXPECT_FALSE(cellularDataHandler_->CheckAttachAndSimState(apnHolder));

    cellularDataHandler_->isMccChanged_ = true;
    EXPECT_FALSE(cellularDataHandler_->CheckAttachAndSimState(apnHolder));

    cellularDataHandler_->isMccChanged_ = false;
    cellularDataHandler_->RemoveAllEvents();
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRegState(_)).WillOnce(Return(1));
    cellularDataHandler_->CheckAttachAndSimState(apnHolder);
    EXPECT_FALSE(cellularDataHandler_->HasInnerEvent(CellularDataEventCode::MSG_RESUME_DATA_PERMITTED_TIMEOUT));
}

HWTEST_F(CellularDataHandlerBranchTest, IsCellularDataAllowedByExt_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    sptr<ApnHolder> apnHolder = nullptr;
    EXPECT_FALSE(cellularDataHandler->CheckCellularDataSlotId(apnHolder));
 
    cellularDataHandler.reset();
    cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    EXPECT_FALSE(cellularDataHandler->CheckCellularDataSlotId(apnHolder));
}

/**
 * @tc.number   SetRilAttachApn_002
 * @tc.name     sim not ready / esm off / esm on without apn / esm on with IA apn
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, SetRilAttachApn_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) sim 未就绪 → 早退
    ON_CALL(*mockSimManager, GetSimState(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(SimState::SIM_STATE_UNKNOWN), Return(0)));
    cellularDataHandler->SetRilAttachApn();
    // B) sim 就绪 + esm=0 → 零值 dataProfile → SetInitApnInfo（telRilManager_ 空，安全早退）
    ON_CALL(*mockSimManager, GetSimState(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(SimState::SIM_STATE_READY), Return(0)));
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 0;
            return 0;
        });
    cellularDataHandler->SetRilAttachApn();
    // C) esm=1 + 无 APN → GetRilAttachApn 空早退
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 1;
            return 0;
        });
    cellularDataHandler->SetRilAttachApn();
    // D) esm=1 + IA 角色 APN → 填 dataProfile → SetInitApnInfo
    sptr<ApnItem> attachApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_IA);
    cellularDataHandler->apnManager_->allApnItem_.push_back(attachApn);
    cellularDataHandler->SetRilAttachApn();
    UnmockManager();
}

/**
 * @tc.number   HandleDsdsModeChanged_001
 * @tc.name     null event / no payload / same mode / full non-TSTS path with VSIM skip
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, HandleDsdsModeChanged_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 空事件早退
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularDataHandler->HandleDsdsModeChanged(nullEvent);
    // B) 无共享对象早退
    cellularDataHandler->HandleDsdsModeChanged(AppExecFwk::InnerEvent::Get(0));
    // C) 模式相同早退：object->data == 当前 dsdsMode
    auto sameObj = std::make_shared<Int32Parcel>();
    sameObj->data = 5;
    ON_CALL(*mockSimManager, GetDsdsMode(_)).WillByDefault([](int32_t &mode) {
        mode = 5;
        return 0;
    });
    cellularDataHandler->HandleDsdsModeChanged(AppExecFwk::InnerEvent::Get(0, sameObj));
    // D) 全路径（非 TSTS）：data=16（低 4 位 0 == DSDS_MODE_V2），默认卡 0，3 卡
    //    i=0==默认卡 → SetDataPermitted(0,·)+ChangeConnectionForDsds（空 controller 安全早退）
    //    i=1：slotDsdsMode==V2 → SetDataPermitted(1,false)；i=2==VSIM → continue
    auto diffObj = std::make_shared<Int32Parcel>();
    diffObj->data = 16;
    ON_CALL(*mockSimManager, GetDsdsMode(_)).WillByDefault([](int32_t &mode) {
        mode = 0;
        return 0;
    });
    ON_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillByDefault(Return(0));
    ON_CALL(*mockSimManager, GetMaxSimCount()).WillByDefault(Return(3));
    ON_CALL(*mockSimManager, SetDsdsMode(_)).WillByDefault(Return(0));
    cellularDataHandler->HandleDsdsModeChanged(AppExecFwk::InnerEvent::Get(0, diffObj));
    UnmockManager();
}

/**
 * @tc.number   HandleDsdsModeChanged_002
 * @tc.name     TSTS mode enabled skips CELLDATA_SLOT_ID_3
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, HandleDsdsModeChanged_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    std::string origTsts = system::GetParameter(PERSIST_TSTS_MODE, TSTS_MODE_DEFAULT_VALUE);
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    auto diffObj = std::make_shared<Int32Parcel>();
    diffObj->data = 16;
    ON_CALL(*mockSimManager, GetDsdsMode(_)).WillByDefault([](int32_t &mode) {
        mode = 0;
        return 0;
    });
    ON_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).WillByDefault(Return(0));
    ON_CALL(*mockSimManager, GetMaxSimCount()).WillByDefault(Return(4));
    ON_CALL(*mockSimManager, SetDsdsMode(_)).WillByDefault(Return(0));
    // i=0==默认卡→SetDataPermitted+ChangeConnectionForDsds；i=1==V2→SetDataPermitted(false)；
    // i=2==VSIM→continue；i=3==CELLDATA_SLOT_ID_3 且 TSTS→continue
    cellularDataHandler->HandleDsdsModeChanged(AppExecFwk::InnerEvent::Get(0, diffObj));
    system::SetParameter(PERSIST_TSTS_MODE, origTsts);
    UnmockManager();
}

/**
 * @tc.number   GetDefaultDataRoamingConfig_001
 * @tc.name     opcfg key true / no key with system param enabled / null switch settings guard
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, GetDefaultDataRoamingConfig_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) opcfg 键 true + dataSwitchSettings_ 空守卫
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.boolValue[KEY_DEFAULT_DATA_ROAMING_BOOL] = true;
            return 0;
        });
    auto savedSettings = std::move(cellularDataHandler->dataSwitchSettings_);
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_TRUE(cellularDataHandler->defaultDataRoamingEnable_);
    cellularDataHandler->dataSwitchSettings_ = std::move(savedSettings);
    // B) 无键 → 系统参数路径：注入 enabled（结束还原）
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _)).WillByDefault([](int32_t, OperatorConfig &) {
        return 0;
    });
    const std::string roamingParam = "persist.telephony.defaultdataroaming0";
    std::string origRoaming = system::GetParameter(roamingParam, "0");
    system::SetParameter(roamingParam, "1");
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_TRUE(cellularDataHandler->defaultDataRoamingEnable_);
    system::SetParameter(roamingParam, origRoaming);
    UnmockManager();
}

/**
 * @tc.number   EstablishDataConnection_002
 * @tc.name     empty retry apns / single-connection blocked by higher priority / overall connecting clears all
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, EstablishDataConnection_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 无 matchedApns → GetNextRetryApn 空 → false
    sptr<ApnHolder> emptyHolder = new ApnHolder("mms", static_cast<int32_t>(DataContextPriority::PRIORITY_NORMAL));
    std::vector<sptr<ApnItem>> noApns;
    emptyHolder->SetAllMatchedApns(noApns);
    EXPECT_FALSE(cellularDataHandler->EstablishDataConnection(emptyHolder, 1));
    // 单连接开关打开（radioTech=1 在 singlePdp 数组）
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intArrayValue[KEY_SINGLE_PDP_RADIO_TYPE_INT_ARRAY] = { 1 };
            return 0;
        });
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn("mms");
    std::vector<sptr<ApnItem>> matchedApns;
    matchedApns.push_back(apnItem);
    sptr<ApnHolder> mmsHolder = new ApnHolder("mms", static_cast<int32_t>(DataContextPriority::PRIORITY_NORMAL));
    mmsHolder->SetAllMatchedApns(matchedApns);
    // B) IA（高优先级）holder 占线 → HasAnyHigherPriorityConnection true → false
    sptr<ApnHolder> iaHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_IA);
    ASSERT_NE(iaHolder, nullptr);
    NetRequest netRequest;
    iaHolder->RequestCellularData(netRequest);
    iaHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTING);
    EXPECT_FALSE(cellularDataHandler->EstablishDataConnection(mmsHolder, 1));
    // C) IA 释放 → 无高优先级；apnManager 侧 mms holder CONNECTING → 整体 CONNECTING → ClearAllConnections → false
    iaHolder->ReleaseAllCellularData();
    iaHolder->SetApnState(ApnProfileState::PROFILE_STATE_IDLE);
    sptr<ApnHolder> innerMmsHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_MMS);
    ASSERT_NE(innerMmsHolder, nullptr);
    innerMmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTING);
    EXPECT_FALSE(cellularDataHandler->EstablishDataConnection(mmsHolder, 1));
    UnmockManager();
}

/**
 * @tc.number   EstablishDataConnectionComplete_002
 * @tc.name     null event / no payload / default-holder full path / mms holder with state machine
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, EstablishDataConnectionComplete_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 空事件早退
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularDataHandler->EstablishDataConnectionComplete(nullEvent);
    // B) 无共享对象早退
    cellularDataHandler->EstablishDataConnectionComplete(AppExecFwk::InnerEvent::Get(0));
    // C) default holder 全路径（CONNECTED + RETRY_TO_SETUP_DATACALL + DataConnCompleteUpdateState 空状态机分支）
    sptr<ApnHolder> defaultHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(defaultHolder, nullptr);
    sptr<ApnItem> defaultApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultHolder->SetCurrentApn(defaultApn);
    auto defaultInfo = std::make_shared<SetupDataCallResultInfo>();
    defaultInfo->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    cellularDataHandler->EstablishDataConnectionComplete(AppExecFwk::InnerEvent::Get(0, defaultInfo));
    EXPECT_EQ(defaultHolder->GetApnState(), ApnProfileState::PROFILE_STATE_IDLE); // DataConnComplete 空状态机→IDLE
    // D) mms holder + 状态机 → UpdateHttpProxy/UpdateNetworkInfo 分支
    sptr<ApnHolder> mmsHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_MMS);
    ASSERT_NE(mmsHolder, nullptr);
    sptr<ApnItem> mmsApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS);
    mmsHolder->SetCurrentApn(mmsApn);
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    ASSERT_NE(connectionManager, nullptr);
    connectionManager->Init();
    std::shared_ptr<CellularDataStateMachine> stateMachine =
        std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    stateMachine->Init();
    mmsHolder->SetCellularDataStateMachine(stateMachine);
    auto mmsInfo = std::make_shared<SetupDataCallResultInfo>();
    mmsInfo->flag = DATA_CONTEXT_ROLE_MMS_ID;
    cellularDataHandler->EstablishDataConnectionComplete(AppExecFwk::InnerEvent::Get(0, mmsInfo));
    UnmockManager();
}

/**
 * @tc.number   MsgEstablishDataConnection_001
 * @tc.name     null event / unknown apn id / disabled holder disconnect branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, MsgEstablishDataConnection_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 空事件早退
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularDataHandler->MsgEstablishDataConnection(nullEvent);
    // B) 未知 apnId → FindApnHolderById 空 → 早退
    cellularDataHandler->MsgEstablishDataConnection(AppExecFwk::InnerEvent::Get(0, 999));
    // C) default holder（dataCallEnabled=false）→ 断连分支：GetPsRadioTech→IsSingleConnectionEnabled 空→
    //    reason=CLEAR → FindDisConnectionReason（bearType 空→原样）→ ClearConnection（状态机空→早退）
    ON_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    ON_CALL(*mockSimManager, GetSimId(_)).WillByDefault(Return(1));
    cellularDataHandler->MsgEstablishDataConnection(
        AppExecFwk::InnerEvent::Get(0, static_cast<uint64_t>(DATA_CONTEXT_ROLE_DEFAULT_ID)));
    UnmockManager();
}

/**
 * @tc.number   ClearConnection_001
 * @tc.name     null apn holder / holder without state machine
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearConnection_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    // A) apnHolder 为空 → 直接早退
    cellularDataHandler->ClearConnection(nullptr, DisConnectionReason::REASON_NORMAL);
    // B) holder 无状态机 → 早退且状态不变
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(apnHolder, nullptr);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_IDLE);
}

/**
 * @tc.number   ClearConnection_002
 * @tc.name     idle / disconnecting / retrying states early return
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearConnection_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    ASSERT_NE(connectionManager, nullptr);
    connectionManager->Init();
    // 状态机未 Init：SendEvent 走 stateMachineEventHandler_ 空守卫，全程安全
    auto stateMachine = std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(apnHolder, nullptr);
    apnHolder->SetCellularDataStateMachine(stateMachine);
    // IDLE / DISCONNECTING / RETRYING 三态均早退，状态保持原值
    apnHolder->SetApnState(PROFILE_STATE_IDLE);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_IDLE);
    apnHolder->SetApnState(PROFILE_STATE_DISCONNECTING);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_DISCONNECTING);
    apnHolder->SetApnState(PROFILE_STATE_RETRYING);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_RETRYING);
    apnHolder->SetCellularDataStateMachine(nullptr);
    UnmockManager();
}

/**
 * @tc.number   ClearConnection_003
 * @tc.name     connected holder transitions to disconnecting and sends event
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearConnection_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    ASSERT_NE(connectionManager, nullptr);
    connectionManager->Init();
    auto stateMachine = std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(apnHolder, nullptr);
    apnHolder->SetCellularDataStateMachine(stateMachine);
    apnHolder->SetApnState(PROFILE_STATE_CONNECTED);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_NORMAL);
    // 正常路径：置 DISCONNECTING 并投递 MSG_SM_DISCONNECT（SendEvent 空守卫）
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_DISCONNECTING);
    apnHolder->SetCellularDataStateMachine(nullptr);
    UnmockManager();
}

/**
 * @tc.number   ClearAllConnections_002
 * @tc.name     handover deactivate failure early return / null apn manager early return
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearAllConnections_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    // A) isHandoverOccurred_ 且 DeactivatePdpAfterHandover 走 telRilManager_(测试进程 null) 失败 → HiSysEvent + 早退
    cellularDataHandler->isHandoverOccurred_ = true;
    cellularDataHandler->ClearAllConnections(DisConnectionReason::REASON_NORMAL);
    EXPECT_TRUE(cellularDataHandler->isHandoverOccurred_);
    // B) apnManager_ 为空 → 早退
    cellularDataHandler->isHandoverOccurred_ = false;
    auto savedApnManager = std::move(cellularDataHandler->apnManager_);
    cellularDataHandler->ClearAllConnections(DisConnectionReason::REASON_NORMAL);
    cellularDataHandler->apnManager_ = std::move(savedApnManager);
}

/**
 * @tc.number   ClearAllConnections_003
 * @tc.name     full clear path skips mms and disconnects non-mms holders
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearAllConnections_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    ASSERT_NE(connectionManager, nullptr);
    connectionManager->Init();
    auto stateMachine = std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    // mms holder 被循环跳过；ia holder 走 ClearConnection 正常路径；default holder 无状态机早退
    sptr<ApnHolder> mmsHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(mmsHolder, nullptr);
    mmsHolder->SetCellularDataStateMachine(stateMachine);
    mmsHolder->SetApnState(PROFILE_STATE_CONNECTED);
    sptr<ApnHolder> iaHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_IA);
    ASSERT_NE(iaHolder, nullptr);
    iaHolder->SetCellularDataStateMachine(stateMachine);
    iaHolder->SetApnState(PROFILE_STATE_CONNECTED);
    cellularDataHandler->ClearAllConnections(DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(mmsHolder->GetApnState(), PROFILE_STATE_CONNECTED);
    EXPECT_EQ(iaHolder->GetApnState(), PROFILE_STATE_DISCONNECTING);
    mmsHolder->SetCellularDataStateMachine(nullptr);
    iaHolder->SetCellularDataStateMachine(nullptr);
    UnmockManager();
}

/**
 * @tc.number   ClearConnectionsOnUpdateApns_003
 * @tc.name     incompatible apn creates lastApnItem_ / existing lastApnItem_ skips new
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearConnectionsOnUpdateApns_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    // A) Init 后 lastApnItem_ 恒非空，显式置空覆盖 291-292 的 new 分支（attach apn 空 → 不兼容 → new）
    cellularDataHandler->lastApnItem_ = nullptr;
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_NORMAL);
    ASSERT_NE(cellularDataHandler->lastApnItem_, nullptr);
    // B) lastApnItem_ 已存在 + attach apn 仍空 → ClearAllConnections + 跳过 new
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_NORMAL);
    ASSERT_NE(cellularDataHandler->lastApnItem_, nullptr);
    UnmockManager();
}

/**
 * @tc.number   DataConnCompleteUpdateState_001
 * @tc.name     holder without state machine goes idle and flips physical link state
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, DataConnCompleteUpdateState_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    sptr<ApnHolder> iaHolder = cellularDataHandler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_IA);
    ASSERT_NE(iaHolder, nullptr);
    ASSERT_EQ(iaHolder->GetCellularDataStateMachine(), nullptr);
    std::shared_ptr<SetupDataCallResultInfo> resultInfo = std::make_shared<SetupDataCallResultInfo>();
    resultInfo->flag = 6; // IA
    cellularDataHandler->physicalConnectionActiveState_ = false;
    cellularDataHandler->DataConnCompleteUpdateState(iaHolder, resultInfo);
    // 无状态机 → IDLE；物理链路 false→true 翻转（DcPhysicalLinkActiveUpdate 走 telCoreService null 早退）
    EXPECT_EQ(iaHolder->GetApnState(), PROFILE_STATE_IDLE);
    EXPECT_TRUE(cellularDataHandler->physicalConnectionActiveState_);
    UnmockManager();
}

/**
 * @tc.number   DisconnectDataComplete_003
 * @tc.name     full path with initialized machine and mms holder not data call enabled
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, DisconnectDataComplete_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    ASSERT_NE(connectionManager, nullptr);
    connectionManager->Init();
    auto stateMachine = std::make_shared<CellularDataStateMachine>(connectionManager, cellularDataHandler);
    // UpdateNetworkInfo 前必须 Init（netLinkInfo_ 就绪），异步 INIT 落地只影响 currentState_ 无碍
    stateMachine->Init();
    sptr<ApnHolder> mmsHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(mmsHolder, nullptr);
    mmsHolder->SetCellularDataStateMachine(stateMachine);
    mmsHolder->SetApnState(PROFILE_STATE_CONNECTED);
    std::shared_ptr<SetupDataCallResultInfo> netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = 2; // MMS
    netInfo->reason = 0;
    auto event = AppExecFwk::InnerEvent::Get(0, netInfo);
    cellularDataHandler->DisconnectDataComplete(event);
    // 正常路径：摘除状态机、状态回 IDLE（IsDataCallEnabled 默认 false → else 分支）
    EXPECT_EQ(mmsHolder->GetCellularDataStateMachine(), nullptr);
    EXPECT_EQ(mmsHolder->GetApnState(), PROFILE_STATE_IDLE);
    UnmockManager();
}

/**
 * @tc.number   GetDefaultDataRoamingConfig_002
 * @tc.name     normal dataSwitchSettings path with equal / unequal roaming state
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, GetDefaultDataRoamingConfig_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // 无 opcfg 键 → 系统参数路径；查询失败但 defaultDataRoamingEnable_(false)==IsUserDataRoamingOn(默认 false) → 跳过写
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _)).WillByDefault([](int32_t, OperatorConfig &) {
        return 0;
    });
    const std::string roamingParam = "persist.telephony.defaultdataroaming0";
    std::string origRoaming = system::GetParameter(roamingParam, "0");
    system::SetParameter(roamingParam, "0");
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_FALSE(cellularDataHandler->defaultDataRoamingEnable_);
    // 参数注入 enabled → true != IsUserDataRoamingOn(false) → UpdateUserDataRoamingOn(true)（纯内存更新）
    system::SetParameter(roamingParam, "1");
    cellularDataHandler->GetDefaultDataRoamingConfig();
    system::SetParameter(roamingParam, origRoaming);
    EXPECT_TRUE(cellularDataHandler->dataSwitchSettings_->IsUserDataRoamingOn());
    UnmockManager();
}

/**
 * @tc.number   DisconnectDataComplete_004
 * @tc.name     null event / null apn manager / null connection manager / no payload / unknown apnId / no machine
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, DisconnectDataComplete_004, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 空事件早退
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularDataHandler->DisconnectDataComplete(nullEvent);
    std::shared_ptr<SetupDataCallResultInfo> netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = 2; // MMS
    auto event = AppExecFwk::InnerEvent::Get(0, netInfo);
    // B) apnManager_ 为空早退
    auto savedApnManager = std::move(cellularDataHandler->apnManager_);
    cellularDataHandler->DisconnectDataComplete(event);
    cellularDataHandler->apnManager_ = std::move(savedApnManager);
    // C) connectionManager_ 为空早退
    auto savedConnectionManager = std::move(cellularDataHandler->connectionManager_);
    cellularDataHandler->DisconnectDataComplete(event);
    cellularDataHandler->connectionManager_ = std::move(savedConnectionManager);
    // D) 无 payload 早退
    auto emptyEvent = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->DisconnectDataComplete(emptyEvent);
    // E) 未知 apnId → holder 为空早退
    netInfo->flag = 99;
    cellularDataHandler->DisconnectDataComplete(event);
    // F) holder 无状态机 → 状态回 IDLE
    netInfo->flag = 2;
    sptr<ApnHolder> mmsHolder = cellularDataHandler->apnManager_->FindApnHolderById(2);
    ASSERT_NE(mmsHolder, nullptr);
    mmsHolder->SetApnState(PROFILE_STATE_CONNECTED);
    cellularDataHandler->DisconnectDataComplete(event);
    EXPECT_EQ(mmsHolder->GetApnState(), PROFILE_STATE_IDLE);
    UnmockManager();
}

/**
 * @tc.number   EstablishDataConnection_003
 * @tc.name     full establish path creates machine and returns true
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerBranchTest, EstablishDataConnection_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(2);
    cellularDataHandler->Init();
    InitMockManager();
    // 非 default/internal holder（mms）走 CheckForCompatibleDataConnection（无活跃连接 → null）；
    // 不注入 singlePdp 数组 → IsSingleConnectionEnabled false → 跳过占线检查；
    // FindIdleCellularDataConnection 无空闲 → CreateCellularDataConnect + Init → 建立链全走 → true
    sptr<ApnHolder> mmsHolder = new ApnHolder("mms", static_cast<int32_t>(DataContextPriority::PRIORITY_NORMAL));
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn("mms");
    std::vector<sptr<ApnItem>> matchedApns;
    matchedApns.push_back(apnItem);
    mmsHolder->SetAllMatchedApns(matchedApns);
    EXPECT_TRUE(cellularDataHandler->EstablishDataConnection(mmsHolder, 1));
    EXPECT_EQ(mmsHolder->GetApnState(), PROFILE_STATE_CONNECTING);
    EXPECT_NE(mmsHolder->GetCellularDataStateMachine(), nullptr);
    UnmockManager();
}

/**
 * @tc.number   FindIdleCellularDataConnection_001
 * @tc.name     test find idle cellular data connection
 * @tc.desc     Function test
 * @tc.note     状态机矩阵注入（connectionManager_->stateMachines_ 直改）：
 *              未 Init 早退 / apnManager_ 空 / null 表项 / inactive 且未被持有（命中）/
 *              非 inactive（Init 后同步切 activating 态）/ inactive 但被 holder 持有（占用）。
 *              非 inactive 态用 currentState_ 直接赋值（TransitionTo 为异步投递不可同步断言）。
 */
HWTEST_F(CellularDataHandlerBranchTest, FindIdleCellularDataConnection_001, Function | MediumTest | Level3)
{
    // A) 未 Init：connectionManager_ 空 → 早退 nullptr
    auto noInitHandler = TestAsyncUtils::MakeDrainedHandler(0);
    EXPECT_EQ(noInitHandler->FindIdleCellularDataConnection(), nullptr);

    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // B) 空状态机表 → 循环零次 → nullptr
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    // C) apnManager_ 空 + 非 null 状态机 → 第二条件早退
    auto machineA = TestAsyncUtils::MakeDrainedStateMachine<CellularDataStateMachine>(
        cellularDataHandler->connectionManager_, cellularDataHandler);
    cellularDataHandler->connectionManager_->stateMachines_.push_back(machineA);
    auto savedApnManager = std::move(cellularDataHandler->apnManager_);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    cellularDataHandler->apnManager_ = std::move(savedApnManager);
    cellularDataHandler->connectionManager_->stateMachines_.clear();
    // D) null 表项 → 第一条件早退
    cellularDataHandler->connectionManager_->stateMachines_.push_back(nullptr);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    cellularDataHandler->connectionManager_->stateMachines_.clear();
    // E) inactive（未 Init：currentState_==inActiveState_==nullptr）+ 未被持有 → 返回该状态机
    auto idleMachine = TestAsyncUtils::MakeDrainedStateMachine<CellularDataStateMachine>(
        cellularDataHandler->connectionManager_, cellularDataHandler);
    cellularDataHandler->connectionManager_->stateMachines_.push_back(idleMachine);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), idleMachine);
    // F) 非 inactive → 循环继续 → nullptr
    auto activeMachine = TestAsyncUtils::MakeDrainedStateMachine<CellularDataStateMachine>(
        cellularDataHandler->connectionManager_, cellularDataHandler);
    activeMachine->Init();
    activeMachine->currentState_ = activeMachine->activatingState_;
    cellularDataHandler->connectionManager_->stateMachines_.clear();
    cellularDataHandler->connectionManager_->stateMachines_.push_back(activeMachine);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    // G) inactive 但被 holder 持有（used）→ IsDataConnectionNotUsed false → 继续 → nullptr
    cellularDataHandler->connectionManager_->stateMachines_.clear();
    cellularDataHandler->connectionManager_->stateMachines_.push_back(idleMachine);
    sptr<ApnHolder> holder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    holder->SetCellularDataStateMachine(idleMachine);
    cellularDataHandler->apnManager_->apnHolders_.push_back(holder);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    UnmockManager();
}

/**
 * @tc.number   ClearConnectionIfRequired_001
 * @tc.name     test clear connection if required
 * @tc.desc     Function test
 * @tc.note     apnHolders_ 矩阵注入 + allApnItem_ 控制 FilterMatchedApns 结果：
 *              null holder / 无匹配 APN / 首次 IsSameMatchedApns false + IDLE（仅刷新不清理）/
 *              二次调用 IsSameMatchedApns true（跳过）/ CONNECTED + 新 holder（ClearConnection
 *              状态机空 → 内部安全早退）/ HasAnyConnectedState 两方向。
 */
HWTEST_F(CellularDataHandlerBranchTest, ClearConnectionIfRequired_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    ON_CALL(*mockNetworkSearchManager, GetPsRoamingState(_)).WillByDefault(Return(0));
    auto &holders = cellularDataHandler->apnManager_->apnHolders_;
    holders.clear();
    // A) null holder → continue
    holders.push_back(nullptr);
    // B) 无匹配 APN（allApnItem_ 清空）→ matchedApns empty → continue
    sptr<ApnHolder> emptyHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    holders.push_back(emptyHolder);
    cellularDataHandler->ClearConnectionIfRequired();
    // C) 有匹配 + 首次 IsSameMatchedApns false + IDLE → SetAllMatchedApns 但不清连接
    cellularDataHandler->apnManager_->allApnItem_.clear();
    sptr<ApnItem> mmsApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS);
    cellularDataHandler->apnManager_->allApnItem_.push_back(mmsApn);
    sptr<ApnHolder> idleHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    idleHolder->SetApnState(ApnProfileState::PROFILE_STATE_IDLE);
    holders.push_back(idleHolder);
    cellularDataHandler->ClearConnectionIfRequired();
    EXPECT_EQ(idleHolder->GetApnState(), ApnProfileState::PROFILE_STATE_IDLE);
    // D) 二次调用 IsSameMatchedApns true → 跳过清理分支
    cellularDataHandler->ClearConnectionIfRequired();
    // E) 新 holder 状态 CONNECTED + 匹配 → ClearConnection（状态机空安全早退）
    sptr<ApnHolder> connectedHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    connectedHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    holders.push_back(connectedHolder);
    cellularDataHandler->ClearConnectionIfRequired();
    EXPECT_EQ(connectedHolder->GetApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    UnmockManager();
}

/**
 * @tc.number   HasAnyHigherPriorityConnection_001
 * @tc.name     test has any higher priority connection
 * @tc.desc     Function test
 * @tc.note     sortedApnHolders_ 矩阵注入：空表 / 高优先级未启用 / 启用+CONNECTED|CONNECTING|
 *              DISCONNECTING（true，写 hisysevent 遥测，仓内先例）/ 启用+IDLE / 低优先级。
 */
HWTEST_F(CellularDataHandlerBranchTest, HasAnyHigherPriorityConnection_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    cellularDataHandler->apnManager_->sortedApnHolders_.clear();
    sptr<ApnHolder> target = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 10);
    // A) 空排序表 → false
    EXPECT_FALSE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // B) 高优先级但未启用 → continue → false
    sptr<ApnHolder> higher = new ApnHolder(DATA_CONTEXT_ROLE_DUN, 100);
    cellularDataHandler->apnManager_->sortedApnHolders_.push_back(higher);
    EXPECT_FALSE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // C) 高优先级启用 + CONNECTED → true
    higher->dataCallEnabled_ = true;
    higher->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // D) CONNECTING → true
    higher->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTING);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // E) DISCONNECTING → true
    higher->SetApnState(ApnProfileState::PROFILE_STATE_DISCONNECTING);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // F) 启用但 IDLE → continue → false
    higher->SetApnState(ApnProfileState::PROFILE_STATE_IDLE);
    EXPECT_FALSE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    // G) 低优先级 → 优先级比较 false
    sptr<ApnHolder> lower = new ApnHolder(DATA_CONTEXT_ROLE_BIP, 1);
    cellularDataHandler->apnManager_->sortedApnHolders_.push_back(lower);
    EXPECT_FALSE(cellularDataHandler->HasAnyHigherPriorityConnection(target));
    UnmockManager();
}

/**
 * @tc.number   CheckMultiApnState_001
 * @tc.name     test check multi apn state
 * @tc.desc     Function test
 * @tc.note     非 DEFAULT 类型早退 / 仅普通 holder / BIP CONNECTED（ReleaseDataConnection
 *              状态机空安全）/ DUN DISCONNECTING / DUN CONNECTING（|| 右操作数）/ 多默认 IDLE。
 */
HWTEST_F(CellularDataHandlerBranchTest, CheckMultiApnState_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    auto &holders = cellularDataHandler->apnManager_->apnHolders_;
    holders.clear();
    // A) 非 DEFAULT 类型 → false
    sptr<ApnHolder> mmsHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    EXPECT_FALSE(cellularDataHandler->CheckMultiApnState(mmsHolder));
    // B) DEFAULT + 仅普通 holder → 全 continue → false
    sptr<ApnHolder> defaultHolder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    holders.push_back(mmsHolder);
    EXPECT_FALSE(cellularDataHandler->CheckMultiApnState(defaultHolder));
    // C) DEFAULT + BIP CONNECTED → ReleaseDataConnection（空状态机安全早退）→ true
    sptr<ApnHolder> bipHolder = new ApnHolder(DATA_CONTEXT_ROLE_BIP, 0);
    bipHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    holders.push_back(bipHolder);
    EXPECT_TRUE(cellularDataHandler->CheckMultiApnState(defaultHolder));
    // D) DEFAULT + DUN DISCONNECTING → true
    holders.pop_back();
    sptr<ApnHolder> dunHolder = new ApnHolder(DATA_CONTEXT_ROLE_DUN, 0);
    dunHolder->SetApnState(ApnProfileState::PROFILE_STATE_DISCONNECTING);
    holders.push_back(dunHolder);
    EXPECT_TRUE(cellularDataHandler->CheckMultiApnState(defaultHolder));
    // E) DUN CONNECTING（|| 右操作数）→ true
    dunHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTING);
    EXPECT_TRUE(cellularDataHandler->CheckMultiApnState(defaultHolder));
    // F) DUN IDLE → 两条件皆假 → false
    dunHolder->SetApnState(ApnProfileState::PROFILE_STATE_IDLE);
    EXPECT_FALSE(cellularDataHandler->CheckMultiApnState(defaultHolder));
    UnmockManager();
}

/**
 * @tc.number   CheckApnState_002
 * @tc.name     test check apn state
 * @tc.desc     Function test
 * @tc.note     holder/apnManager 空守卫 / DISCONNECTING 补发事件 / RETRYING / FAILED 复位 IDLE
 *              （无匹配→false、有匹配→SetAllMatchedApns+true）/ CONNECTED 非 IDLE 早退。
 */
HWTEST_F(CellularDataHandlerBranchTest, CheckApnState_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) null holder → false
    sptr<ApnHolder> nullHolder = nullptr;
    EXPECT_FALSE(cellularDataHandler->CheckApnState(nullHolder));
    // B) apnManager_ 空 → false
    sptr<ApnHolder> mmsHolder = new ApnHolder(DATA_CONTEXT_ROLE_MMS, 0);
    auto savedApnManager = std::move(cellularDataHandler->apnManager_);
    EXPECT_FALSE(cellularDataHandler->CheckApnState(mmsHolder));
    cellularDataHandler->apnManager_ = std::move(savedApnManager);
    // C) DISCONNECTING + 无挂起建立事件 → 补发事件 + false
    mmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_DISCONNECTING);
    EXPECT_FALSE(cellularDataHandler->CheckApnState(mmsHolder));
    // D) RETRYING → false
    mmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_RETRYING);
    EXPECT_FALSE(cellularDataHandler->CheckApnState(mmsHolder));
    // E) FAILED → 复位 IDLE 继续；无匹配 APN → false
    cellularDataHandler->apnManager_->allApnItem_.clear();
    mmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_FAILED);
    EXPECT_FALSE(cellularDataHandler->CheckApnState(mmsHolder));
    EXPECT_EQ(mmsHolder->GetApnState(), ApnProfileState::PROFILE_STATE_IDLE);
    // F) FAILED 复位后有匹配 → SetAllMatchedApns + true
    cellularDataHandler->apnManager_->allApnItem_.push_back(ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS));
    mmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_FAILED);
    EXPECT_TRUE(cellularDataHandler->CheckApnState(mmsHolder));
    // G) CONNECTED → 非 IDLE → false
    mmsHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    EXPECT_FALSE(cellularDataHandler->CheckApnState(mmsHolder));
    UnmockManager();
}

/**
 * @tc.number   UpdateApnInfo_SimInvalid_001
 * @tc.name     test update apn info with invalid sim id
 * @tc.desc     Function test
 * @tc.note     GetSimId 返回 0（=INVALID_SIM_ID）→ 早退，不触达 DataShare。
 */
HWTEST_F(CellularDataHandlerBranchTest, UpdateApnInfo_SimInvalid_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    ON_CALL(*mockSimManager, GetSimId(_)).WillByDefault(Return(0));
    cellularDataHandler->UpdateApnInfo(1);
    UnmockManager();
}

/**
 * @tc.number   GetEsmFlagFromOpCfg_001
 * @tc.name     test get esm flag from operator config
 * @tc.desc     Function test
 * @tc.note     GetOperatorConfigs mock 四变体：键存在=1（true）/=0（false）/键缺失
 *              （ESM_FLAG_INVALID=-1 越界 → return esmFlag!=0 = true）/=5 越界（true）。
 */
HWTEST_F(CellularDataHandlerBranchTest, GetEsmFlagFromOpCfg_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    InitMockManager();
    // A) 键存在 =1 → true
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 1;
            return 0;
        });
    EXPECT_TRUE(cellularDataHandler->GetEsmFlagFromOpCfg());
    // B) 键存在 =0 → false
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 0;
            return 0;
        });
    EXPECT_FALSE(cellularDataHandler->GetEsmFlagFromOpCfg());
    // C) 键缺失 → ESM_FLAG_INVALID(-1) → 越界日志 → return (esmFlag != 0) = true
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _)).WillByDefault([](int32_t, OperatorConfig &) {
        return 0;
    });
    EXPECT_TRUE(cellularDataHandler->GetEsmFlagFromOpCfg());
    // D) 键存在 =5（>1 越界）→ true
    ON_CALL(*mockSimManager, GetOperatorConfigs(_, _))
        .WillByDefault([](int32_t, OperatorConfig &poc) {
            poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 5;
            return 0;
        });
    EXPECT_TRUE(cellularDataHandler->GetEsmFlagFromOpCfg());
    UnmockManager();
}
}  // namespace Telephony
}  // namespace OHOS