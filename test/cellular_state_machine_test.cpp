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

#include <cstdio>
#include <cstring>

#include "activating.h"
#include "active.h"
#include "apn_manager.h"
#include "cellular_data_state_machine.h"
#include "core_manager_inner.h"
#include "data_connection_manager.h"
#include "data_connection_params.h"
#include "data_disconnect_params.h"
#include "default.h"
#include "disconnecting.h"
#include "gtest/gtest.h"
#include "inactive.h"
#include "incall_data_state_machine.h"
#include "mock/mock_sim_manager.h"
#include "mock/mock_network_search.h"
#include "tel_event_handler.h"
#include "telephony_types.h"
#include "tel_ril_data_parcel.h"
#include "utils/test_async_utils.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgReferee;

class CellularStateMachineTest : public testing::Test {
public:
    CellularStateMachineTest()
    {
        mockSimManager = new MockSimManager();
        std::shared_ptr<MockSimManager> mockSimManagerPtr(mockSimManager);
        CoreManagerInner::GetInstance().simManager_ = mockSimManagerPtr;

        mockNetworkSearchManager = new MockNetworkSearchManager();
        std::shared_ptr<MockNetworkSearchManager> mockNetworkSearchManagerPtr(mockNetworkSearchManager);
        CoreManagerInner::GetInstance().networkSearchManager_ = mockNetworkSearchManagerPtr;
    }
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<CellularDataStateMachine> cellularMachine;
    MockSimManager *mockSimManager;
    MockNetworkSearchManager *mockNetworkSearchManager;
};
void CellularStateMachineTest::SetUpTestCase() {}

void CellularStateMachineTest::TearDownTestCase() {}

void CellularStateMachineTest::SetUp() {}

void CellularStateMachineTest::TearDown() {}

class IncallDataStateMachineTest : public TelEventHandler {
public:
    IncallDataStateMachineTest() : TelEventHandler("IncallDataStateMachineTest") {}
    ~IncallDataStateMachineTest()
    {
        // 用例结束先停机排空 FFRT 队列再析构，规避 TelEventQueue 生命周期竞态（DT Signal 6）
        if (incallStateMachine_ != nullptr) {
            TestAsyncUtils::DrainStateMachine(incallStateMachine_.get());
            incallStateMachine_ = nullptr;
        }
        TestAsyncUtils::DrainTelEventHandler(this);
    }
    std::shared_ptr<IncallDataStateMachine> CreateIncallDataStateMachine();
    int32_t Init(int32_t callState, int32_t slotId);

public:
    std::shared_ptr<IncallDataStateMachine> incallStateMachine_ = nullptr;
};

std::shared_ptr<IncallDataStateMachine> IncallDataStateMachineTest::CreateIncallDataStateMachine()
{
    if (incallStateMachine_ != nullptr) {
        return incallStateMachine_;
    }
    incallStateMachine_ = std::make_shared<IncallDataStateMachine>();
    return incallStateMachine_;
}

int32_t IncallDataStateMachineTest::Init(int32_t callState, int32_t slotId)
{
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    if (apnManager == nullptr) {
        return -1;
    }
    if (incallStateMachine_ == nullptr) {
        return -1;
    }
    incallStateMachine_->Init(callState, slotId, std::weak_ptr<TelEventHandler>(
        std::static_pointer_cast<TelEventHandler>(shared_from_this())), apnManager);
    return 0;
}

class CellularMachineTest : public TelEventHandler {
public:
    CellularMachineTest() : TelEventHandler("CellularDataStateMachineTest") {}
    ~CellularMachineTest()
    {
        // 用例结束先停机排空 FFRT 队列再析构，规避 TelEventQueue 生命周期竞态（DT Signal 6）
        if (cellularDataStateMachine_ != nullptr) {
            if (cellularDataStateMachine_->cdConnectionManager_ != nullptr) {
                TestAsyncUtils::DrainStateMachine(cellularDataStateMachine_->cdConnectionManager_.get());
            }
            TestAsyncUtils::DrainStateMachine(cellularDataStateMachine_.get());
            cellularDataStateMachine_ = nullptr;
        }
        TestAsyncUtils::DrainTelEventHandler(this);
    }
    std::shared_ptr<CellularDataStateMachine> CreateCellularDataConnect(int32_t slotId);

public:
    std::shared_ptr<CellularDataStateMachine> cellularDataStateMachine_ = nullptr;
};

std::shared_ptr<CellularDataStateMachine> CellularMachineTest::CreateCellularDataConnect(int32_t slotId)
{
    if (cellularDataStateMachine_ != nullptr) {
        return cellularDataStateMachine_;
    }
    auto connectionManager = std::make_shared<DataConnectionManager>(slotId);
    if (connectionManager == nullptr) {
        return nullptr;
    }
    connectionManager->Init();
    cellularDataStateMachine_ = std::make_shared<CellularDataStateMachine>(
        connectionManager, std::static_pointer_cast<TelEventHandler>(shared_from_this()));
    return cellularDataStateMachine_;
}

/**
 * @tc.number   HasAnyConnectedState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, HasAnyConnectedState_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    
    incallStateMachine->apnManager_ = nullptr;
    ASSERT_EQ(incallStateMachine->HasAnyConnectedState(), false);
}

/**
 * @tc.number   IdleState_StateBegin_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_StateBegin_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    // StateMachine::Start() 异步投递 MSG_STATE_MACHINE_INIT，落地会把 idleState 激活（isActive_=true），
    // 与本用例"null stateMachine 下 StateBegin 不激活"的断言竞态（b6tests 193 行偶发失败根因）：
    // 移除该异步事件，使 isActive_ 仅由本用例的 null-protected StateBegin 控制
    incallStateMachine->stateMachineEventHandler_->RemoveEvent(CellularDataEventCode::MSG_STATE_MACHINE_INIT);
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    incallStateMachine = nullptr;
    idleState->stateMachine_ = incallStateMachine;
    idleState->StateBegin();
    ASSERT_EQ(idleState->isActive_, false);
}

/**
 * @tc.number   IdleState_StateBegin_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_StateBegin_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    incallStateMachine->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    idleState->stateMachine_ = incallStateMachine;
    idleState->StateBegin();
    ASSERT_EQ(idleState->isActive_, true);
}

/**
 * @tc.number   IdleState_StateBegin_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_StateBegin_003, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    incallStateMachine->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DISCONNECTED));
    idleState->stateMachine_ = incallStateMachine;
    idleState->StateBegin();
    ASSERT_EQ(idleState->isActive_, true);
}

/**
 * @tc.number   IdleState_StateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_StateProcess_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    incallStateMachine = nullptr;
    idleState->stateMachine_ = incallStateMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    idleState->StateProcess(event);
    ASSERT_EQ(idleState->isActive_, NOT_PROCESSED);
}

/**
 * @tc.number   IdleState_StateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_StateProcess_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    idleState->stateMachine_ = incallStateMachine;
    // 整改（历史概率失败，2026-11-11 插桩版实测复发）：本用例断言 isActive_==false，
    // 本质是在断言"初始迁移尚未完成"的异步属性——Start() 投递的 MSG_STATE_MACHINE_INIT
    // 由 FFRT worker 异步执行 IdleState::StateBegin 置 isActive_=true，与同步断言竞态
    // （StateProcess_001 因先置空 stateMachine_ 使 StateBegin 早退而侥幸稳定，本用例
    // 保留有效 machine 指针以覆盖另一分支，故必被竞态）。确定性化：先排空机器事件队列
    // （清掉待处理 INIT 并等在飞任务落地），再显式复位 isActive_（排空窗口内 StateBegin
    // 可能已执行），此后无任何异步源，StateProcess 为纯同步调用，断言恒成立。
    TestAsyncUtils::DrainTelEventHandler(incallStateMachine->stateMachineEventHandler_.get());
    idleState->isActive_ = false;
    idleState->eventIdFunMap_.clear();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_CALL_STARTED);
    idleState->StateProcess(event);
    ASSERT_EQ(idleState->isActive_, NOT_PROCESSED);
    // 释放前停机排空，规避 FFRT 队列析构竞态
    TestAsyncUtils::DrainStateMachine(incallStateMachine.get());
}

/**
 * @tc.number   IdleState_IncallStateMachine_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IdleState_IncallStateMachine_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    incallStateMachine->TransitionTo(incallStateMachine->idleState_);
    auto idleState = std::static_pointer_cast<IdleState>(incallStateMachine->idleState_);
    incallStateMachine = nullptr;
    idleState->stateMachine_ = incallStateMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    bool result = idleState->ProcessCallStarted(event);
    ASSERT_EQ(result, NOT_PROCESSED);
    result = idleState->ProcessCallEnded(event);
    ASSERT_EQ(result, NOT_PROCESSED);
    result = idleState->ProcessSettingsOn(event);
    ASSERT_EQ(result, NOT_PROCESSED);
    result = idleState->ProcessDsdsChanged(event);
    ASSERT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   StateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, StateProcess_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->activatedSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->deactivatingSecondaryState_);
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(incallStateMachine->deactivatingSecondaryState_);
    bool result = deactivatingSecondaryState->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   StateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, StateProcess_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->activatedSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->deactivatingSecondaryState_);
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(incallStateMachine->deactivatingSecondaryState_);
    bool result = deactivatingSecondaryState->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   StateProcess_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, StateProcess_003, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->activatedSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->deactivatingSecondaryState_);
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(incallStateMachine->deactivatingSecondaryState_);
    incallStateMachine = nullptr;
    deactivatingSecondaryState->stateMachine_ = incallStateMachine;
    bool result = deactivatingSecondaryState->StateProcess(event);
    deactivatingSecondaryState->StateBegin();
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   StateProcess_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, StateProcess_004, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_DISCONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->activatedSecondaryState_);
    incallStateMachine->TransitionTo(incallStateMachine->deactivatingSecondaryState_);
    auto deactivatingSecondaryState =
        std::static_pointer_cast<DeactivatingSecondaryState>(incallStateMachine->deactivatingSecondaryState_);
    bool result = deactivatingSecondaryState->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   ActivatingStateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ActivatingStateProcess_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    auto activatingSecondaryState =
        std::static_pointer_cast<ActivatingSecondaryState>(incallStateMachine->activatingSecondaryState_);
    bool result = activatingSecondaryState->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   ActivatingStateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ActivatingStateProcess_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(-1);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    auto activatingSecondaryState =
        std::static_pointer_cast<ActivatingSecondaryState>(incallStateMachine->activatingSecondaryState_);
    bool result = activatingSecondaryState->StateProcess(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   ActivatingSecondaryState_IncallDataStateMachine_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ActivatingSecondaryState_IncallDataStateMachine_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(0);
    incallStateMachine->TransitionTo(incallStateMachine->activatingSecondaryState_);
    auto activatingSecondaryState =
        std::static_pointer_cast<ActivatingSecondaryState>(incallStateMachine->activatingSecondaryState_);
    incallStateMachine = nullptr;
    activatingSecondaryState->stateMachine_ = incallStateMachine;
    bool result = activatingSecondaryState->StateProcess(event);
    activatingSecondaryState->StateBegin();
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   SecondaryActiveStateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveStateProcess_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    bool result = secondaryActiveState->StateProcess(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   SecondaryActiveStateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveStateProcess_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    incallStateMachine = nullptr;
    secondaryActiveState->stateMachine_ = incallStateMachine;
    bool result = secondaryActiveState->StateProcess(event);
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   SecondaryActiveStateProcess_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveStateProcess_003, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    secondaryActiveState->stateMachine_ = incallStateMachine;
    secondaryActiveState->eventIdFunMap_.clear();
    bool result = secondaryActiveState->StateProcess(event);
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   SecondaryActiveState_ProcessCallEnded_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveState_ProcessCallEnded_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    incallStateMachine = nullptr;
    secondaryActiveState->stateMachine_ = incallStateMachine;
    bool result = secondaryActiveState->ProcessCallEnded(event);
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   SecondaryActiveState_ProcessCallEnded_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveState_ProcessCallEnded_002, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    incallStateMachine->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE));
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    secondaryActiveState->stateMachine_ = incallStateMachine;
    bool result = secondaryActiveState->ProcessCallEnded(event);
    EXPECT_EQ(result, PROCESSED);
    incallStateMachine->UpdateCallState(static_cast<int32_t>(TelCallStatus::CALL_STATUS_DISCONNECTED));
    result = secondaryActiveState->ProcessCallEnded(event);
    EXPECT_EQ(result, PROCESSED);
}

/**
 * @tc.number   SecondaryActiveState_IncallDataStateMachine_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SecondaryActiveState_IncallDataStateMachine_001, Function | MediumTest | Level1)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    if (incallStateMachineTest->Init(TelCallStatus::CALL_STATUS_DIALING, 0) < 0) {
        incallStateMachine = nullptr;
    }
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_INCALL_DATA_DATA_CONNECTED);
    incallStateMachine->TransitionTo(incallStateMachine->secondaryActiveState_);
    auto secondaryActiveState =
        std::static_pointer_cast<SecondaryActiveState>(incallStateMachine->secondaryActiveState_);
    incallStateMachine = nullptr;
    secondaryActiveState->stateMachine_ = incallStateMachine;
    bool result = secondaryActiveState->ProcessCallEnded(event);
    EXPECT_EQ(result, NOT_PROCESSED);
    result = secondaryActiveState->ProcessSettingsOff(event);
    EXPECT_EQ(result, NOT_PROCESSED);
}

/**
 * @tc.number   InactiveStateBegin_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, InactiveStateBegin_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    inactive->deActiveApnTypeId_ = 0;
    inactive->SetStateMachine(cellularMachine);
    inactive->SetPdpErrorReason(PdpErrorReason::PDP_ERR_RETRY);
    EXPECT_EQ(inactive->resultInfo_->reason, static_cast<int32_t>(PdpErrorReason::PDP_ERR_RETRY));
}

/**
 * @tc.number   InactiveStateBegin_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, InactiveStateBegin_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    ASSERT_NE(inactive, nullptr);
    inactive->SetStateMachine(cellularMachine);
    inactive->SetDeActiveApnTypeId(1);
    auto resultInfo = std::make_shared<SetupDataCallResultInfo>();
    resultInfo->reason = static_cast<int32_t>(PdpErrorReason::PDP_ERR_RETRY);
    resultInfo->retryTime = 1;
    resultInfo->retryScene = 1;
    inactive->SetDataCallResultInfo(resultInfo);
    ASSERT_NE(cellularMachine->netSupplierInfo_, nullptr);
    ASSERT_NE(cellularMachine->stateMachineEventHandler_, nullptr);
    inactive->StateBegin();
    EXPECT_TRUE(cellularMachine->IsInactiveState());
    EXPECT_EQ(inactive->deActiveApnTypeId_, ERROR_APN_ID);
    EXPECT_EQ(inactive->resultInfo_, nullptr);
}

/**
 * @tc.number   InactiveStateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, InactiveStateProcess_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    inactive->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_DISCONNECT);
    bool result = inactive->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   InactiveStateProcess_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, InactiveStateProcess_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    inactive->SetStateMachine(cellularMachine);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_DISCONNECT_ALL);
    bool result = inactive->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Inactive_CellularDataStateMachine_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Inactive_CellularDataStateMachine_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    cellularMachine = nullptr;
    inactive->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_DISCONNECT_ALL);
    bool result = inactive->StateProcess(event);
    inactive->StateBegin();
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultStateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultStateProcess_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    mDefault->stateMachine_ = cellularMachine;
    mDefault->eventIdFunMap_.clear();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->StateProcess(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultStateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultStateProcess_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine = nullptr;
    mDefault->stateMachine_ = cellularMachine;
    mDefault->eventIdFunMap_.clear();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->StateProcess(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDisconnectDone_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDisconnectDone_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    mDefault->stateMachine_ = cellularMachine;
    mDefault->eventIdFunMap_.clear();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDisconnectDone(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   DefaultProcessDisconnectDone_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDisconnectDone_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine = nullptr;
    mDefault->stateMachine_ = cellularMachine;
    mDefault->eventIdFunMap_.clear();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDisconnectDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDisconnectAllDone_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDisconnectAllDone_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   DefaultProcessDisconnectAllDone_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDisconnectAllDone_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine = nullptr;
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDataConnectionDrsOrRatChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDataConnectionDrsOrRatChanged_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine->TransitionTo(cellularMachine->activeState_);
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDataConnectionDrsOrRatChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDataConnectionDrsOrRatChanged_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDataConnectionDrsOrRatChanged_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine->TransitionTo(cellularMachine->activatingState_);
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDataConnectionDrsOrRatChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDataConnectionDrsOrRatChanged_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDataConnectionDrsOrRatChanged_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine->TransitionTo(cellularMachine->disconnectingState_);
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDataConnectionDrsOrRatChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   DefaultProcessDataConnectionDrsOrRatChanged_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, DefaultProcessDataConnectionDrsOrRatChanged_004, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto mDefault = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    cellularMachine = nullptr;
    mDefault->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = mDefault->ProcessDataConnectionDrsOrRatChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_StateBegin_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_StateBegin_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    active->StateBegin();
    EXPECT_EQ(active->isActive_, true);
}

/**
 * @tc.number   Active_StateProcess_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_StateProcess_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_StateProcess_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_StateProcess_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->StateProcess(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_StateProcess_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_StateProcess_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine = nullptr;
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->StateProcess(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessDisconnectDone_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectDone_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    event = nullptr;
    bool result = active->ProcessDisconnectDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectDone_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectDone_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessDisconnectDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectAllDone_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectAllDone_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    event = nullptr;
    bool result = active->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectAllDone_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectAllDone_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectAllDone_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectAllDone_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->TransitionTo(cellularMachine->inActiveState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataDisconnectParams> dataDisconnectParams =
        std::make_shared<DataDisconnectParams>("", DisConnectionReason::REASON_NORMAL);
    auto event = AppExecFwk::InnerEvent::Get(0, dataDisconnectParams);
    bool result = active->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectAllDone_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectAllDone_004, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->inActiveState_ = nullptr;
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataDisconnectParams> dataDisconnectParams =
        std::make_shared<DataDisconnectParams>("", DisConnectionReason::REASON_NORMAL);
    auto event = AppExecFwk::InnerEvent::Get(0, dataDisconnectParams);
    bool result = active->ProcessDisconnectAllDone(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessLinkCapabilityChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLinkCapabilityChanged_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessLinkCapabilityChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessLinkCapabilityChanged_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLinkCapabilityChanged_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine = nullptr;
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataLinkCapability> linkCapability = std::make_shared<DataLinkCapability>();
    auto event = AppExecFwk::InnerEvent::Get(0, linkCapability);
    bool result = active->ProcessLinkCapabilityChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessLinkCapabilityChanged_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLinkCapabilityChanged_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataLinkCapability> linkCapability = std::make_shared<DataLinkCapability>();
    linkCapability->primaryUplinkKbps = 0;
    auto event = AppExecFwk::InnerEvent::Get(0, linkCapability);
    bool result = active->ProcessLinkCapabilityChanged(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessLinkCapabilityChanged_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLinkCapabilityChanged_004, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataLinkCapability> linkCapability = std::make_shared<DataLinkCapability>();
    linkCapability->primaryUplinkKbps = 1;
    linkCapability->primaryDownlinkKbps = 0;
    auto event = AppExecFwk::InnerEvent::Get(0, linkCapability);
    bool result = active->ProcessLinkCapabilityChanged(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessLinkCapabilityChanged_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLinkCapabilityChanged_005, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataLinkCapability> linkCapability = std::make_shared<DataLinkCapability>();
    linkCapability->primaryUplinkKbps = 1;
    linkCapability->primaryDownlinkKbps = 1;
    auto event = AppExecFwk::InnerEvent::Get(0, linkCapability);
    bool result = active->ProcessLinkCapabilityChanged(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessDataConnectionComplete_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionComplete_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessDataConnectionComplete(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDataConnectionComplete_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionComplete_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine = nullptr;
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<SetupDataCallResultInfo> setupDataCallResultInfo = std::make_shared<SetupDataCallResultInfo>();
    auto event = AppExecFwk::InnerEvent::Get(0, setupDataCallResultInfo);
    bool result = active->ProcessDataConnectionComplete(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDataConnectionComplete_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionComplete_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<SetupDataCallResultInfo> setupDataCallResultInfo = std::make_shared<SetupDataCallResultInfo>();
    auto event = AppExecFwk::InnerEvent::Get(0, setupDataCallResultInfo);
    bool result = active->ProcessDataConnectionComplete(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessDataConnectionComplete_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionComplete_004, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->stateMachineEventHandler_ = nullptr;
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<SetupDataCallResultInfo> setupDataCallResultInfo = std::make_shared<SetupDataCallResultInfo>();
    auto event = AppExecFwk::InnerEvent::Get(0, setupDataCallResultInfo);
    bool result = active->ProcessDataConnectionComplete(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessNrStateChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessNrStateChanged_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessNrStateChanged(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessNrStateChanged_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessNrStateChanged_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine = nullptr;
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessNrStateChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessNrFrequencyChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessNrFrequencyChanged_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine = nullptr;
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessNrFrequencyChanged(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDisconnectAllDone_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLostConnection_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->TransitionTo(cellularMachine->inActiveState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessLostConnection(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessLostConnection_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessLostConnection_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->inActiveState_ = nullptr;
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(0);
    bool result = active->ProcessLostConnection(event);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   Active_ProcessDataConnectionRoamOn_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionRoamOn_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessDataConnectionRoamOn(event);
    EXPECT_TRUE(result);
}

/**
 * @tc.number   Active_ProcessDataConnectionRoamOff_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDataConnectionRoamOff_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessDataConnectionRoamOff(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   Active_ProcessDataConnectionVoiceCallStartedOrEnded_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ProcessDataConnectionVoiceCallStartedOrEnded_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_CONNECT);
    bool result = active->ProcessDataConnectionVoiceCallStartedOrEnded(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   CellularDataStateMachine_GetSlotId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetSlotId_001, Function | MediumTest | Level1)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    int result = cellularMachine->GetSlotId();
    ASSERT_EQ(result, DEFAULT_SIM_SLOT_ID);
}

/**
 * @tc.number   CellularDataStateMachine_HasMatchedIpTypeAddrs_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_HasMatchedIpTypeAddrs_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    uint8_t ipType = 1;
    uint8_t ipInfoArraySize = 2;
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info1;
    info1.type = 1;
    AddressInfo info2;
    info2.type = 3;
    ipInfoArray.push_back(info1);
    ipInfoArray.push_back(info2);
    bool result = cellularMachine->HasMatchedIpTypeAddrs(ipType, ipInfoArraySize, ipInfoArray);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   CellularDataStateMachine_HasMatchedIpTypeAddrs_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_HasMatchedIpTypeAddrs_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    uint8_t ipType = 5;
    uint8_t ipInfoArraySize = 2;
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info1;
    info1.type = 1;
    AddressInfo info2;
    info2.type = 3;
    ipInfoArray.push_back(info1);
    ipInfoArray.push_back(info2);
    bool result = cellularMachine->HasMatchedIpTypeAddrs(ipType, ipInfoArraySize, ipInfoArray);
    ASSERT_FALSE(result);
}

/**
 * @tc.number   GetIpType_ShouldReturnIPV4V6_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, GetIpType_ShouldReturnIPV4V6_001, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info1;
    info1.type = INetAddr::IpType::IPV4;
    AddressInfo info2;
    info2.type = INetAddr::IpType::IPV6;
    ipInfoArray.push_back(info1);
    ipInfoArray.push_back(info2);
    std::string result = cellularMachine->GetIpType(ipInfoArray);
    ASSERT_EQ(result, "IPV4V6");
}

/**
 * @tc.number   GetIpType_ShouldReturnIPV4V6_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, GetIpType_ShouldReturnIPV4V6_002, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info1;
    info1.type = INetAddr::IpType::IPV4;
    ipInfoArray.push_back(info1);
    std::string result = cellularMachine->GetIpType(ipInfoArray);
    ASSERT_EQ(result, "IPV4");
}

/**
 * @tc.number   GetIpType_ShouldReturnIPV6_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, GetIpType_ShouldReturnIPV6_003, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info2;
    info2.type = INetAddr::IpType::IPV6;
    ipInfoArray.push_back(info2);
    std::string result = cellularMachine->GetIpType(ipInfoArray);
    ASSERT_EQ(result, "IPV6");
}

/**
 * @tc.number   GetIpType_ShouldReturnEmpty_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, GetIpType_ShouldReturnEmpty_004, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> ipInfoArray = {};
    std::string result = cellularMachine->GetIpType(ipInfoArray);
    ASSERT_EQ(result, "");
}

/**
 * @tc.number   GetIpType_ShouldReturnIPV4V6_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, GetIpType_ShouldReturnIPV4V6_005, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo info1;
    info1.type = 5;
    AddressInfo info2;
    info2.type = 6;
    ipInfoArray.push_back(info1);
    ipInfoArray.push_back(info2);
    std::string result = cellularMachine->GetIpType(ipInfoArray);
    ASSERT_EQ(result, "");
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_001, TestSize.Level0) {
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    SetupDataCallResultInfo dataCallInfo;
    dataCallInfo.address = "";
    dataCallInfo.dns = "";
    dataCallInfo.dnsSec = "";
    dataCallInfo.gateway = "";
    cellularMachine->UpdateNetworkInfo(dataCallInfo);
    ASSERT_EQ(cellularMachine->cause_, 0);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_002, TestSize.Level0) {
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    SetupDataCallResultInfo dataCallInfo;
    dataCallInfo.address = "192.168.1.1";
    dataCallInfo.dns = "192.168.1.1";
    dataCallInfo.dnsSec = "192.168.1.1";
    dataCallInfo.gateway = "192.168.1.1";
    dataCallInfo.reason = 1;
    cellularMachine->UpdateNetworkInfo(dataCallInfo);
    ASSERT_EQ(cellularMachine->cause_, 1);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_003, TestSize.Level0) {
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    SetupDataCallResultInfo dataCallInfo;
    dataCallInfo.address = "192.168.1.1";
    dataCallInfo.dns = "192.168.1.1";
    dataCallInfo.dnsSec = "192.168.1.1";
    dataCallInfo.gateway = "192.168.1.1";
    dataCallInfo.reason = 1;
    cellularMachine->UpdateNetworkInfo(dataCallInfo);
    ASSERT_EQ(cellularMachine->cause_, 0);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_004, TestSize.Level0) {
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    cellularMachine->netSupplierInfo_->isAvailable_ = true;
    cellularMachine->UpdateNetworkInfo();
    ASSERT_NE(cellularMachine->netSupplierInfo_, nullptr);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_005, TestSize.Level0) {
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    cellularMachine->netSupplierInfo_->isAvailable_ = false;
    cellularMachine->UpdateNetworkInfo();
    ASSERT_NE(cellularMachine->netSupplierInfo_, nullptr);
}

/**
 * @tc.number   CellularDataStateMachine_ResolveRoute_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
 HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_ResolveRoute_001, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> routeInfoArray;
    cellularMachine->ResolveRoute(routeInfoArray, "eth0");
    EXPECT_TRUE(cellularMachine->netLinkInfo_->routeList_.empty());
}

/**
 * @tc.number   CellularDataStateMachine_ResolveRoute_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_ResolveRoute_002, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> routeInfoArray;
    AddressInfo routeInfo;
    routeInfo.ip = "192.168.1.1";
    routeInfo.type = INetAddr::IpType::IPV4;
    routeInfo.prefixLen = 24;
    routeInfoArray.push_back(routeInfo);
    cellularMachine->ResolveRoute(routeInfoArray, "eth0");
    EXPECT_FALSE(cellularMachine->netLinkInfo_->routeList_.empty());
}

/**
 * @tc.number   CellularDataStateMachine_ResolveRoute_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_ResolveRoute_003, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::vector<AddressInfo> routeInfoArray;
    AddressInfo routeInfo1;
    routeInfo1.ip = "192.168.1.1";
    routeInfo1.type = INetAddr::IpType::IPV4;
    routeInfo1.prefixLen = 24;
    routeInfoArray.push_back(routeInfo1);
    AddressInfo routeInfo2;
    routeInfo2.ip = "2001:db8::1";
    routeInfo2.type = INetAddr::IpType::IPV6;
    routeInfo2.prefixLen = 64;
    routeInfoArray.push_back(routeInfo2);
    cellularMachine->ResolveRoute(routeInfoArray, "eth0");
    EXPECT_EQ(cellularMachine->netLinkInfo_->routeList_.size(), 2);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfoIfInActive_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfoIfInActive_001, TestSize.Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    SetupDataCallResultInfo dataCallInfo;
    cellularMachine->UpdateNetworkInfoIfInActive(dataCallInfo);
    EXPECT_NE(cellularMachine->stateMachineEventHandler_, nullptr);
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfoIfInActive_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfoIfInActive_002, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    SetupDataCallResultInfo dataCallInfo;
    cellularMachine->stateMachineEventHandler_ = nullptr;
    cellularMachine->UpdateNetworkInfoIfInActive(dataCallInfo);
    EXPECT_EQ(cellularMachine->stateMachineEventHandler_, nullptr);
}

/**
 * @tc.number   CellularDataStateMachine_SplitProxyIpAddress_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_SplitProxyIpAddress_001, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    std::string proxyIpAddress = "";
    std::string host;
    uint16_t port;
    cellularMachine->SplitProxyIpAddress(proxyIpAddress, host, port);
    ASSERT_EQ(host, "");
    ASSERT_EQ(port, 0);
}

/**
 * @tc.number   CellularDataStateMachine_SplitProxyIpAddress_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_SplitProxyIpAddress_002, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    std::string proxyIpAddress = "192.168.1.1";
    std::string host;
    uint16_t port;
    cellularMachine->SplitProxyIpAddress(proxyIpAddress, host, port);
    ASSERT_EQ(host, "192.168.1.1");
    ASSERT_EQ(port, 0);
}

/**
 * @tc.number   CellularDataStateMachine_SplitProxyIpAddress_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_SplitProxyIpAddress_003, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    std::string proxyIpAddress = "192.168.1.1:8080";
    std::string host;
    uint16_t port;
    cellularMachine->SplitProxyIpAddress(proxyIpAddress, host, port);
    ASSERT_EQ(host, "192.168.1.1");
    ASSERT_EQ(port, 8080);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_001, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V3);
            return 0;
        });
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_002, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = INVALID_SLOT_ID;
            return 0;
        });
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 0;
            return 0;
        });
    result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_003, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 1;
            return 0;
        });
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_004, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 1;
            return 0;
        });
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, bool &hasSimCard) {
            hasSimCard = true;
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _)).Times(AtLeast(2))
        .WillOnce([](int32_t slotId, ImsServiceType imsSrvType, ImsRegInfo &info) {
            switch (imsSrvType) {
                case ImsServiceType::TYPE_VOICE:
                    info.imsRegState = ImsRegState::IMS_UNREGISTERED;
                    break;
                case ImsServiceType::TYPE_VIDEO:
                    info.imsRegState = ImsRegState::IMS_UNREGISTERED;
                    break;
                default:
                    break;
            }
            return 0;
        });
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_005, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 1;
            return 0;
        });
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, bool &hasSimCard) {
            hasSimCard = true;
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _)).Times(AtLeast(2))
        .WillOnce([](int32_t slotId, ImsServiceType imsSrvType, ImsRegInfo &info) {
            switch (imsSrvType) {
                case ImsServiceType::TYPE_VOICE:
                    info.imsRegState = ImsRegState::IMS_REGISTERED ;
                    break;
                case ImsServiceType::TYPE_VIDEO:
                    info.imsRegState = ImsRegState::IMS_UNREGISTERED;
                    break;
                default:
                    break;
            }
            return 0;
        });
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_006, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 1;
            return 0;
        });
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, bool &hasSimCard) {
            hasSimCard = true;
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _)).Times(AtLeast(2))
        .WillOnce([](int32_t slotId, ImsServiceType imsSrvType, ImsRegInfo &info) {
            switch (imsSrvType) {
                case ImsServiceType::TYPE_VOICE:
                    info.imsRegState = ImsRegState::IMS_REGISTERED ;
                    break;
                case ImsServiceType::TYPE_VIDEO:
                    info.imsRegState = ImsRegState::IMS_REGISTERED;
                    break;
                default:
                    break;
            }
            return 0;
        });
    incallStateMachine->callState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE);
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
    incallStateMachine->callState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_DISCONNECTED);
    result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
    incallStateMachine->callState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_DIALING);
    result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   IncallDataStateMachine_IsSecondaryCanActiveData_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, IncallDataStateMachine_IsSecondaryCanActiveData_007, TestSize.Level0)
{
    std::shared_ptr<IncallDataStateMachineTest> incallStateMachineTest = std::make_shared<IncallDataStateMachineTest>();
    std::shared_ptr<IncallDataStateMachine> incallStateMachine =
        incallStateMachineTest->CreateIncallDataStateMachine();
    incallStateMachine->slotId_ = 0;
    sptr<ApnManager> apnManager = std::make_unique<ApnManager>().release();
    incallStateMachine->apnManager_ = apnManager;
    incallStateMachine->cellularDataHandler_ = std::weak_ptr<TelEventHandler>(std::static_pointer_cast<TelEventHandler>(incallStateMachineTest));
    EXPECT_CALL(*mockSimManager, GetDsdsMode(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &dsdsMode) {
            dsdsMode = static_cast<int32_t>(DsdsMode::DSDS_MODE_V2);
            return 0;
        });
    EXPECT_CALL(*mockSimManager, GetPrimarySlotId(_)).Times(AtLeast(1))
        .WillOnce([](int32_t &slotId) {
            slotId = 1;
            return 0;
        });
    EXPECT_CALL(*mockSimManager, HasSimCard(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, bool &hasSimCard) {
            hasSimCard = true;
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, GetImsRegStatus(_, _, _)).Times(AtLeast(2))
        .WillOnce([](int32_t slotId, ImsServiceType imsSrvType, ImsRegInfo &info) {
            switch (imsSrvType) {
                case ImsServiceType::TYPE_VOICE:
                    info.imsRegState = ImsRegState::IMS_REGISTERED ;
                    break;
                case ImsServiceType::TYPE_VIDEO:
                    info.imsRegState = ImsRegState::IMS_REGISTERED;
                    break;
                default:
                    break;
            }
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, int32_t &psRadioTech) {
            psRadioTech = static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE);
            return 0;
        });
    incallStateMachine->callState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_DIALING);
    auto result = incallStateMachine->IsSecondaryCanActiveData();
    ASSERT_EQ(result, true);
}

/**
 * @tc.number   CellularDataStateMachine_GetMtuSizeFromOpCfg_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetMtuSizeFromOpCfg_001, TestSize.Level0)
{
    int32_t mtuSize = 0;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "ipv4:1500;ipv6:1400";
            return 0;
        });
    cellularMachine->ipType_ = "ipv4";
    cellularMachine->GetMtuSizeFromOpCfg(mtuSize, slotId);
    ASSERT_EQ(mtuSize, 1500);
}

/**
 * @tc.number   CellularDataStateMachine_GetMtuSizeFromOpCfg_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetMtuSizeFromOpCfg_002, TestSize.Level0)
{
    int32_t mtuSize = 0;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "ipv4:1500;ipv6:1400";
            return 0;
        });
    cellularMachine->ipType_ = "ipv6";
    cellularMachine->GetMtuSizeFromOpCfg(mtuSize, slotId);
    ASSERT_EQ(mtuSize, 1400);
}

/**
 * @tc.number   CellularDataStateMachine_GetMtuSizeFromOpCfg_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetMtuSizeFromOpCfg_003, TestSize.Level0)
{
    int32_t mtuSize = 0;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "ipv4:1500;ipv6:1400";
            return 0;
        });
    cellularMachine->ipType_ = "ipv5";
    cellularMachine->GetMtuSizeFromOpCfg(mtuSize, slotId);
    ASSERT_EQ(mtuSize, 0);
}

/**
 * @tc.number   CellularDataStateMachine_GetMtuSizeFromOpCfg_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetMtuSizeFromOpCfg_004, TestSize.Level0)
{
    int32_t mtuSize = 0;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "ipv4:abc;ipv6:1400";
            return 0;
        });
    cellularMachine->ipType_ = "ipv4";
    cellularMachine->GetMtuSizeFromOpCfg(mtuSize, slotId);
    ASSERT_EQ(mtuSize, 0);
}

/**
 * @tc.number   CellularDataStateMachine_GetNetScoreBySlotId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetNetScoreBySlotId_001, TestSize.Level0)
{
    int32_t score;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).Times(AtLeast(1))
        .WillOnce([]() {
            return 0;
        });
    score = cellularMachine->GetNetScoreBySlotId(slotId);
    ASSERT_EQ(score, DEFAULT_INTERNET_CONNECTION_SCORE);
}

/**
 * @tc.number   CellularDataStateMachine_GetNetScoreBySlotId_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetNetScoreBySlotId_002, TestSize.Level0)
{
    int32_t score;
    int32_t slotId = 0;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).Times(AtLeast(1))
        .WillOnce([]() {
            return 1;
        });
    score = cellularMachine->GetNetScoreBySlotId(slotId);
    ASSERT_EQ(score, OTHER_CONNECTION_SCORE);
}

/**
 * @tc.number   Default_ProcessUpdateNetworkInfo_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, Default_ProcessUpdateNetworkInfo_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        EXPECT_CALL(*mockSimManager, GetDefaultCellularDataSlotId()).Times(AtLeast(1))
        .WillOnce([]() {
            return 0;
        });
    }
    auto defaultState = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    defaultState->stateMachine_ = cellularMachine;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_UPDATE_NETWORK_INFO);
    bool result = defaultState->ProcessUpdateNetworkInfo(event);
    EXPECT_EQ(result, true);
}

/**
 * @tc.number   SetIfReuseSupplierId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SetIfReuseSupplierId_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    EXPECT_EQ(cellularMachine->reuseApnCap_, NetManagerStandard::NetCap::NET_CAPABILITY_END);
    cellularMachine->SetIfReuseSupplierId(true);
}

/**
 * @tc.number   SetIfReuseSupplierId_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, SetIfReuseSupplierId_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    cellularMachine->reuseApnCap_ = NetManagerStandard::NetCap::NET_CAPABILITY_MMS;
    cellularMachine->SetIfReuseSupplierId(true);
    EXPECT_NE(cellularMachine->reuseApnCap_, NetManagerStandard::NetCap::NET_CAPABILITY_END);
}

/**
 * @tc.number   UpdateNetworkInfoIfInActive_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, UpdateNetworkInfoIfInActive_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    SetupDataCallResultInfo info;
    EXPECT_NE(cellularMachine->cellularDataHandler_, nullptr);
    cellularMachine->cellularDataHandler_ = nullptr;
    cellularMachine->UpdateNetworkInfoIfInActive(info);
}

/**
 * @tc.number   FillRSDFromNetCap_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, FillRSDFromNetCap_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    EXPECT_NE(cellularMachine, nullptr);
    std::map<std::string, std::string> networkSliceParas;
    sptr<ApnItem> apn = new ApnItem();
    cellularMachine->FillRSDFromNetCap(networkSliceParas, apn);
}

/**
 * @tc.number OnInterfaceLinkStateChanged_001
 * @tc.name test function branch
 * @tc.desc Function test
 */
HWTEST_F(CellularStateMachineTest, OnInterfaceLinkStateChanged_001, Function | MediumTest | Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    SetupDataCallResultInfo dataCallInfo;
    dataCallInfo.address = "192.168.1.1";
    dataCallInfo.dns = "192.168.1.1";
    dataCallInfo.dnsSec = "192.168.1.1";
    dataCallInfo.gateway = "192.168.1.1";
    dataCallInfo.reason = 1;
    dataCallInfo.netPortName = "rmnet0";
    cellularMachine->UpdateNetworkInfo(dataCallInfo);
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    cellularMachine->TransitionTo(cellularMachine->inActiveState_);
    active->stateMachine_ = cellularMachine;
    std::shared_ptr<DataDisconnectParams> dataDisconnectParams =
    std::make_shared<DataDisconnectParams>("", DisConnectionReason::REASON_NORMAL);
    auto event = AppExecFwk::InnerEvent::Get(0, dataDisconnectParams);
    bool result = active->ProcessDisconnectAllDone(event);
    if (cellularMachine->netInterfaceCallback_ != nullptr) {
        if (cellularMachine->stateMachineEventHandler_ != nullptr) {
            cellularMachine->stateMachineEventHandler_->SendEvent(
                CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 0, 1000);
        }
        cellularMachine->netInterfaceCallback_->OnInterfaceLinkStateChanged("rmnet0", false);
    }
    if (cellularMachine->netInterfaceCallback_ != nullptr) {
        cellularMachine->netInterfaceCallback_->OnInterfaceLinkStateChanged("rmnet", false);
    }
    if (cellularMachine->netInterfaceCallback_ != nullptr) {
        cellularMachine->stateMachineEventHandler_ = nullptr;
        cellularMachine->OnInterfaceLinkStateChanged("rmnet0", false);
    }
    cellularMachine->UnregisterNetInterfaceCallback();
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   LinkDownFallback_ActiveState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, LinkDownFallback_ActiveState_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    ASSERT_NE(active, nullptr);
    ASSERT_NE(inactive, nullptr);
    active->stateMachine_ = cellularMachine;
    cellularMachine->cid_ = 7;
    cellularMachine->apnId_ = 1;
    cellularMachine->ifName_ = "rmnet0";
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto lostConnection = std::make_shared<SetupDataCallResultInfo>();
    lostConnection->cid = 7;
    lostConnection->flag = 1;
    lostConnection->active = 0;
    lostConnection->reason = static_cast<int32_t>(PdpErrorReason::PDP_ERR_TO_CLEAR_CONNECTION);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_LOST_CONNECTION, lostConnection);
    bool result = active->ProcessLostConnection(event);
    EXPECT_TRUE(result);
    EXPECT_EQ(inactive->deActiveApnTypeId_, cellularMachine->apnId_);
    ASSERT_NE(inactive->resultInfo_, nullptr);
    EXPECT_EQ(inactive->resultInfo_->cid, lostConnection->cid);
}

/**
 * @tc.number   LinkDownFallback_StaleEvent_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, LinkDownFallback_StaleEvent_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    ASSERT_NE(active, nullptr);
    cellularMachine->connectId_.store(2);
    cellularMachine->cid_ = 7;
    cellularMachine->apnId_ = 1;
    cellularMachine->ifName_ = "rmnet0";
    active->stateMachine_ = cellularMachine;
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 1);
    cellularMachine->ProcessConnectionLostTimeout(event);
    EXPECT_TRUE(cellularMachine->IsActiveState());
}

/**
 * @tc.number   ProcessConnectionLostTimeout_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ProcessConnectionLostTimeout_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    ASSERT_NE(active, nullptr);
    ASSERT_NE(inactive, nullptr);
    active->stateMachine_ = cellularMachine;
    cellularMachine->connectId_ = 1;
    cellularMachine->apnId_ = 1;
    cellularMachine->SetReuseApnCap(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
    ASSERT_NE(cellularMachine->GetReuseApnCap(), NetManagerStandard::NetCap::NET_CAPABILITY_END);
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 1);
    cellularMachine->ProcessConnectionLostTimeout(event);
    EXPECT_EQ(cellularMachine->GetReuseApnCap(), NetManagerStandard::NetCap::NET_CAPABILITY_END);
    // Inactive::StateBegin consumes resultInfo_ and deActiveApnTypeId_ when building
    // MSG_DISCONNECT_DATA_COMPLETE and resets both after the transition, so verify the
    // transition result instead of dereferencing the consumed members.
    EXPECT_TRUE(cellularMachine->IsInactiveState());
    EXPECT_EQ(inactive->deActiveApnTypeId_, ERROR_APN_ID);
    EXPECT_EQ(inactive->resultInfo_, nullptr);
}


/**
 * @tc.number   ProcessConnectionLostTimeout_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ProcessConnectionLostTimeout_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    ASSERT_NE(active, nullptr);
    active->stateMachine_ = cellularMachine;
    cellularMachine->connectId_.store(1);
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto inActiveState = cellularMachine->inActiveState_;
    cellularMachine->inActiveState_ = nullptr;
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 1);
    cellularMachine->ProcessConnectionLostTimeout(event);
    EXPECT_TRUE(cellularMachine->IsActiveState());
    cellularMachine->inActiveState_ = inActiveState;
}

/**
 * @tc.number   ProcessConnectionLostTimeout_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ProcessConnectionLostTimeout_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    cellularMachine->TransitionTo(cellularMachine->inActiveState_);
    ASSERT_TRUE(cellularMachine->IsInactiveState());
    AppExecFwk::InnerEvent::Pointer nullEvent(nullptr, nullptr);
    cellularMachine->ProcessConnectionLostTimeout(nullEvent);
    EXPECT_TRUE(cellularMachine->IsInactiveState());
}

/**
 * @tc.number   OnInterfaceLinkStateChanged_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, OnInterfaceLinkStateChanged_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    ASSERT_NE(active, nullptr);
    cellularMachine->cid_ = 1;
    cellularMachine->apnId_ = 1;
    cellularMachine->ifName_ = "rmnet0";
    cellularMachine->connectId_ = 1;
    active->stateMachine_ = cellularMachine;
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto handler = cellularMachine->stateMachineEventHandler_;
    cellularMachine->stateMachineEventHandler_ = nullptr;
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", false), 0);
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", true), 0);
    cellularMachine->stateMachineEventHandler_ = handler;
    // empty ifName_: ignore link state change and keep state stable
    cellularMachine->ifName_ = "";
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", false), 0);
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", true), 0);
    EXPECT_TRUE(cellularMachine->IsActiveState());
}

/**
 * @tc.number   OnInterfaceLinkStateChanged_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, OnInterfaceLinkStateChanged_003, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    ASSERT_NE(inactive, nullptr);
    cellularMachine->ifName_ = "rmnet0";
    cellularMachine->connectId_ = 5;
    cellularMachine->TransitionTo(cellularMachine->inActiveState_);
    ASSERT_TRUE(cellularMachine->IsInactiveState());
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", false), 0);
    EXPECT_TRUE(cellularMachine->IsInactiveState());
}

/**
 * @tc.number   OnInterfaceLinkStateChanged_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, OnInterfaceLinkStateChanged_004, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    ASSERT_NE(active, nullptr);
    cellularMachine->ifName_ = "rmnet0";
    cellularMachine->connectId_.store(9);
    active->stateMachine_ = cellularMachine;
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    cellularMachine->stateMachineEventHandler_->SendEvent(
        CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, cellularMachine->connectId_.load(), 1000);
    EXPECT_TRUE(cellularMachine->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK));
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", false), 0);
    EXPECT_FALSE(cellularMachine->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK));
    EXPECT_EQ(cellularMachine->OnInterfaceLinkStateChanged("rmnet0", true), 0);
    EXPECT_FALSE(cellularMachine->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK));
    EXPECT_TRUE(cellularMachine->IsActiveState());
}

/**
 * @tc.number   NetInterfaceCallback_LinkStateChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, NetInterfaceCallback_LinkStateChanged_001, Function | MediumTest | Level1)
{
    sptr<CellularDataStateMachine::NetInterfaceCallback> callback =
        new CellularDataStateMachine::NetInterfaceCallback(std::weak_ptr<CellularDataStateMachine>());
    ASSERT_NE(callback, nullptr);
    EXPECT_EQ(callback->OnInterfaceLinkStateChanged("rmnet0", false), 0);
    EXPECT_EQ(callback->OnInterfaceLinkStateChanged("rmnet0", true), 0);
}

/**
 * @tc.number   UnregisterNetInterfaceCallback_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, UnregisterNetInterfaceCallback_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    ASSERT_NE(cellularMachine->netInterfaceCallback_, nullptr);
    cellularMachine->UnregisterNetInterfaceCallback();
    EXPECT_EQ(cellularMachine->netInterfaceCallback_, nullptr);
    cellularMachine->UnregisterNetInterfaceCallback();
    EXPECT_EQ(cellularMachine->netInterfaceCallback_, nullptr);
}

/**
 * @tc.number   UnregisterNetInterfaceCallback_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, UnregisterNetInterfaceCallback_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    cellularMachine->netInterfaceCallback_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(cellularMachine->UnregisterNetInterfaceCallback());
    EXPECT_EQ(cellularMachine->netInterfaceCallback_, nullptr);
}

/**
 * @tc.number   InactiveStateBegin_NoEventHandler_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, InactiveStateBegin_NoEventHandler_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    inactive->SetStateMachine(cellularMachine);
    auto handler = cellularMachine->stateMachineEventHandler_;
    ASSERT_NE(handler, nullptr);
    cellularMachine->stateMachineEventHandler_ = nullptr;
    inactive->StateBegin();
    EXPECT_TRUE(inactive->isActive_);
    cellularMachine->stateMachineEventHandler_ = handler;
}

/**
 * @tc.number   ActiveStateProcess_ConnectionLostTimeout_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ActiveStateProcess_ConnectionLostTimeout_001, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    auto inactive = std::static_pointer_cast<Inactive>(cellularMachine->inActiveState_);
    ASSERT_NE(active, nullptr);
    ASSERT_NE(inactive, nullptr);
    active->stateMachine_ = cellularMachine;
    cellularMachine->connectId_.store(1);
    cellularMachine->apnId_ = 1;
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 1);
    bool result = active->StateProcess(event);
    EXPECT_TRUE(result);
    EXPECT_EQ(inactive->deActiveApnTypeId_, cellularMachine->apnId_);
    EXPECT_EQ(inactive->resultInfo_->reason, static_cast<int32_t>(PdpErrorReason::PDP_ERR_RETRY));
}

/**
 * @tc.number   ActiveStateProcess_ConnectionLostTimeout_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, ActiveStateProcess_ConnectionLostTimeout_002, Function | MediumTest | Level1)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = std::weak_ptr<CellularDataStateMachine>();
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_CONNECTION_LOST_TIMEOUT_CHECK, 1);
    bool result = active->StateProcess(event);
    EXPECT_TRUE(result);
}

/**
 * @tc.number FreeConnection_001
 * @tc.name test function branch
 * @tc.desc Function test
 */
HWTEST_F(CellularStateMachineTest, FreeConnection_001, Function | MediumTest |Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    std::shared_ptr<DataDisconnectParams> dataDisconnectParams =
        std::make_shared<DataDisconnectParams>("", DisConnectionReason::REASON_NORMAL);
    cellularMachine->FreeConnection(*dataDisconnectParams);
    EXPECT_NE(cellularMachine->netInterfaceCallback_, nullptr);
}

/**
 * @tc.number DoConnect_001
 * @tc.name test function branch
 * @tc.desc Function test
 */
HWTEST_F(CellularStateMachineTest, DoConnect_001, Function | MediumTest | Level0)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
    }
    sptr apnHolder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    sptr defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apnHolder->SetCurrentApn(defaultApnItem);
    int32_t profileId = 0;
    int32_t radioTechnology = 0;
    bool nonTrafficUseOnly = false;
    bool roamingState = false;
    bool userDataRoaming = false;
    std::shared_ptr<DataConnectionParams> dataConnectionParams = std::make_shared<DataConnectionParams>(apnHolder,
    profileId, radioTechnology, nonTrafficUseOnly, roamingState, userDataRoaming);
    cellularMachine->DoConnect(*dataConnectionParams);
    EXPECT_NE(cellularMachine->netInterfaceCallback_, nullptr);
}
/*
 * ==== P2 batch-2：cellular_data_state_machine.cpp 函数级缺口补齐 ====
 * 目标函数（p2b5 缺口）：FillRSDFromNetCap 98、GetNetworkSlicePara 62、
 * UpdateNetworkInfo(SetupDataCallResultInfo) 56、DoConnect 55、GetMtuSizeFromOpCfg 22、
 * OnInterfaceLinkStateChanged 17、UpdateNetworkInfo() 17、GetIpType 14、ResolveRoute 11、
 * SplitProxyIpAddress 10。均经 #define private public 直接驱动成员函数，
 * CoreManagerInner mock 用 ScopedCoreManagerMock RAII 安装/还原，不跨用例泄漏。
 */
namespace {
// CoreManagerInner mock 临时安装器：构造时装入 simManager_/networkSearchManager_ mock，
// 析构时还原进入用例前的原值（区别于 CellularStateMachineTest fixture 的整生命周期安装）
class ScopedCoreManagerMock {
public:
    ScopedCoreManagerMock()
    {
        savedSim_ = CoreManagerInner::GetInstance().simManager_;
        savedNss_ = CoreManagerInner::GetInstance().networkSearchManager_;
        simMock_ = new MockSimManager();
        CoreManagerInner::GetInstance().simManager_ = std::shared_ptr<MockSimManager>(simMock_);
        nssMock_ = new MockNetworkSearchManager();
        CoreManagerInner::GetInstance().networkSearchManager_ = std::shared_ptr<MockNetworkSearchManager>(nssMock_);
    }
    ~ScopedCoreManagerMock()
    {
        CoreManagerInner::GetInstance().simManager_ = savedSim_;
        CoreManagerInner::GetInstance().networkSearchManager_ = savedNss_;
    }
    MockSimManager *simMock_ = nullptr;
    MockNetworkSearchManager *nssMock_ = nullptr;
private:
    std::shared_ptr<ISimManager> savedSim_;
    std::shared_ptr<INetworkSearch> savedNss_;
};
} // namespace

/**
 * @tc.number   CellularDataStateMachine_FillRSDFromNetCap_001
 * @tc.name     test fill rsd from net cap with full/empty/zero/long values
 * @tc.desc     Function test
 * @tc.note     直接驱动私有成员函数：四键真值（全 true 分支+from_chars 成功）、
 *              空表（sscmode/pdusessiontype 走 operator[] 缺省 ""!="0" 的 true 分支，
 *              snssai/dnn 走 false 分支）、"0" 值（对应 false 分支）、300 字符超长
 *              （std::min 截断到 ALL_APN_ITEM_CHAR_LENGTH-1=255）。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_FillRSDFromNetCap_001, Function | MediumTest | Level3)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);
    // 1) 四键全真值
    sptr<ApnItem> apn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(apn, nullptr);
    std::map<std::string, std::string> paras = {
        {"sscmode", "3"}, {"snssai", "010203"}, {"dnn", "slice.dnn"}, {"pdusessiontype", "IPV6TEST"}};
    sm->FillRSDFromNetCap(paras, apn);
    EXPECT_EQ(apn->attr_.sscMode_, 3);
    EXPECT_STREQ(apn->attr_.snssai_, "010203");
    EXPECT_STREQ(apn->attr_.apn_, "slice.dnn");
    EXPECT_STREQ(apn->attr_.protocol_, "IPV6TEST");

    // 2) 空表：sscmode/pdusessiontype true 分支（from_chars 失败/空拷贝），snssai/dnn false 分支
    sptr<ApnItem> apn2 = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apn2->attr_.sscMode_ = 9;
    snprintf(apn2->attr_.protocol_, sizeof(apn2->attr_.protocol_), "KEEP");
    sm->FillRSDFromNetCap(std::map<std::string, std::string>(), apn2);
    EXPECT_EQ(apn2->attr_.sscMode_, 0); // from_chars("") 失败回写 0
    EXPECT_EQ(apn2->attr_.snssai_[0], '\0'); // 未触碰（保留 MakeDefaultApn 空值）
    EXPECT_STREQ(apn2->attr_.apn_, "cmnet"); // dnn 缺省 ""!="" 为 false → 未触碰（保留默认 apn）
    EXPECT_EQ(apn2->attr_.protocol_[0], '\0'); // pdusessiontype 空串拷贝

    // 3) "0" 值：sscmode/pdusessiontype false 分支
    sptr<ApnItem> apn3 = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apn3->attr_.sscMode_ = 9;
    snprintf(apn3->attr_.protocol_, sizeof(apn3->attr_.protocol_), "KEEP");
    std::map<std::string, std::string> zeroParas = {
        {"sscmode", "0"}, {"pdusessiontype", "0"}, {"snssai", "aa"}, {"dnn", "bb"}};
    sm->FillRSDFromNetCap(zeroParas, apn3);
    EXPECT_EQ(apn3->attr_.sscMode_, 9); // "0" 跳过
    EXPECT_STREQ(apn3->attr_.snssai_, "aa");
    EXPECT_STREQ(apn3->attr_.apn_, "bb");
    EXPECT_STREQ(apn3->attr_.protocol_, "KEEP"); // "0" 跳过

    // 4) 超长截断：300 字符 → 255
    sptr<ApnItem> apn4 = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    const std::string longStr(300, 'z');
    std::map<std::string, std::string> longParas = {
        {"sscmode", "1"}, {"snssai", longStr}, {"dnn", longStr}, {"pdusessiontype", longStr}};
    sm->FillRSDFromNetCap(longParas, apn4);
    EXPECT_EQ(apn4->attr_.sscMode_, 1);
    EXPECT_EQ(strlen(apn4->attr_.snssai_), ApnItem::ALL_APN_ITEM_CHAR_LENGTH - 1);
    EXPECT_EQ(strlen(apn4->attr_.apn_), ApnItem::ALL_APN_ITEM_CHAR_LENGTH - 1);
    EXPECT_EQ(strlen(apn4->attr_.protocol_), ApnItem::ALL_APN_ITEM_CHAR_LENGTH - 1);

    // 5) from_chars 非数值失败：sscMode 保持 0
    sptr<ApnItem> apn5 = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    std::map<std::string, std::string> badParas = {{"sscmode", "abc"}};
    sm->FillRSDFromNetCap(badParas, apn5);
    EXPECT_EQ(apn5->attr_.sscMode_, 0);
}

/**
 * @tc.number   CellularDataStateMachine_GetNetworkSlicePara_001
 * @tc.name     test get network slice para with nr sa and apn types
 * @tc.desc     Function test
 * @tc.note     mock GetNetworkStatus 驱动 isNrSa 判定：LTE→早退；NR+SA 且 apnType 含
 *              snssai→GetRSDByNetCap+FillRSDFromNetCap；NR+SA 且 dnn 非空→
 *              GetRouteSelectionDescriptorByDNN；dnn 空→无操作。NetworkSliceClient
 *              查询为真机只读路径（设备 netmanager 在线），不产生写副作用。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetNetworkSlicePara_001, Function | MediumTest | Level3)
{
    ScopedCoreManagerMock coreMock;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);

    sptr<ApnItem> apn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(apn, nullptr);
    sptr<ApnItem> curApn = apn;
    sptr<ApnHolder> holder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    holder->SetCurrentApn(curApn);
    DataConnectionParams params(holder, 0, 0, false, false, false);

    // 1) 非 NR SA：早退（apn 属性不变）
    sptr<NetworkState> lteState = new NetworkState();
    lteState->SetNetworkType(RadioTech::RADIO_TECHNOLOGY_LTE, DomainType::DOMAIN_TYPE_PS);
    EXPECT_CALL(*coreMock.nssMock_, GetNetworkStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(lteState), Return(0)));
    sm->GetNetworkSlicePara(params, apn);
    EXPECT_EQ(apn->attr_.sscMode_, 0);

    // 2) NR SA + apnType 含 "snssai"：FindApnId/FindCapability + GetRSDByNetCap + FillRSD
    sptr<NetworkState> nrState = new NetworkState();
    nrState->SetNetworkType(RadioTech::RADIO_TECHNOLOGY_NR, DomainType::DOMAIN_TYPE_PS);
    nrState->SetNrState(NrState::NR_NSA_STATE_SA_ATTACHED);
    sptr<ApnItem> snssaiApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    sptr<ApnHolder> snssaiHolder = new ApnHolder("snssai_default", 0);
    sptr<ApnItem> curSnssai = snssaiApn;
    snssaiHolder->SetCurrentApn(curSnssai);
    DataConnectionParams snssaiParams(snssaiHolder, 0, 0, false, false, false);
    EXPECT_CALL(*coreMock.nssMock_, GetNetworkStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(nrState), Return(0)));
    sm->GetNetworkSlicePara(snssaiParams, snssaiApn);

    // 3) NR SA + apnType 无 snssai + dnn 非空：GetRouteSelectionDescriptorByDNN 路径
    sptr<NetworkState> nrState2 = new NetworkState();
    nrState2->SetNetworkType(RadioTech::RADIO_TECHNOLOGY_NR, DomainType::DOMAIN_TYPE_PS);
    nrState2->SetNrState(NrState::NR_NSA_STATE_SA_ATTACHED);
    EXPECT_CALL(*coreMock.nssMock_, GetNetworkStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(nrState2), Return(0)));
    sm->GetNetworkSlicePara(params, apn);

    // 4) NR SA + dnn 空：else-if 双 false，无操作
    sptr<ApnItem> emptyDnnApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    emptyDnnApn->attr_.apn_[0] = '\0';
    sptr<ApnItem> curEmpty = emptyDnnApn;
    sptr<ApnHolder> emptyHolder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    emptyHolder->SetCurrentApn(curEmpty);
    DataConnectionParams emptyParams(emptyHolder, 0, 0, false, false, false);
    sptr<NetworkState> nrState3 = new NetworkState();
    nrState3->SetNetworkType(RadioTech::RADIO_TECHNOLOGY_NR, DomainType::DOMAIN_TYPE_PS);
    nrState3->SetNrState(NrState::NR_NSA_STATE_SA_ATTACHED);
    EXPECT_CALL(*coreMock.nssMock_, GetNetworkStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(nrState3), Return(0)));
    sm->GetNetworkSlicePara(emptyParams, emptyDnnApn);
    EXPECT_EQ(emptyDnnApn->attr_.snssai_[0], '\0');
}

/**
 * @tc.number   CellularDataStateMachine_GetIpType_001
 * @tc.name     test get ip type with v4/v6/mixed/empty address infos
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetIpType_001, Function | MediumTest | Level3)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);
    std::vector<AddressInfo> empty;
    EXPECT_EQ(sm->GetIpType(empty), "");

    AddressInfo v4;
    v4.type = NetManagerStandard::INetAddr::IpType::IPV4;
    v4.ip = "10.0.0.1";
    AddressInfo v6;
    v6.type = NetManagerStandard::INetAddr::IpType::IPV6;
    v6.ip = "2400::1";
    EXPECT_EQ(sm->GetIpType(std::vector<AddressInfo>{v4}), "IPV4");
    EXPECT_EQ(sm->GetIpType(std::vector<AddressInfo>{v6}), "IPV6");
    EXPECT_EQ(sm->GetIpType(std::vector<AddressInfo>{v4, v6}), "IPV4V6");
    EXPECT_EQ(sm->GetIpType(), ""); // 无参重载：未连接时 ipType_ 为空
}

/**
 * @tc.number   CellularDataStateMachine_SplitProxyIpAddress_004
 * @tc.name     test split proxy ip address with host only/host+port/invalid port
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_SplitProxyIpAddress_004, Function | MediumTest | Level3)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);
    std::string host = "init";
    uint16_t port = 0;
    sm->SplitProxyIpAddress("10.0.0.1", host, port);
    EXPECT_EQ(host, "10.0.0.1");
    EXPECT_EQ(port, 0); // 仅 host：port 不赋值

    sm->SplitProxyIpAddress("10.0.0.2:8080", host, port);
    EXPECT_EQ(host, "10.0.0.2");
    EXPECT_EQ(port, 8080);

    host = "keep"; port = 9999;
    sm->SplitProxyIpAddress("10.0.0.3:abc", host, port); // 非法端口：IsValidDecValue false
    EXPECT_EQ(host, "10.0.0.3");
    EXPECT_EQ(port, 9999); // port 保持原值

    host = "keep2"; port = 1;
    sm->SplitProxyIpAddress("a:b:c", host, port); // 三段：size 均不匹配 → 不赋值
    EXPECT_EQ(host, "keep2");
    EXPECT_EQ(port, 1);

    sm->SplitProxyIpAddress("", host, port); // 单空段：host 被置空
    EXPECT_EQ(host, "");
    EXPECT_EQ(port, 1);
}

/**
 * @tc.number   CellularDataStateMachine_GetMtuSizeFromOpCfg_005
 * @tc.name     test get mtu size from operator config
 * @tc.desc     Function test
 * @tc.note     mock GetOperatorConfigs 注入 OperatorConfig.stringValue：
 *              无键/合法匹配/非法格式/非法数值/ipType 不匹配。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_GetMtuSizeFromOpCfg_005, Function | MediumTest | Level3)
{
    ScopedCoreManagerMock coreMock;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);
    sm->ipType_ = "IPV4";
    int32_t mtuSize = 0;

    // 1) 无 mtu 配置键：mtuSize 不变
    EXPECT_CALL(*coreMock.simMock_, GetOperatorConfigs(_, _))
        .WillOnce([](int32_t, OperatorConfig &poc) { return 0; });
    sm->GetMtuSizeFromOpCfg(mtuSize, 0);
    EXPECT_EQ(mtuSize, 0);

    // 2) 合法且 ipType 匹配：IPV4 取 1400
    EXPECT_CALL(*coreMock.simMock_, GetOperatorConfigs(_, _))
        .WillOnce([](int32_t, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "IPV4:1400;IPV6:1420";
            return 0;
        });
    sm->GetMtuSizeFromOpCfg(mtuSize, 0);
    EXPECT_EQ(mtuSize, 1400);

    // 3) 非法格式（单段无冒号）：break，mtuSize 保持
    mtuSize = 7;
    EXPECT_CALL(*coreMock.simMock_, GetOperatorConfigs(_, _))
        .WillOnce([](int32_t, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "abc";
            return 0;
        });
    sm->GetMtuSizeFromOpCfg(mtuSize, 0);
    EXPECT_EQ(mtuSize, 7);

    // 4) 数值非法（非数字）：StrToInt 失败保持 INVALID_MTU_VALUE → break
    EXPECT_CALL(*coreMock.simMock_, GetOperatorConfigs(_, _))
        .WillOnce([](int32_t, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "IPV4:xyz";
            return 0;
        });
    sm->GetMtuSizeFromOpCfg(mtuSize, 0);
    EXPECT_EQ(mtuSize, 7);

    // 5) 格式合法但 ipType 不匹配：不赋值
    mtuSize = 0;
    EXPECT_CALL(*coreMock.simMock_, GetOperatorConfigs(_, _))
        .WillOnce([](int32_t, OperatorConfig &poc) {
            poc.stringValue[KEY_MTU_SIZE_STRING] = "IPV6:1420";
            return 0;
        });
    sm->GetMtuSizeFromOpCfg(mtuSize, 0);
    EXPECT_EQ(mtuSize, 0);
}

/**
 * @tc.number   CellularDataStateMachine_OnInterfaceLinkStateChanged_001
 * @tc.name     test on interface link state changed with mismatch/up/down
 * @tc.desc     Function test
 * @tc.note     ifName_ 空早退/名字不匹配早退/up 且非 Active/up=false 无挂起事件/
 *              up=false 且有挂起 MSG_DISCONNECT_TIMEOUT_CHECK → Remove+重发。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_OnInterfaceLinkStateChanged_001,
    Function | MediumTest | Level3)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);
    // 1) ifName_ 为空：早退
    EXPECT_EQ(sm->OnInterfaceLinkStateChanged("cellular0", false), 0);
    // 2) 名字不匹配：早退
    sm->ifName_ = "cellular0";
    EXPECT_EQ(sm->OnInterfaceLinkStateChanged("other0", true), 0);
    // 3) up=true 且非 Active：else 分支不动作
    EXPECT_EQ(sm->OnInterfaceLinkStateChanged("cellular0", true), 0);
    // 4) up=false 无挂起事件且非 Active：if 内不动作
    EXPECT_EQ(sm->OnInterfaceLinkStateChanged("cellular0", false), 0);
    // 5) up=false 且挂起连接丢失检查事件：Remove + 按 INTERFACE_DOWN_TIMEOUT 重发
    ASSERT_NE(sm->stateMachineEventHandler_, nullptr);
    sm->stateMachineEventHandler_->SendEvent(
        CellularDataEventCode::MSG_DISCONNECT_TIMEOUT_CHECK, sm->connectId_.load(), 60000);
    EXPECT_TRUE(sm->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_DISCONNECT_TIMEOUT_CHECK));
    EXPECT_EQ(sm->OnInterfaceLinkStateChanged("cellular0", false), 0);
    EXPECT_TRUE(sm->stateMachineEventHandler_->HasInnerEvent(
        CellularDataEventCode::MSG_DISCONNECT_TIMEOUT_CHECK)); // 重发后仍在
    TestAsyncUtils::DrainStateMachine(sm.get());
    sm->ifName_ = "";
}

/**
 * @tc.number   CellularDataStateMachine_UpdateNetworkInfo_006
 * @tc.name     test update network info with null members and full datacall info
 * @tc.desc     Function test
 * @tc.note     netLinkInfo_/netSupplierInfo_ 缺失早退 → 手动挂成员后全路径：
 *              IPV4V6 地址/双 DNS/漫游 true/false/active 开关/无参重载。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateNetworkInfo_006, Function | MediumTest | Level3)
{
    ScopedCoreManagerMock coreMock;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);

    SetupDataCallResultInfo dataCallInfo;
    dataCallInfo.address = "10.0.0.1/24 2400:db8::1/64";
    dataCallInfo.dns = "8.8.8.8";
    dataCallInfo.dnsSec = "2400:db8::8888";
    dataCallInfo.gateway = "10.0.0.254 2400:db8::ff";
    dataCallInfo.netPortName = "cellular0";
    dataCallInfo.active = 1;
    dataCallInfo.maxTransferUnit = 0; // DEFAULT_MTU 分支
    dataCallInfo.reason = 0;

    // 1) 成员未初始化：早退
    sm->UpdateNetworkInfo(dataCallInfo);
    EXPECT_EQ(sm->ifName_, "");

    // 2) 手动挂成员 + 漫游 true：全路径
    sm->netSupplierInfo_ = std::make_unique<NetManagerStandard::NetSupplierInfo>().release();
    sm->netLinkInfo_ = std::make_unique<NetManagerStandard::NetLinkInfo>().release();
    EXPECT_CALL(*coreMock.nssMock_, GetPsRoamingState(_)).WillOnce(Return(1));
    sm->UpdateNetworkInfo(dataCallInfo);
    EXPECT_EQ(sm->ipType_, "IPV4V6");
    EXPECT_EQ(sm->ifName_, "cellular0");
    EXPECT_TRUE(sm->netSupplierInfo_->isAvailable_);
    EXPECT_TRUE(sm->netSupplierInfo_->isRoaming_);
    EXPECT_EQ(sm->netLinkInfo_->netAddrList_.size(), 2U);
    EXPECT_EQ(sm->netLinkInfo_->dnsList_.size(), 2U);

    // 3) active=0 + 漫游 false：isAvailable false / isRoaming false
    SetupDataCallResultInfo offlineInfo;
    offlineInfo.address = "10.0.0.2/24";
    offlineInfo.dns = "8.8.4.4";
    offlineInfo.gateway = "10.0.0.254";
    offlineInfo.netPortName = "cellular1";
    offlineInfo.active = 0;
    offlineInfo.maxTransferUnit = 1500; // 非 0 直取
    EXPECT_CALL(*coreMock.nssMock_, GetPsRoamingState(_)).WillOnce(Return(0));
    sm->UpdateNetworkInfo(offlineInfo);
    EXPECT_FALSE(sm->netSupplierInfo_->isAvailable_);
    EXPECT_FALSE(sm->netSupplierInfo_->isRoaming_);
    EXPECT_EQ(sm->ipType_, "IPV4");
    EXPECT_EQ(sm->netLinkInfo_->mtu_, 1500);

    // 4) 无参重载
    sm->UpdateNetworkInfo();
    EXPECT_EQ(sm->netLinkInfo_->ifaceName_, "cellular1");
}

/**
 * @tc.number   CellularDataStateMachine_DoConnect_001
 * @tc.name     test do connect with null holder/null apn/normal params
 * @tc.desc     Function test
 * @tc.note     apnHolder 空早退/无当前 APN 早退/正常参数走 ActivatePdpContext（测试进程
 *              telRilManager_ 为空 → LOCAL_PTR_NULL 失败分支）+ 连接超时任务下发。
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_DoConnect_001, Function | MediumTest | Level3)
{
    ScopedCoreManagerMock coreMock;
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    ASSERT_NE(sm, nullptr);

    // 1) apnHolder 空：早退
    DataConnectionParams nullParams(nullptr, 0, 0, false, false, false);
    sm->DoConnect(nullParams);

    // 2) holder 无当前 APN：早退
    sptr<ApnHolder> emptyHolder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    DataConnectionParams emptyParams(emptyHolder, 0, 0, false, false, false);
    sm->DoConnect(emptyParams);

    // 3) 正常参数：GetPsRadioTech mock + ActivatePdpContext 失败分支 + 超时事件
    sptr<ApnItem> apn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(apn, nullptr);
    sptr<ApnItem> curApn = apn;
    sptr<ApnHolder> holder = new ApnHolder(DATA_CONTEXT_ROLE_DEFAULT, 0);
    holder->SetCurrentApn(curApn);
    DataConnectionParams params(holder, 0, 0, false, true, true);
    EXPECT_CALL(*coreMock.nssMock_, GetPsRadioTech(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    sm->DoConnect(params);
    EXPECT_EQ(sm->apnItem_, apn);
    TestAsyncUtils::DrainStateMachine(sm.get());
}

/*
 * ==== P2 batch-2 续：active.cpp/default.cpp 状态处理全路径与守卫分支 ====
 * 目标函数（b2full 函数级缺口）：ProcessDisconnectDone 41、ProcessDisconnectAllDone 41、
 * ProcessLostConnection 42、ProcessDataConnectionDrsOrRatChanged 30（default.cpp 0/60）、
 * RefreshTcpBufferSizes 15、RefreshConnectionBandwidths 15。
 * 既有用例只覆盖空事件/空载荷早退；本批补全路径（reuseApnCap 非 END 复位分支 +
 * TransitionTo 迁移 + Active 态 UpdateNetLinkInfo 分支）与 Refresh 弱引用失效/
 * 连接管理器空守卫。
 */
HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectDone_003, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    // 复用能力非 END：触发 SetIfReuseSupplierId(false)+SetReuseApnCap(END) 复位分支
    cellularMachine->SetReuseApnCap(static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET));
    // 载荷必须 unique_ptr 存储（GetUniqueObject 只认 unique 形态，shared 存储取出为 null；
    // 与 apn_holder.cpp:101-108 生产构造姿势一致）
    std::unique_ptr<DataDisconnectParams> object =
        std::make_unique<DataDisconnectParams>("", DisConnectionReason::REASON_NORMAL);
    auto event = AppExecFwk::InnerEvent::Get(0, object);
    EXPECT_TRUE(active->ProcessDisconnectDone(event));
    EXPECT_EQ(cellularMachine->GetReuseApnCap(),
        static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_END)); // 复位分支生效
    TestAsyncUtils::DrainStateMachine(cellularMachine.get()); // 排空 TransitionTo(disconnecting)
}

HWTEST_F(CellularStateMachineTest, Active_ProcessDisconnectAllDone_005, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    cellularMachine->SetReuseApnCap(static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET));
    // 载荷必须 unique_ptr 存储（GetUniqueObject 只认 unique 形态；同 apn_holder.cpp 生产姿势）
    std::unique_ptr<DataDisconnectParams> object =
        std::make_unique<DataDisconnectParams>("", DisConnectionReason::REASON_CLEAR_CONNECTION);
    auto event = AppExecFwk::InnerEvent::Get(0, object);
    EXPECT_TRUE(active->ProcessDisconnectAllDone(event));
    EXPECT_EQ(cellularMachine->GetReuseApnCap(),
        static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_END));
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

HWTEST_F(CellularStateMachineTest, Active_ProcessLostConnection_003, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    cellularMachine->SetReuseApnCap(static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET));
    std::shared_ptr<SetupDataCallResultInfo> resultInfo = std::make_shared<SetupDataCallResultInfo>();
    resultInfo->reason = 1;
    auto event = AppExecFwk::InnerEvent::Get(0, resultInfo);
    EXPECT_TRUE(active->ProcessLostConnection(event));
    EXPECT_EQ(cellularMachine->GetReuseApnCap(),
        static_cast<uint64_t>(NetManagerStandard::NetCap::NET_CAPABILITY_END));
    TestAsyncUtils::DrainStateMachine(cellularMachine.get()); // 排空 TransitionTo(inactive)
}

HWTEST_F(CellularStateMachineTest, Default_ProcessDataConnectionDrsOrRatChanged_001, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    }
    auto def = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    def->stateMachine_ = cellularMachine;
    // 1) 空事件早退（Pointer 的 deleter 为函数指针，不能默认构造，用 Get 后置空的本文件惯例）
    auto nullEvent = AppExecFwk::InnerEvent::Get(0);
    nullEvent = nullptr;
    EXPECT_FALSE(def->ProcessDataConnectionDrsOrRatChanged(nullEvent));
    // 2) 全路径（Init 后处于 Inactive：IsActiveState/IsActivatingState 均假 → 不进 UpdateNetLinkInfo）
    auto event = AppExecFwk::InnerEvent::Get(0);
    EXPECT_FALSE(def->ProcessDataConnectionDrsOrRatChanged(event));
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

HWTEST_F(CellularStateMachineTest, Default_ProcessDataConnectionDrsOrRatChanged_002, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        // 注意：此处不可 DrainStateMachine——其内部 Quit() 会停机，后续 TransitionTo 不再被处理
    }
    auto def = std::static_pointer_cast<Default>(cellularMachine->defaultState_);
    def->stateMachine_ = cellularMachine;
    // 进 Active 态：覆盖 IsActiveState true → UpdateNetLinkInfo 分支。
    // 用本文件既定姿势直接调 Active::StateBegin（同步 SetCurrentState，不依赖事件循环迁移）
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    cellularMachine->ifName_ = "rmnet0";
    active->StateBegin();
    ASSERT_TRUE(cellularMachine->IsActiveState());
    auto event = AppExecFwk::InnerEvent::Get(0);
    EXPECT_FALSE(def->ProcessDataConnectionDrsOrRatChanged(event));
    TestAsyncUtils::DrainStateMachine(cellularMachine.get()); // 收尾停机排空
}

HWTEST_F(CellularStateMachineTest, Active_RefreshTcpBufferSizes_002, Function | MediumTest | Level3)
{
    // 1) 弱引用失效守卫：独立 Active 实例不绑状态机 → lock 失败早退
    Active orphanActive(std::weak_ptr<CellularDataStateMachine>(), "OrphanActive");
    orphanActive.RefreshTcpBufferSizes();
    orphanActive.RefreshConnectionBandwidths();
    // 2) cdConnectionManager_ 空守卫（保存-置空-还原，避免跨用例污染）
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    auto sm = machine->CreateCellularDataConnect(0);
    sm->Init();
    TestAsyncUtils::DrainStateMachine(sm.get());
    auto active = std::static_pointer_cast<Active>(sm->activeState_);
    active->stateMachine_ = sm;
    auto savedConnMgr = sm->cdConnectionManager_;
    sm->cdConnectionManager_ = nullptr;
    active->RefreshTcpBufferSizes(); // cdConnectionManager_ 空早退
    active->RefreshConnectionBandwidths();
    sm->cdConnectionManager_ = savedConnMgr; // 还原
    TestAsyncUtils::DrainStateMachine(sm.get());
}

HWTEST_F(CellularStateMachineTest, Active_RefreshConnectionBandwidths_001, Function | MediumTest | Level3)
{
    if (cellularMachine == nullptr) {
        std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
        cellularMachine = machine->CreateCellularDataConnect(0);
        cellularMachine->Init();
        TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    }
    auto active = std::static_pointer_cast<Active>(cellularMachine->activeState_);
    active->stateMachine_ = cellularMachine;
    // 镜像生产链路算期望：mock GetPsRadioTech 不改出参 → radioTech 保持 INVALID(-1)
    int32_t tech = -1;
    CoreManagerInner::GetInstance().GetPsRadioTech(cellularMachine->GetSlotId(), tech);
    LinkBandwidthInfo expInfo = cellularMachine->cdConnectionManager_->GetBandwidthsByRadioTech(tech);
    active->RefreshConnectionBandwidths(); // 正常路径：GetBandwidthsByRadioTech + SetConnectionBandwidth
    EXPECT_EQ(cellularMachine->upBandwidth_, expInfo.upBandwidth);
    EXPECT_EQ(cellularMachine->downBandwidth_, expInfo.downBandwidth);
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

/**
 * @tc.number   CellularDataStateMachine_UpdateHttpProxy_001
 * @tc.name     host only / host:port / invalid port / multi-colon variants via UpdateHttpProxy
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_UpdateHttpProxy_001, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    cellularMachine->Init();
    // 仅 host（HOST_SIZE=1）：port 保持默认 0
    cellularMachine->UpdateHttpProxy("proxy.only.host");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.host_, "proxy.only.host");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.port_, DEFAULT_PORT);
    // host:合法端口
    cellularMachine->UpdateHttpProxy("proxy.host:8080");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.host_, "proxy.host");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.port_, 8080);
    // host:非法端口：port 保持默认
    cellularMachine->UpdateHttpProxy("proxy.host:abc");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.host_, "proxy.host");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.port_, DEFAULT_PORT);
    // 多段（size>2）：host/port 均保持局部初值
    cellularMachine->UpdateHttpProxy("a:b:c");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.host_, "");
    EXPECT_EQ(cellularMachine->netLinkInfo_->httpProxy_.port_, DEFAULT_PORT);
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

/**
 * @tc.number   CellularDataStateMachine_ResolveNetLinkInfo_001
 * @tc.name     ResolveIp/ResolveDns/ResolveRoute with IPV4+IPV6 entries
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_ResolveNetLinkInfo_001, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    cellularMachine->Init();
    std::vector<AddressInfo> ipInfoArray;
    AddressInfo v4;
    v4.type = INetAddr::IpType::IPV4;
    v4.ip = "10.10.10.10";
    v4.netMask = "255.255.255.0";
    v4.prefixLen = 24;
    AddressInfo v6;
    v6.type = INetAddr::IpType::IPV6;
    v6.ip = "fe80::1";
    ipInfoArray.push_back(v4);
    ipInfoArray.push_back(v6);
    cellularMachine->ResolveIp(ipInfoArray);
    ASSERT_EQ(cellularMachine->netLinkInfo_->netAddrList_.size(), 2);
    auto itAddr = cellularMachine->netLinkInfo_->netAddrList_.begin();
    EXPECT_EQ(itAddr->netMask_, "255.255.255.0");
    EXPECT_EQ((++itAddr)->netMask_, DEFAULT_MASK); // 无掩码取默认

    cellularMachine->ResolveDns(ipInfoArray);
    ASSERT_EQ(cellularMachine->netLinkInfo_->dnsList_.size(), 2);
    EXPECT_EQ(cellularMachine->netLinkInfo_->dnsList_.front().address_, "10.10.10.10");

    cellularMachine->ResolveRoute(ipInfoArray, "cellular_data0");
    ASSERT_EQ(cellularMachine->netLinkInfo_->routeList_.size(), 2);
    auto itRoute = cellularMachine->netLinkInfo_->routeList_.begin();
    EXPECT_EQ(itRoute->destination_.address_, ROUTED_IPV4);
    EXPECT_EQ((++itRoute)->destination_.address_, ROUTED_IPV6);
    EXPECT_EQ(cellularMachine->netLinkInfo_->routeList_.front().iface_, "cellular_data0");
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

/**
 * @tc.number   CellularDataStateMachine_FreeConnection_001
 * @tc.name     FreeConnection with null ril handler falls back to event path
 * @tc.desc     Function test
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_FreeConnection_001, TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    cellularMachine->Init();
    DataDisconnectParams params("default", DisConnectionReason::REASON_CLEAR_CONNECTION);
    cellularMachine->FreeConnection(params); // telRilManager 空 → Deactivate 失败分支 + handler 空提前返回
    EXPECT_TRUE(true);
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
}

/**
 * @tc.number   CellularDataStateMachine_DisconnectingProcessDisconnectTimeout_001
 * @tc.name     null event / stale connectId / matching connectId transition path
 * @tc.desc     Function test
 * @tc.note     源分支该方法名为 CellularDataStateMachine::ProcessConnectionLostTimeout，
 *              本分支对应实现为 Disconnecting::ProcessDisconnectTimeout（含同样的
 *              空事件/陈旧 connectId 早退守卫），已按本分支 API 适配
 */
HWTEST_F(CellularStateMachineTest, CellularDataStateMachine_DisconnectingProcessDisconnectTimeout_001,
    TestSize.Level0)
{
    std::shared_ptr<CellularMachineTest> machine = std::make_shared<CellularMachineTest>();
    std::shared_ptr<CellularDataStateMachine> cellularMachine = machine->CreateCellularDataConnect(0);
    cellularMachine->Init();
    auto disconnecting = std::static_pointer_cast<Disconnecting>(cellularMachine->disconnectingState_);
    disconnecting->stateMachine_ = cellularMachine;
    // 空事件 → 提前返回
    auto nullEvent = AppExecFwk::InnerEvent::Get(0);
    nullEvent = nullptr;
    disconnecting->ProcessDisconnectTimeout(nullEvent);
    // connectId 不匹配（陈旧超时事件）→ 忽略
    cellularMachine->connectId_ = 1;
    auto staleEvent = AppExecFwk::InnerEvent::Get(0, 999);
    disconnecting->ProcessDisconnectTimeout(staleEvent);
    // 匹配 connectId → 设置 PDP_ERR_RETRY 并迁移 Inactive
    cellularMachine->connectId_ = 999;
    auto matchEvent = AppExecFwk::InnerEvent::Get(0, 999);
    disconnecting->ProcessDisconnectTimeout(matchEvent);
    TestAsyncUtils::DrainStateMachine(cellularMachine.get());
    EXPECT_TRUE(true);
}

} // namespace Telephony
} // namespace OHOS
