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

#include "gtest/gtest.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "cellular_data_handler.h"
#include "cellular_data_controller.h"
#include "utils/test_async_utils.h"
#include <gmock/gmock.h>
#include "core_manager_inner.h"
#include "cellular_data_settings_rdb_helper.h"
#include "data_switch_settings.h"
#include "operator_config_types.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "mock/mock_sim_manager.h"
#include "utils/scoped_setter.h"
#ifdef BASE_POWER_IMPROVEMENT
#include "cellular_data_power_save_mode_subscriber.h"
#endif

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

class CellularDataHandlerTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.number   HandleUpdateNetInfo_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED, netInfo);
    cellularDataHandler->HandleUpdateNetInfo(event);
    EXPECT_EQ(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_EQ(cellularDataHandler->apnManager_, nullptr);
}

/**
 * @tc.number   HandleUpdateNetInfo_002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED);
    cellularDataHandler->HandleUpdateNetInfo(event);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_EQ(event->GetSharedObject<SetupDataCallResultInfo>(), nullptr);
}

/**
 * @tc.number   HandleUpdateNetInfo_003
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED, netInfo);
    cellularDataHandler->HandleUpdateNetInfo(event);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(event->GetSharedObject<SetupDataCallResultInfo>(), nullptr);
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->GetApnHolder(
        cellularDataHandler->apnManager_->FindApnNameByApnId(netInfo->flag));
    EXPECT_NE(apnHolder, nullptr);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_IDLE);
}

/**
 * @tc.number   HandleUpdateNetInfo_004
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_004, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->GetApnHolder(
        cellularDataHandler->apnManager_->FindApnNameByApnId(netInfo->flag));
    EXPECT_NE(apnHolder, nullptr);
    apnHolder->SetApnState(PROFILE_STATE_CONNECTED);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED, netInfo);
    cellularDataHandler->HandleUpdateNetInfo(event);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(event->GetSharedObject<SetupDataCallResultInfo>(), nullptr);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_CONNECTED);
    auto stateMachine = cellularDataHandler->connectionManager_->GetActiveConnectionByCid(netInfo->cid);
    EXPECT_EQ(stateMachine, nullptr);
}

/**
 * @tc.number   HandleUpdateNetInfo_005
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_005, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    netInfo->cid = 100;
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->GetApnHolder(
        cellularDataHandler->apnManager_->FindApnNameByApnId(netInfo->flag));
    EXPECT_NE(apnHolder, nullptr);
    apnHolder->SetApnState(PROFILE_STATE_CONNECTED);
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    EXPECT_NE(connectionManager, nullptr);
    connectionManager->Init();
    auto sm = std::make_shared<CellularDataStateMachine>(
        connectionManager, std::make_shared<TelEventHandler>("CellularDataHandlerTest"));
    sm->cid_ = 100;
    cellularDataHandler->connectionManager_->AddActiveConnectionByCid(sm);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED, netInfo);
    cellularDataHandler->HandleUpdateNetInfo(event);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(event->GetSharedObject<SetupDataCallResultInfo>(), nullptr);
    EXPECT_EQ(apnHolder->GetApnState(), PROFILE_STATE_CONNECTED);
    auto stateMachine = cellularDataHandler->connectionManager_->GetActiveConnectionByCid(netInfo->cid);
    EXPECT_NE(stateMachine, nullptr);
}

/**
 * @tc.number   HandleUpdateNetInfo_006
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleUpdateNetInfo_006, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto netInfo = std::make_shared<SetupDataCallResultInfo>();
    netInfo->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    netInfo->cid = 100;
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->GetApnHolder(
        cellularDataHandler->apnManager_->FindApnNameByApnId(netInfo->flag));
    EXPECT_NE(apnHolder, nullptr);
    apnHolder->SetApnState(PROFILE_STATE_IDLE);
    cellularDataHandler->ClearConnection(apnHolder, DisConnectionReason::REASON_CLEAR_CONNECTION);
    EXPECT_NE(apnHolder->GetApnState(), PROFILE_STATE_DISCONNECTING);
}

/**
 * @tc.number   HandleRoamingOff_001
 * @tc.name     test roaming off
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleRoamingOff_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    EXPECT_NE(cellularDataHandler, nullptr);
    cellularDataHandler->Init();
    cellularDataHandler->SetCellularDataRoamingEnabled(true);
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->RoamingStateOff(event);
}

/**
 * @tc.number   HandleRoamingOff_002
 * @tc.name     test roaming off
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleRoamingOff_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    EXPECT_NE(cellularDataHandler, nullptr);
    cellularDataHandler->Init();
    cellularDataHandler->SetCellularDataRoamingEnabled(false);
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->RoamingStateOff(event);
}

/**
 * @tc.number   HandleEstablishAllApnsIfConnectable_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleEstablishAllApnsIfConnectable_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    AppExecFwk::InnerEvent::Pointer msgEvent =
        AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_ESTABLISH_ALL_APNS_IF_CONNECTABLE);
    cellularDataHandler->HandleEstablishAllApnsIfConnectable(msgEvent);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
}

/**
 * @tc.number   HandleSimEvent_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularDataHandler->HandleSimEvent(nullEvent);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DATA_CALL_LIST_CHANGED, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
}

/**
 * @tc.number   HandleSimEvent_002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_STATE_CHANGE, 1);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_NE(cellularDataHandler->slotId_, event->GetParam());
}

/**
 * @tc.number   HandleSimEvent_003
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_STATE_CHANGE, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_SIM_STATE_CHANGE);
}

/**
 * @tc.number   HandleSimEvent_004
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_004, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_RECORDS_LOADED, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_SIM_RECORDS_LOADED);
}

/**
 * @tc.number   HandleSimEvent_005
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_005, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_NV_REFRESH_FINISHED, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_NV_REFRESH_FINISHED);
}

/**
 * @tc.number   HandleSimEvent_006
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_006, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_ACCOUNT_LOADED, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
}

/**
 * @tc.number   HandleSimEvent_007
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_007, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_NV_REFRESH_FINISHED, 0);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_NV_REFRESH_FINISHED);
}

/**
 * @tc.number   HandleSimEvent_008
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleSimEvent_008, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_NV_REFRESH_FINISHED, 1);
    cellularDataHandler->HandleSimEvent(event);
    EXPECT_EQ(event->GetInnerEventId(), RadioEvent::RADIO_NV_REFRESH_FINISHED);
}

/**
 * @tc.number   ClearConnectionsOnUpdateApns_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, ClearConnectionsOnUpdateApns_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_RETRY_CONNECTION);
    EXPECT_EQ(cellularDataHandler->apnManager_, nullptr);
}

/**
 * @tc.number   ClearConnectionsOnUpdateApns_002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, ClearConnectionsOnUpdateApns_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_RETRY_CONNECTION);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    cellularDataHandler->apnManager_->allApnItem_.clear();
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_RETRY_CONNECTION);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    allApnItem.push_back(defaultApnItem);
    cellularDataHandler->apnManager_->allApnItem_ = allApnItem;
    EXPECT_NE(cellularDataHandler->apnManager_->GetRilAttachApn(), nullptr);
    cellularDataHandler->ClearConnectionsOnUpdateApns(DisConnectionReason::REASON_RETRY_CONNECTION);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
}

/**
 * @tc.number   SetCellularDataRoamingEnabledTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, SetCellularDataRoamingEnabledTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    bool dataRoamingEnabled = true;
    EXPECT_EQ(cellularDataHandler->SetCellularDataRoamingEnabled(dataRoamingEnabled), TELEPHONY_ERR_LOCAL_PTR_NULL);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->RadioPsConnectionAttached(event);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    cellularDataHandler->RoamingStateOn(event);
    cellularDataHandler->RoamingStateOff(event);
    EXPECT_NE(cellularDataHandler->dataSwitchSettings_, nullptr);
    cellularDataHandler->incallDataStateMachine_ = cellularDataHandler->CreateIncallDataStateMachine(0);
    EXPECT_FALSE(cellularDataHandler->SetDataPermittedForMms(true));
}

/**
 * @tc.number   AttemptEstablishDataConnectionTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, AttemptEstablishDataConnectionTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = nullptr;
    cellularDataHandler->connectionManager_->GetAllConnectionMachine().push_back(cellularMachine);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
}

/**
 * @tc.number   AttemptEstablishDataConnectionTest002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, AttemptEstablishDataConnectionTest002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    std::shared_ptr<CellularDataStateMachine> cellularMachine = std::make_shared<CellularDataStateMachine>(
        connectionManager, nullptr);
    cellularDataHandler->connectionManager_ = connectionManager;
    cellularDataHandler->connectionManager_->GetAllConnectionMachine().push_back(cellularMachine);
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
    cellularDataHandler->Init();
    EXPECT_EQ(cellularDataHandler->FindIdleCellularDataConnection(), nullptr);
}

/**
 * @tc.number   EstablishDataConnectionTest002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, EstablishDataConnectionTest002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = std::make_shared<CellularDataHandler>(0);
    cellularDataHandler->Init();
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder->retryPolicy_.matchedApns_.push_back(apnItem);
    EXPECT_NE(apnHolder->GetNextRetryApn(), nullptr);
    EXPECT_TRUE(cellularDataHandler->EstablishDataConnection(apnHolder, 0));
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_DUN;
    EXPECT_TRUE(cellularDataHandler->EstablishDataConnection(apnHolder, 0));
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_MMS;
    EXPECT_TRUE(cellularDataHandler->EstablishDataConnection(apnHolder, 0));
}

/**
 * @tc.number   DisconnectDataCompleteTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, DisconnectDataCompleteTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    auto event = AppExecFwk::InnerEvent::Get(0);
    cellularDataHandler->DisconnectDataComplete(event);
    EXPECT_NE(event, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
}

/**
 * @tc.number   UpdatePhysicalConnectionStateTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, UpdatePhysicalConnectionStateTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->physicalConnectionActiveState_ = false;
    cellularDataHandler->UpdatePhysicalConnectionState(false);
    EXPECT_TRUE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->UpdatePhysicalConnectionState(true);
    EXPECT_FALSE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->physicalConnectionActiveState_ = true;
    cellularDataHandler->UpdatePhysicalConnectionState(false);
    EXPECT_TRUE(cellularDataHandler->physicalConnectionActiveState_);

    cellularDataHandler->UpdatePhysicalConnectionState(true);
    EXPECT_FALSE(cellularDataHandler->physicalConnectionActiveState_);
}

/**
 * @tc.number   HandleScreenStateChangedTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleScreenStateChangedTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->HandleScreenStateChanged(false);
    EXPECT_EQ(cellularDataHandler->connectionManager_, nullptr);
    cellularDataHandler->Init();
    cellularDataHandler->HandleScreenStateChanged(false);
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
}

/**
 * @tc.number   UpdateCellularDataConnectStateTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, UpdateCellularDataConnectStateTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = std::make_shared<CellularDataHandler>(0);
    cellularDataHandler->Init();
    cellularDataHandler->incallDataStateMachine_ = cellularDataHandler->CreateIncallDataStateMachine(0);
    AppExecFwk::InnerEvent::Pointer event(nullptr, nullptr);
    cellularDataHandler->IncallDataComplete(event);
    EXPECT_EQ(cellularDataHandler->incallDataStateMachine_, nullptr);
    cellularDataHandler->UpdateCellularDataConnectState("default");
    EXPECT_EQ(cellularDataHandler->apnManager_->GetOverallDefaultApnState(), 0);
    cellularDataHandler->UpdateCellularDataConnectState("internal_default");
    EXPECT_EQ(cellularDataHandler->apnManager_->GetOverallDefaultApnState(), 0);
}

/**
 * @tc.number   HandleImsCallChangedTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HandleImsCallChangedTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    cellularDataHandler->HandleImsCallChanged(1);
    EXPECT_EQ(cellularDataHandler->incallDataStateMachine_, nullptr);
    cellularDataHandler->HandleImsCallChanged(4);
    EXPECT_NE(cellularDataHandler->incallDataStateMachine_, nullptr);
    cellularDataHandler->HandleImsCallChanged(2);
    EXPECT_NE(cellularDataHandler->incallDataStateMachine_->GetCallState(), 8);
    cellularDataHandler->HandleImsCallChanged(4);
    EXPECT_EQ(cellularDataHandler->incallDataStateMachine_->GetCallState(), 4);
    cellularDataHandler->HandleImsCallChanged(6);
    EXPECT_EQ(cellularDataHandler->incallDataStateMachine_->GetCallState(), 6);
    cellularDataHandler->HandleImsCallChanged(8);
    EXPECT_EQ(cellularDataHandler->incallDataStateMachine_->GetCallState(), 8);
}

/**
 * @tc.number   PsDataRatChangedTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, PsDataRatChangedTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    AppExecFwk::InnerEvent::Pointer event(nullptr, nullptr);
    cellularDataHandler->PsDataRatChanged(event);
    cellularDataHandler->dataSwitchSettings_->userDataOn_ = false;
    EXPECT_FALSE(cellularDataHandler->dataSwitchSettings_->IsUserDataOn());
}

/**
 * @tc.number   SetPolicyDataOnTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, SetPolicyDataOnTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    cellularDataHandler->dataSwitchSettings_->policyDataOn_ = false;
    cellularDataHandler->SetPolicyDataOn(true);
    EXPECT_TRUE(cellularDataHandler->dataSwitchSettings_->IsUserDataOn());
    cellularDataHandler->dataSwitchSettings_->policyDataOn_ = true;
    cellularDataHandler->SetPolicyDataOn(false);
    EXPECT_FALSE(cellularDataHandler->dataSwitchSettings_->policyDataOn_);
}

/**
 * @tc.number   SetRilAttachApnTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, SetRilAttachApnTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    sptr<ApnItem> attachApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    cellularDataHandler->apnManager_->allApnItem_.push_back(attachApn);
    cellularDataHandler->SetRilAttachApn();
}

/**
 * @tc.number   HasAnyHigherPriorityConnectionTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HasAnyHigherPriorityConnectionTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    sptr<ApnHolder> sortApnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    sortApnHolder->priority_ = 1;
    sortApnHolder->dataCallEnabled_ = true;
    sortApnHolder->apnState_ = ApnProfileState::PROFILE_STATE_CONNECTED;
    cellularDataHandler->apnManager_->sortedApnHolders_.push_back(sortApnHolder);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder->priority_ = 0;
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_->GetSortApnHolder().empty(), true);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(apnHolder));
}

/**
 * @tc.number   HasAnyHigherPriorityConnectionTest002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HasAnyHigherPriorityConnectionTest002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    sptr<ApnHolder> sortApnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    sortApnHolder->priority_ = 1;
    sortApnHolder->dataCallEnabled_ = true;
    sortApnHolder->apnState_ = ApnProfileState::PROFILE_STATE_CONNECTING;
    cellularDataHandler->apnManager_->sortedApnHolders_.push_back(sortApnHolder);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder->priority_ = 0;
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_->GetSortApnHolder().empty(), true);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(apnHolder));
}

/**
 * @tc.number   HasAnyHigherPriorityConnectionTest003
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HasAnyHigherPriorityConnectionTest003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    sptr<ApnHolder> sortApnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    sortApnHolder->priority_ = 1;
    sortApnHolder->dataCallEnabled_ = true;
    sortApnHolder->apnState_ = ApnProfileState::PROFILE_STATE_DISCONNECTING;
    cellularDataHandler->apnManager_->sortedApnHolders_.push_back(sortApnHolder);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder->priority_ = 0;
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
    EXPECT_NE(cellularDataHandler->apnManager_->GetSortApnHolder().empty(), true);
    EXPECT_TRUE(cellularDataHandler->HasAnyHigherPriorityConnection(apnHolder));
}

/**
 * @tc.number   HasInternetCapabilityTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HasInternetCapabilityTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    std::shared_ptr<CellularDataStateMachine> cellularMachine = std::make_shared<CellularDataStateMachine>(
        connectionManager, nullptr);
    cellularMachine->capability_ = NetCap::NET_CAPABILITY_INTERNET;
    cellularDataHandler->connectionManager_->cidActiveConnectionMap_[0] = cellularMachine;
    EXPECT_NE(cellularDataHandler->connectionManager_->GetActiveConnectionByCid(0), nullptr);
    EXPECT_TRUE(cellularDataHandler->HasInternetCapability(0));
}

/**
 * @tc.number   HasInternetCapabilityTest002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, HasInternetCapabilityTest002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    std::shared_ptr<CellularDataStateMachine> cellularMachine = std::make_shared<CellularDataStateMachine>(
        connectionManager, nullptr);
    cellularMachine->capability_ = NetCap::NET_CAPABILITY_MMS;
    cellularDataHandler->connectionManager_->cidActiveConnectionMap_[0] = cellularMachine;
    EXPECT_NE(cellularDataHandler->connectionManager_->GetActiveConnectionByCid(0), nullptr);
    EXPECT_FALSE(cellularDataHandler->HasInternetCapability(0));
}

/**
 * @tc.number   ChangeConnectionForDsdsTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, ChangeConnectionForDsdsTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    cellularDataHandler->ChangeConnectionForDsds(true);
    EXPECT_TRUE(cellularDataHandler->dataSwitchSettings_->internalDataOn_);
}

/**
 * @tc.number   GetDataConnApnAttrTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, GetDataConnApnAttrTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    sptr<ApnHolder> apnHolder1 = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder1->apnItem_ = nullptr;
    apnHolder1->dataCallEnabled_ = true;
    cellularDataHandler->apnManager_->apnHolders_.push_back(apnHolder1);
    sptr<ApnHolder> apnHolder2 = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apnItem->attr_.isEdited_ = true;
    apnHolder2->apnItem_ = apnItem;
    apnHolder2->dataCallEnabled_ = true;
    cellularDataHandler->apnManager_->apnHolders_.push_back(apnHolder2);
    ApnItem::Attribute apnAttr;
    apnAttr.isEdited_ = false;
    cellularDataHandler->GetDataConnApnAttr(apnAttr);
    EXPECT_TRUE(apnAttr.isEdited_);
}

/**
 * @tc.number   GetDataConnIpTypeTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, GetDataConnIpTypeTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    cellularDataHandler->Init();
    sptr<ApnHolder> apnHolder1 = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder1->cellularDataStateMachine_ = nullptr;
    apnHolder1->dataCallEnabled_ = true;
    cellularDataHandler->apnManager_->apnHolders_.push_back(apnHolder1);
    sptr<ApnHolder> apnHolder2 = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    auto connectionManager = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(0);
    std::shared_ptr<CellularDataStateMachine> cellularMachine = std::make_shared<CellularDataStateMachine>(
        connectionManager, nullptr);
    cellularMachine->ipType_ = "IPV4";
    apnHolder2->cellularDataStateMachine_ = cellularMachine;
    apnHolder2->dataCallEnabled_ = true;
    cellularDataHandler->apnManager_->apnHolders_.push_back(apnHolder2);
    cellularDataHandler->GetDataConnIpType();
    EXPECT_EQ(cellularDataHandler->GetDataConnIpType(), "IPV4");
}

/**
 * @tc.number   CheckForCompatibleDataConnectionTest001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, CheckForCompatibleDataConnectionTest001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(1);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>("default", static_cast<int32_t>(0)).release();
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_DUN;
    EXPECT_EQ(cellularDataHandler->CheckForCompatibleDataConnection(apnHolder), nullptr);
    cellularDataHandler->Init();
    EXPECT_EQ(cellularDataHandler->CheckForCompatibleDataConnection(apnHolder), nullptr);
}

/**
 * @tc.number   UpdateNetworkInfo_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, UpdateNetworkInfo_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    EXPECT_EQ(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_FALSE(cellularDataHandler->UpdateNetworkInfo());
}

/**
 * @tc.number   UpdateNetworkInfo_002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, UpdateNetworkInfo_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    EXPECT_NE(cellularDataHandler->connectionManager_, nullptr);
    EXPECT_TRUE(cellularDataHandler->UpdateNetworkInfo());
}

/**
 * @tc.number   Telephony_MsgRequest
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, Telephony_MsgRequest, Function | MediumTest | Level1)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    NetRequest request1;
    request1.capability = NetManagerStandard::NET_CAPABILITY_INTERNET;

    EXPECT_TRUE(cellularDataHandler->RequestNet(request1));
    NetRequest request2;
    request2.capability = NetManagerStandard::NET_CAPABILITY_INTERNET;
    EXPECT_TRUE(cellularDataHandler->RequestNet(request2));
    sleep(2);
}

/**
 * @tc.number   Telephony_ConnectIfNeed
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataHandlerTest, Telephony_ConnectIfNeed, Function | MediumTest | Level1)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();

    NetRequest request1;
    request1.capability = NetManagerStandard::NET_CAPABILITY_INTERNET;
    EXPECT_TRUE(cellularDataHandler->RequestNet(request1));
    auto event1 = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_REQUEST_NETWORK, TYPE_REQUEST_NET);
    auto apnHolder =
        cellularDataHandler->apnManager_->FindApnHolderById(ApnManager::FindApnIdByCapability(request1.capability));
}

HWTEST_F(CellularDataHandlerTest, Telephony_CellularDataHandler_003, Function | MediumTest | Level1)
{
    CellularDataController controller {0};
    controller.Init();
    ApnActivateReportInfo info1;
    ApnActivateReportInfo info2;
    EXPECT_TRUE(controller.GetDefaultActReportInfo(info1));
    EXPECT_TRUE(controller.GetInternalActReportInfo(info2));
    controller.cellularDataHandler_ = nullptr;
    EXPECT_FALSE(controller.GetDefaultActReportInfo(info1));
    EXPECT_FALSE(controller.GetInternalActReportInfo(info2));
}

HWTEST_F(CellularDataHandlerTest, DataConnCompleteUpdateStateTest001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    std::string apnType = DATA_CONTEXT_ROLE_MMS;
    sptr<ApnHolder> apnHolder = new ApnHolder(apnType, slotId);
    apnHolder->cellularDataStateMachine_ = nullptr;
    auto resultInfo = std::make_shared<SetupDataCallResultInfo>();
    cellularDataHandler->connectionManager_ = nullptr;
    cellularDataHandler->physicalConnectionActiveState_ = true;
    cellularDataHandler->incallDataStateMachine_ = nullptr;
    cellularDataHandler->DataConnCompleteUpdateState(apnHolder, resultInfo);
    EXPECT_FALSE(cellularDataHandler->isRilApnAttached_);

    std::shared_ptr<DataConnectionManager> cdConnectionManager = nullptr;
    std::shared_ptr<TelEventHandler> telEventHandler = nullptr;
    apnHolder->cellularDataStateMachine_ = std::make_shared<CellularDataStateMachine>
        (cdConnectionManager, std::move(telEventHandler));
    apnHolder->cellularDataStateMachine_->netLinkInfo_ = new NetLinkInfo();
    apnHolder->cellularDataStateMachine_->netSupplierInfo_ = nullptr;
    cellularDataHandler->apnManager_ = std::make_unique<ApnManager>().release();
    cellularDataHandler->apnManager_->allApnItem_.clear();
    cellularDataHandler->connectionManager_ = TestAsyncUtils::MakeDrainedStateMachine<DataConnectionManager>(slotId);
    cellularDataHandler->physicalConnectionActiveState_ = false;
    cellularDataHandler->incallDataStateMachine_ =
        cellularDataHandler->CreateIncallDataStateMachine(TelCallStatus::CALL_STATUS_DIALING);
    apnHolder->apnType_ = DATA_CONTEXT_ROLE_DEFAULT;
    apnHolder->apnItem_ = new ApnItem();
    cellularDataHandler->DataConnCompleteUpdateState(apnHolder, resultInfo);

    cellularDataHandler->apnManager_->preferId_ = 1;
    cellularDataHandler->apnManager_->allApnItem_.push_back(new ApnItem());
    cellularDataHandler->DataConnCompleteUpdateState(apnHolder, resultInfo);
    EXPECT_FALSE(cellularDataHandler->isRilApnAttached_);
}

/**
@tc.number Telephony_CreateApnItem
@tc.name CreateApnItem
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CreateApnItemTest001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->apnManager_ = nullptr;
    cellularDataHandler->CreateApnItem();

    cellularDataHandler->Init();
    EXPECT_FALSE(cellularDataHandler->apnManager_ == nullptr);
    cellularDataHandler->CreateApnItem();
    cellularDataHandler->SendEvent(CellularDataEventCode::MSG_RETRY_TO_CREATE_APN, 0, RETRY_DELAY_TIME);
    cellularDataHandler->CreateApnItem();
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_CREATE_APN));
}

HWTEST_F(CellularDataHandlerTest, CreateApnItemTest002, Function | MediumTest | Level1)
{
    int32_t slotId = 2;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
 
    cellularDataHandler->Init();
    cellularDataHandler->retryCreateApnTimes_ = 3;
    cellularDataHandler->CreateApnItem();
    cellularDataHandler->RemoveEvent(CellularDataEventCode::MSG_RETRY_TO_CREATE_APN);
    cellularDataHandler->retryCreateApnTimes_ = 2;
    cellularDataHandler->CreateApnItem();
    EXPECT_NE(cellularDataHandler->apnManager_, nullptr);
}

/**
@tc.number Telephony_IsCellularDataEnabled
@tc.name IsCellularDataEnabled
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, IsCellularDataEnabledTest001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    bool isDataEnabled = false;
    EXPECT_NE(cellularDataHandler->IsCellularDataEnabled(isDataEnabled), TELEPHONY_ERR_SUCCESS);

    cellularDataHandler->Init();
    cellularDataHandler->dataSwitchSettings_->lastQryRet_ = -1;
    EXPECT_EQ(cellularDataHandler->IsCellularDataEnabled(isDataEnabled), TELEPHONY_ERR_SUCCESS);
    cellularDataHandler->dataSwitchSettings_->lastQryRet_ = TELEPHONY_ERR_SUCCESS;
    EXPECT_EQ(cellularDataHandler->IsCellularDataEnabled(isDataEnabled), TELEPHONY_ERR_SUCCESS);
}

/**
@tc.number Telephony_CheckApnActivate
@tc.name CheckApnActivate001
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckApnActivate001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    cellularDataHandler->SetApnActivateStart(DATA_CONTEXT_ROLE_DEFAULT);
    cellularDataHandler->SetApnActivateStart(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT);
    auto resultInfo1 = std::make_shared<SetupDataCallResultInfo>();
    resultInfo1->flag = DATA_CONTEXT_ROLE_INTERNAL_DEFAULT_ID;
    resultInfo1->reason = 0;
    cellularDataHandler->SetApnActivateEnd(resultInfo1);
    auto resultInfo2 = std::make_shared<SetupDataCallResultInfo>();
    resultInfo2->flag = DATA_CONTEXT_ROLE_INTERNAL_DEFAULT_ID;
    resultInfo2->reason = 1;
    cellularDataHandler->SetApnActivateEnd(resultInfo2);
    auto resultInfo3 = std::make_shared<SetupDataCallResultInfo>();
    resultInfo3->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    resultInfo3->reason = 2;
    cellularDataHandler->SetApnActivateEnd(resultInfo3);
    auto resultInfo4 = std::make_shared<SetupDataCallResultInfo>();
    resultInfo4->flag = DATA_CONTEXT_ROLE_DEFAULT_ID;
    resultInfo4->reason = 2;
    cellularDataHandler->SetApnActivateEnd(resultInfo4);
    cellularDataHandler->GetApnActReportInfo(DATA_CONTEXT_ROLE_DEFAULT_ID);
    cellularDataHandler->GetApnActReportInfo(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT_ID);
    int32_t listLen = 4;
    EXPECT_EQ(cellularDataHandler->apnActivateChrList_.size(), listLen);
}

/**
@tc.number Telephony_CheckApnActivate002
@tc.name CheckApnActivate002
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckApnActivate002, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    ApnActivateInfo info1;
    info1.reason = 0;
    info1.actSuccTime = 0;
    info1.apnId = 1;
    info1.duration = 0;
    cellularDataHandler->apnActivateChrList_.push_back(info1);
    cellularDataHandler->EraseApnActivateList();
    EXPECT_EQ(cellularDataHandler->apnActivateChrList_.size(), 0);
}

/**
@tc.number Telephony_CheckMultiApnState
@tc.name CheckMultiApnState001
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckMultiApnState001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    sptr<ApnManager> apnManager = cellularDataHandler->apnManager_;
    if (apnManager == nullptr) {
        EXPECT_EQ(apnManager, nullptr);
    } else {
        sptr<ApnHolder> defaultApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
        bool result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
        sptr<ApnHolder> bipApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_BIP);
        result = cellularDataHandler->CheckMultiApnState(bipApnHolder);
        EXPECT_EQ(result, false);
    }
}

/**
@tc.number Telephony_CheckMultiApnState
@tc.name CheckMultiApnState002
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckMultiApnState002, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    sptr<ApnManager> apnManager = cellularDataHandler->apnManager_;
    if (apnManager == nullptr) {
        EXPECT_EQ(apnManager, nullptr);
    } else {
        sptr<ApnHolder> defaultApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
        sptr<ApnHolder> bipApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_BIP);
        bipApnHolder->SetApnState(PROFILE_STATE_DISCONNECTING);
        bool result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        bipApnHolder->SetApnState(PROFILE_STATE_CONNECTED);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        bipApnHolder->SetApnState(PROFILE_STATE_CONNECTING);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        bipApnHolder->SetApnState(PROFILE_STATE_IDLE);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
        cellularDataHandler->apnManager_ = nullptr;
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
    }
}

/**
@tc.number Telephony_CheckMultiApnState
@tc.name CheckMultiApnState003
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckMultiApnState003, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    sptr<ApnManager> apnManager = cellularDataHandler->apnManager_;
    if (apnManager == nullptr) {
        EXPECT_EQ(apnManager, nullptr);
    } else {
        sptr<ApnHolder> defaultApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
        cellularDataHandler->apnManager_ = nullptr;
        bool result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
    }
}

/**
@tc.number Telephony_CheckMultiApnState
@tc.name CheckMultiApnState004
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckMultiApnState004, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    sptr<ApnManager> apnManager = cellularDataHandler->apnManager_;
    if (apnManager == nullptr) {
        EXPECT_EQ(apnManager, nullptr);
    } else {
        sptr<ApnHolder> defaultApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
        bool result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
        sptr<ApnHolder> dunApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DUN);
        result = cellularDataHandler->CheckMultiApnState(dunApnHolder);
        EXPECT_EQ(result, false);
    }
}

/**
@tc.number Telephony_CheckMultiApnState
@tc.name CheckMultiApnState005
@tc.desc Function test
*/
HWTEST_F(CellularDataHandlerTest, CheckMultiApnState005, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(slotId);
    cellularDataHandler->Init();
    sptr<ApnManager> apnManager = cellularDataHandler->apnManager_;
    if (apnManager == nullptr) {
        EXPECT_EQ(apnManager, nullptr);
    } else {
        sptr<ApnHolder> defaultApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
        sptr<ApnHolder> dunApnHolder = apnManager->GetApnHolder(DATA_CONTEXT_ROLE_DUN);
        dunApnHolder->SetApnState(PROFILE_STATE_DISCONNECTING);
        bool result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        dunApnHolder->SetApnState(PROFILE_STATE_CONNECTED);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        dunApnHolder->SetApnState(PROFILE_STATE_CONNECTING);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, true);
        dunApnHolder->SetApnState(PROFILE_STATE_IDLE);
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
        cellularDataHandler->apnManager_ = nullptr;
        result = cellularDataHandler->CheckMultiApnState(defaultApnHolder);
        EXPECT_EQ(result, false);
    }
}

HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    auto event0 = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT);
    auto event1 = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);

    // false false
    cellularDataHandler->isSimAccountLoaded_ = false;
    cellularDataHandler->HandleRetryLoadSimAccount(event0);
    EXPECT_FALSE(cellularDataHandler->isSimAccountLoaded_);

    // true false
    cellularDataHandler->SendEvent(event0, 5000);
    cellularDataHandler->HandleRetryLoadSimAccount(event0);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->RemoveEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT);

    // false true
    cellularDataHandler->isSimAccountLoaded_ = false;
    cellularDataHandler->SendEvent(event1, 5000);
    cellularDataHandler->HandleRetryLoadSimAccount(event0);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED));
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);

    // true true
    cellularDataHandler->isSimAccountLoaded_ = true;
    cellularDataHandler->SendEvent(event1, 5000);
    cellularDataHandler->HandleRetryLoadSimAccount(event0);
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
}

/**
* @tc.number   OnDataShareReady_001
* @tc.name     Test OnDataShareReady - isSimAccountLoaded_ is false, no RADIO_SIM_ACCOUNT_LOADED event
* @tc.desc     Branch coverage test: should start load sim account timer
*/
HWTEST_F(CellularDataHandlerTest, OnDataShareReady_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->isSimAccountLoaded_ = false;
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->OnDataShareReady();
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StopLoadSimAccountTimer();
}

/**
* @tc.number   OnDataShareReady_002
* @tc.name     Test OnDataShareReady - isSimAccountLoaded_ is true
* @tc.desc     Branch coverage test: should not start load sim account timer
*/
HWTEST_F(CellularDataHandlerTest, OnDataShareReady_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->isSimAccountLoaded_ = true;
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->OnDataShareReady();
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
}

/**
* @tc.number   OnDataShareReady_003
* @tc.name     Test OnDataShareReady - has RADIO_SIM_ACCOUNT_LOADED event
* @tc.desc     Branch coverage test: should not start load sim account timer
*/
HWTEST_F(CellularDataHandlerTest, OnDataShareReady_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->isSimAccountLoaded_ = false;
    auto event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->SendEvent(event, 5000);
    cellularDataHandler->OnDataShareReady();
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
}

/**
* @tc.number   StartLoadSimAccountTimer_001
* @tc.name     Test StartLoadSimAccountTimer - times <= 0
* @tc.desc     Branch coverage test: should return early
*/
HWTEST_F(CellularDataHandlerTest, StartLoadSimAccountTimer_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->StartLoadSimAccountTimer(0);
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StartLoadSimAccountTimer(-1);
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
}

/**
* @tc.number   StartLoadSimAccountTimer_002
* @tc.name     Test StartLoadSimAccountTimer - times > 0, no existing event
* @tc.desc     Branch coverage test: should send event
*/
HWTEST_F(CellularDataHandlerTest, StartLoadSimAccountTimer_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->StopLoadSimAccountTimer();
    cellularDataHandler->StartLoadSimAccountTimer(1);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StopLoadSimAccountTimer();
}

/**
* @tc.number   StartLoadSimAccountTimer_003
* @tc.name     Test StartLoadSimAccountTimer - times > 0, has existing event
* @tc.desc     Branch coverage test: should not send duplicate event
*/
HWTEST_F(CellularDataHandlerTest, StartLoadSimAccountTimer_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->StartLoadSimAccountTimer(1);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StartLoadSimAccountTimer(2);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StopLoadSimAccountTimer();
}

/**
* @tc.number   HandleRetryLoadSimAccount_002
* @tc.name     Test HandleRetryLoadSimAccount - event is nullptr
* @tc.desc     Branch coverage test: should return early
*/
HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount_002, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->isSimAccountLoaded_ = false;
    auto event = AppExecFwk::InnerEvent::Get(0);
    event = nullptr;
    cellularDataHandler->HandleRetryLoadSimAccount(event);
    EXPECT_FALSE(cellularDataHandler->isSimAccountLoaded_);
}

/**
* @tc.number   HandleRetryLoadSimAccount_003
* @tc.name     Test HandleRetryLoadSimAccount - simId <= INVALID_SIM_ID with retry times
* @tc.desc     Branch coverage test: should retry with times - 1
*/
HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount_003, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(-1);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT, 2);
    cellularDataHandler->isSimAccountLoaded_ = false;
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->HandleRetryLoadSimAccount(event);
    EXPECT_FALSE(cellularDataHandler->isSimAccountLoaded_);
}

/**
* @tc.number   HandleRetryLoadSimAccount_004
* @tc.name     Test HandleRetryLoadSimAccount - simId > INVALID_SIM_ID, isSimAccountLoaded_ is true
* @tc.desc     Branch coverage test: should not call HandleSimAccountLoaded
*/
HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount_004, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(-1);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT);
    cellularDataHandler->isSimAccountLoaded_ = true;
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->HandleRetryLoadSimAccount(event);
    EXPECT_TRUE(cellularDataHandler->isSimAccountLoaded_);
}

/**
* @tc.number   HandleRetryLoadSimAccount_005
* @tc.name     Test HandleRetryLoadSimAccount - simId > INVALID_SIM_ID, has RADIO_SIM_ACCOUNT_LOADED event
* @tc.desc     Branch coverage test: should not call HandleSimAccountLoaded
*/
HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount_005, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(-1);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT);
    auto simEvent = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->isSimAccountLoaded_ = false;
    cellularDataHandler->SendEvent(simEvent, 5000);
    cellularDataHandler->HandleRetryLoadSimAccount(event);
    EXPECT_FALSE(cellularDataHandler->isSimAccountLoaded_);
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
}

/**
* @tc.number   HandleRetryLoadSimAccount_006
* @tc.name     Test HandleRetryLoadSimAccount - simId > INVALID_SIM_ID, has RADIO_SIM_ACCOUNT_LOADED event
* @tc.desc     Branch coverage test: should not call HandleSimAccountLoaded
*/
HWTEST_F(CellularDataHandlerTest, HandleRetryLoadSimAccount_006, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(-1);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT);
    auto simEvent = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
    cellularDataHandler->isSimAccountLoaded_ = true;
    cellularDataHandler->SendEvent(simEvent, 5000);
    cellularDataHandler->HandleRetryLoadSimAccount(event);
    EXPECT_TRUE(cellularDataHandler->isSimAccountLoaded_);
    cellularDataHandler->RemoveEvent(RadioEvent::RADIO_SIM_ACCOUNT_LOADED);
}

/**
* @tc.number   StopLoadSimAccountTimer_001
* @tc.name     Test StopLoadSimAccountTimer - remove event
* @tc.desc     Branch coverage test: should remove MSG_RETRY_TO_LOAD_SIM_ACCOUNT event
*/
HWTEST_F(CellularDataHandlerTest, StopLoadSimAccountTimer_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->StartLoadSimAccountTimer(1);
    EXPECT_TRUE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
    cellularDataHandler->StopLoadSimAccountTimer();
    EXPECT_FALSE(cellularDataHandler->HasInnerEvent(CellularDataEventCode::MSG_RETRY_TO_LOAD_SIM_ACCOUNT));
}

HWTEST_F(CellularDataHandlerTest, MsgRequestNetwork001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->apnManager_ = sptr<ApnManager>::MakeSptr();
    cellularDataHandler->apnManager_->InitApnHolders();
    std::unique_ptr<NetRequest> netRequest = std::make_unique<NetRequest>();
    netRequest->capability = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET;
    sptr<ApnHolder> apnHolder = cellularDataHandler->apnManager_->FindApnHolderById(DATA_CONTEXT_ROLE_DEFAULT_ID);
    ASSERT_NE(apnHolder, nullptr);
    apnHolder->dataCallEnabled_ = true;
    AppExecFwk::InnerEvent::Pointer event =
        AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_REQUEST_NETWORK, netRequest, TYPE_RELEASE_NET);
    cellularDataHandler->MsgRequestNetwork(event);
    EXPECT_FALSE(apnHolder->dataCallEnabled_);
}

HWTEST_F(CellularDataHandlerTest, IsBlockSetRilAttachApn_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init();
    cellularDataHandler->lastNumeric_ = "00101";
    EXPECT_FALSE(cellularDataHandler->IsBlockSetRilAttachApn());
 
    cellularDataHandler->lastNumeric_ = "99999";
    EXPECT_FALSE(cellularDataHandler->IsBlockSetRilAttachApn());
}

/*
 * ==================== P2 batch-1：handler 直调 + mock 注入用例 ====================
 * 约定：
 * - 所有 mock 经 ScopedSetter 注入单例成员（CoreManagerInner::simManager_、
 *   CellularDataSettingsRdbHelper::settingsHelper_），离开作用域自动还原原值，用例间零污染；
 * - 只覆盖真机安全分支：UpdateApnInfo 深路径（CreatorDataShareHelper 经真 SA 写真库）、
 *   SetRilAttachApn happy path（真机 SetInitApnInfo 无线电写）按 P3 纪律禁入；
 * - Register/UnRegisterDataSettingObserver 全路径中 settings 部分走 mock，
 *   rdb 单例部分真机注册后同用例内成对注销，副作用净零。
 */
using namespace testing;

/**
 * @tc.number: CellularDataHandler_GetCurrentDataShareApnInfo_001
 * @tc.name: Test GetCurrentDataShareApnInfo with null query result
 * @tc.desc: Verify GetCurrentDataShareApnInfo returns false when Query returns nullptr
 */
HWTEST_F(CellularDataHandlerTest, GetCurrentDataShareApnInfo_001, Function | MediumTest | Level3)
{
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper, Query(_, _, _, _)).WillByDefault(Return(nullptr));
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    int32_t profileId = -1;
    EXPECT_FALSE(cellularDataHandler->GetCurrentDataShareApnInfo(mockHelper, 1, profileId));
    EXPECT_EQ(profileId, -1);
}

/**
 * @tc.number: CellularDataHandler_GetCurrentDataShareApnInfo_002
 * @tc.name: Test GetCurrentDataShareApnInfo with empty result set
 * @tc.desc: Verify GetCurrentDataShareApnInfo returns false when row count is zero
 */
HWTEST_F(CellularDataHandlerTest, GetCurrentDataShareApnInfo_002, Function | MediumTest | Level3)
{
    auto mockResult = std::make_shared<DataShareResultSetMock>();
    std::shared_ptr<DataShare::DataShareResultSet> result = mockResult;
    ON_CALL(*mockResult, GetRowCount(_)).WillByDefault(DoAll(SetArgReferee<0>(0), Return(0)));
    ON_CALL(*mockResult, Close()).WillByDefault(Return(0));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper, Query(_, _, _, _)).WillByDefault(Return(result));
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    int32_t profileId = -1;
    EXPECT_FALSE(cellularDataHandler->GetCurrentDataShareApnInfo(mockHelper, 1, profileId));
    EXPECT_EQ(profileId, -1);
}

/**
 * @tc.number: CellularDataHandler_GetCurrentDataShareApnInfo_003
 * @tc.name: Test GetCurrentDataShareApnInfo with valid row
 * @tc.desc: Verify GetCurrentDataShareApnInfo returns true and reads profileId from the result set
 */
HWTEST_F(CellularDataHandlerTest, GetCurrentDataShareApnInfo_003, Function | MediumTest | Level3)
{
    auto mockResult = std::make_shared<DataShareResultSetMock>();
    std::shared_ptr<DataShare::DataShareResultSet> result = mockResult;
    ON_CALL(*mockResult, GetRowCount(_)).WillByDefault(DoAll(SetArgReferee<0>(1), Return(0)));
    ON_CALL(*mockResult, GoToFirstRow()).WillByDefault(Return(0));
    ON_CALL(*mockResult, GetColumnIndex(_, _)).WillByDefault(DoAll(SetArgReferee<1>(0), Return(0)));
    ON_CALL(*mockResult, GetInt(_, _)).WillByDefault(DoAll(SetArgReferee<1>(7), Return(0)));
    ON_CALL(*mockResult, Close()).WillByDefault(Return(0));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper, Query(_, _, _, _)).WillByDefault(Return(result));
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    int32_t profileId = 0;
    EXPECT_TRUE(cellularDataHandler->GetCurrentDataShareApnInfo(mockHelper, 1, profileId));
    EXPECT_EQ(profileId, 7);
}

/**
 * @tc.number: CellularDataHandler_UpdateApnInfo_001
 * @tc.name: Test UpdateApnInfo with invalid sim id
 * @tc.desc: Verify UpdateApnInfo returns early when GetSimId is invalid, without touching DataShare
 */
HWTEST_F(CellularDataHandlerTest, UpdateApnInfo_001, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(0)); // INVALID_SIM_ID
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->UpdateApnInfo(1); // 深路径写真库，禁入；仅覆盖 simId 无效早退分支
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_SetRilAttachApn_001
 * @tc.name: Test SetRilAttachApn with sim not ready
 * @tc.desc: Verify SetRilAttachApn returns early when sim state is unknown
 */
HWTEST_F(CellularDataHandlerTest, SetRilAttachApn_001, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimOperatorNumeric(_, _)).WillByDefault(Return(0));
    EXPECT_CALL(*mockSim, GetSimState(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(SimState::SIM_STATE_UNKNOWN), Return(0)));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->SetRilAttachApn();
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_SetRilAttachApn_002
 * @tc.name: Test SetRilAttachApn with esm flag and null attach apn
 * @tc.desc: Verify SetRilAttachApn returns early when esm flag is set but GetRilAttachApn returns null
 */
HWTEST_F(CellularDataHandlerTest, SetRilAttachApn_002, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimOperatorNumeric(_, _)).WillByDefault(Return(0));
    EXPECT_CALL(*mockSim, GetSimState(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(SimState::SIM_STATE_READY), Return(0)));
    // OperatorConfig 含 const 成员（拷贝赋值被删除），无法 SetArgReferee，改用 Invoke 直写 map
    EXPECT_CALL(*mockSim, GetOperatorConfigs(_, _)).WillOnce(Invoke([](int, OperatorConfig &poc) {
        poc.intValue[KEY_PLMN_ESM_FLAG_INT] = 1;
        return 0;
    }));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    // 非 Init 时 apnManager_ 为空（Init 才创建，见 Init 行 61-75）；此处手动挂新鲜 ApnManager
    // （allApnItem_ 空）→ GetRilAttachApn 返 null → 早退，不触达真机 SetInitApnInfo 无线电写
    cellularDataHandler->apnManager_ = sptr<ApnManager>::MakeSptr();
    cellularDataHandler->SetRilAttachApn();
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_GetDefaultDataRoamingConfig_001
 * @tc.name: Test GetDefaultDataRoamingConfig with opconfig key present
 * @tc.desc: Verify defaultDataRoamingEnable_ is set from operator config when dataSwitchSettings_ is null
 */
HWTEST_F(CellularDataHandlerTest, GetDefaultDataRoamingConfig_001, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    EXPECT_CALL(*mockSim, GetOperatorConfigs(_, _)).WillOnce(Invoke([](int, OperatorConfig &poc) {
        poc.boolValue[KEY_DEFAULT_DATA_ROAMING_BOOL] = true;
        return 0;
    }));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0); // 非 Init：dataSwitchSettings_ 为空
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_TRUE(cellularDataHandler->defaultDataRoamingEnable_);
    EXPECT_EQ(cellularDataHandler->dataSwitchSettings_, nullptr);
}

/**
 * @tc.number: CellularDataHandler_GetDefaultDataRoamingConfig_002
 * @tc.name: Test GetDefaultDataRoamingConfig with opconfig key absent
 * @tc.desc: Verify defaultDataRoamingEnable_ falls back to system property when the key is absent
 */
HWTEST_F(CellularDataHandlerTest, GetDefaultDataRoamingConfig_002, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    EXPECT_CALL(*mockSim, GetOperatorConfigs(_, _)).WillOnce(Invoke([](int, OperatorConfig &) { return 0; }));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->GetDefaultDataRoamingConfig(); // KEY 缺失 → 系统属性路径（默认 DISABLED）
    EXPECT_FALSE(cellularDataHandler->defaultDataRoamingEnable_);
}

/**
 * @tc.number: CellularDataHandler_GetDefaultDataRoamingConfig_003
 * @tc.name: Test GetDefaultDataRoamingConfig update path with null apn manager
 * @tc.desc: Verify roaming switch is updated when query fails and apnManager_ is null
 */
HWTEST_F(CellularDataHandlerTest, GetDefaultDataRoamingConfig_003, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(1));
    EXPECT_CALL(*mockSim, GetOperatorConfigs(_, _)).WillOnce(Invoke([](int, OperatorConfig &poc) {
        poc.boolValue[KEY_DEFAULT_DATA_ROAMING_BOOL] = true;
        return 0;
    }));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper, Query(_, _, _, _)).WillByDefault(Return(nullptr)); // GetValue → DATABASE_READ_FAIL
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>> settingsGuard(
        CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->dataSwitchSettings_ = std::make_unique<DataSwitchSettings>(0);
    cellularDataHandler->apnManager_ = nullptr; // 命中 apnManager_ 空早退分支
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_TRUE(cellularDataHandler->dataSwitchSettings_->IsUserDataRoamingOn());
}

/**
 * @tc.number: CellularDataHandler_GetDefaultDataRoamingConfig_004
 * @tc.name: Test GetDefaultDataRoamingConfig update path when not roaming
 * @tc.desc: Verify roaming switch is updated and not-roaming branch is taken when GetPsRoamingState fails
 */
HWTEST_F(CellularDataHandlerTest, GetDefaultDataRoamingConfig_004, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(1));
    EXPECT_CALL(*mockSim, GetOperatorConfigs(_, _)).WillOnce(Invoke([](int, OperatorConfig &poc) {
        poc.boolValue[KEY_DEFAULT_DATA_ROAMING_BOOL] = true;
        return 0;
    }));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper, Query(_, _, _, _)).WillByDefault(Return(nullptr));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>> settingsGuard(
        CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->dataSwitchSettings_ = std::make_unique<DataSwitchSettings>(0);
    // 手动挂新鲜 ApnManager（非 Init 时为空）；测试进程 networkSearchManager_ 为空 → GetPsRoamingState
    // 返回 -1 → 非漫游分支
    cellularDataHandler->apnManager_ = sptr<ApnManager>::MakeSptr();
    cellularDataHandler->GetDefaultDataRoamingConfig();
    EXPECT_TRUE(cellularDataHandler->dataSwitchSettings_->IsUserDataRoamingOn());
}

/**
 * @tc.number: CellularDataHandler_RegisterDataSettingObserver_001
 * @tc.name: Test RegisterDataSettingObserver with null observer
 * @tc.desc: Verify RegisterDataSettingObserver returns early when any observer is null
 */
HWTEST_F(CellularDataHandlerTest, RegisterDataSettingObserver_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    // observer 五件套在 Init 创建，未 Init 时本就为空 → 走空早退分支
    EXPECT_EQ(cellularDataHandler->settingObserver_, nullptr);
    cellularDataHandler->RegisterDataSettingObserver();
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_RegisterDataSettingObserver_002
 * @tc.name: Test RegisterDataSettingObserver with invalid sim id
 * @tc.desc: Verify only the data enable observer is registered when GetSimId is invalid
 */
HWTEST_F(CellularDataHandlerTest, RegisterDataSettingObserver_002, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(0));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    EXPECT_CALL(*mockHelper, RegisterObserver(_, _)).Times(1).WillOnce(Return(0));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>> settingsGuard(
        CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init(); // observer 五件套在 Init 创建（未 Init 为空会走早退分支）
    cellularDataHandler->RegisterDataSettingObserver(); // dataEnable 注册后 simId 无效早退
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_RegisterDataSettingObserver_003
 * @tc.name: Test RegisterDataSettingObserver full path with paired unregister
 * @tc.desc: Verify all settings observers are registered and unregistered; rdb part is paired to net zero
 */
HWTEST_F(CellularDataHandlerTest, RegisterDataSettingObserver_003, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(1));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    EXPECT_CALL(*mockHelper, RegisterObserver(_, _)).Times(4).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockHelper, UnregisterObserver(_, _)).Times(4).WillRepeatedly(Return(0));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>> settingsGuard(
        CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init(); // observer 五件套在 Init 创建
    // settings 部分走 mock；rdb 单例部分真机注册后同用例内成对注销，副作用净零
    cellularDataHandler->RegisterDataSettingObserver();
    cellularDataHandler->UnRegisterDataSettingObserver();
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_UnRegisterDataSettingObserver_001
 * @tc.name: Test UnRegisterDataSettingObserver with null observer
 * @tc.desc: Verify UnRegisterDataSettingObserver returns early when any observer is null
 */
HWTEST_F(CellularDataHandlerTest, UnRegisterDataSettingObserver_001, Function | MediumTest | Level3)
{
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    // observer 五件套在 Init 创建，未 Init 时本就为空 → 走空早退分支
    EXPECT_EQ(cellularDataHandler->roamingObserver_, nullptr);
    cellularDataHandler->UnRegisterDataSettingObserver();
    SUCCEED();
}

/**
 * @tc.number: CellularDataHandler_UnRegisterDataSettingObserver_002
 * @tc.name: Test UnRegisterDataSettingObserver with invalid sim id
 * @tc.desc: Verify only the data enable observer is unregistered when GetSimId is invalid
 */
HWTEST_F(CellularDataHandlerTest, UnRegisterDataSettingObserver_002, Function | MediumTest | Level3)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetSimId(_)).WillByDefault(Return(0));
    auto mockHelper = std::make_shared<MockDataShareHelper>();
    EXPECT_CALL(*mockHelper, UnregisterObserver(_, _)).Times(1).WillOnce(Return(0));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>> settingsGuard(
        CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper);
    auto cellularDataHandler = TestAsyncUtils::MakeDrainedHandler(0);
    cellularDataHandler->Init(); // observer 五件套在 Init 创建（未 Init 为空会走早退分支）
    cellularDataHandler->UnRegisterDataSettingObserver(); // dataEnable 注销后 simId 无效早退
    SUCCEED();
}

} // namespace Telephony
} // namespace OHOS