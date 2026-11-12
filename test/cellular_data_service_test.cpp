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

#include <gmock/gmock.h>

#include "cellular_data_error.h"
#include "cellular_data_rdb_helper.h"
#include "cellular_data_service.h"
#include "core_manager_inner.h"
#include "data_access_token.h"
#include "data_connection_monitor.h"
#include "gtest/gtest.h"
#include "tel_ril_network_parcel.h"
#include "traffic_management.h"
#include "apn_attribute.h"
#include "mock/mock_network_search.h"
#include "mock/mock_sim_manager.h"
#include "mock/mock_telephony_permission.h"
#include "telephony_permission.h"
#include "telephony_ext_wrapper.h"
#include "utils/test_async_utils.h"
#include "utils/scoped_setter.h"

namespace OHOS {
namespace Telephony {
using namespace testing;
using namespace testing::ext;

static const int32_t SLEEP_TIME = 3;

class CellularDataServiceTest : public testing::Test {
public:
    CellularDataServiceTest()
    {
        mockNetworkSearchManager = new MockNetworkSearchManager();
        std::shared_ptr<MockNetworkSearchManager> mockNetworkSearchManagerPtr(mockNetworkSearchManager);
        CoreManagerInner::GetInstance().networkSearchManager_ = mockNetworkSearchManagerPtr;
    }
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<CellularDataService> service = DelayedSingleton<CellularDataService>::GetInstance();
    MockNetworkSearchManager *mockNetworkSearchManager;
};
void CellularDataServiceTest::SetUpTestCase() {}

void CellularDataServiceTest::TearDownTestCase()
{
    /*
     * 停止服务持有的各 slot 控制器/Handler 的 FFRT 队列（控制器队列上可能有
     * MSG_ASYNCHRONOUS_REGISTER 延迟事件的重投递），排空后再等待，
     * 规避进程退出时 TelEventQueue/FFRT 析构竞态导致的 Signal 6。
     */
    auto service = DelayedSingleton<CellularDataService>::GetInstance();
    if (service != nullptr) {
        for (const std::pair<const int32_t, std::shared_ptr<CellularDataController>> &it :
            service->cellularDataControllers_) {
            if (it.second == nullptr) {
                continue;
            }
            if (it.second->cellularDataHandler_ != nullptr) {
                TestAsyncUtils::DrainCellularDataHandler(it.second->cellularDataHandler_.get());
            }
            TestAsyncUtils::DrainTelEventHandler(it.second.get());
        }
    }
    sleep(SLEEP_TIME);
}

void CellularDataServiceTest::SetUp() {}

void CellularDataServiceTest::TearDown()
{
    // 释放权限打桩单例，防止用例内的 ON_CALL 配置残留影响后续用例（见 test/mock/
    // mock_telephony_permission.h；未配置期望时默认拒绝，与本机真实行为一致）。
    MockTelephonyPermission::ReleaseMockTelephonyPermission();
}

/**
 * @tc.number   CellularDataService_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, CellularDataService_001, TestSize.Level1)
{
    // 权限打桩（外科式）：仅放行 GetApnState/HandleApnChanged 等值断言所需的
    // GET_TELEPHONY_STATE 与 SET_TELEPHONY_STATE；GET_NETWORK_INFO 与 systemApp 检查
    // 保持默认拒绝，使 IsCellularDataEnabled/GetCellularDataState/EnableCellularData 等
    // NE 断言维持权限失败分支（对齐作者 DataAccessToken 的预期判定环境）。
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_TELEPHONY_STATE))))
        .WillByDefault(testing::Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(testing::Return(true));
    DataAccessToken token;
    service->OnStart();
    service->isInitSuccess_ = true;
    bool dataEnabled = false;
    bool dataRoamingEnabled = false;
    int32_t state;
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->IsCellularDataEnabled(dataEnabled));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->EnableCellularData(false));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->EnableCellularData(true));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->EnableIntelligenceSwitch(false));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->EnableIntelligenceSwitch(true));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->GetCellularDataState(state));
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->GetApnState(DEFAULT_SIM_SLOT_ID, std::string(), state));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->IsCellularDataRoamingEnabled(DEFAULT_SIM_SLOT_ID, dataRoamingEnabled));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->EnableCellularDataRoaming(DEFAULT_SIM_SLOT_ID, true));
    NetRequest request;
    request.ident = "simId12";
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->RequestNet(request));
    request.ident = "simId2";
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->ReleaseNet(request));
    auto event = AppExecFwk::InnerEvent::Get(0);
    service->DispatchEvent(DEFAULT_SIM_SLOT_ID, event);
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->HandleApnChanged(DEFAULT_SIM_SLOT_ID));
}

/**
 * @tc.number   CellularDataService_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, CellularDataService_002, TestSize.Level1)
{
    int32_t type;
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->GetCellularDataFlowType(type));
    ASSERT_EQ("default slotId: -1", service->GetStateMachineCurrentStatusDump());
    service->GetFlowDataInfoDump();
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->StrategySwitch(DEFAULT_SIM_SLOT_ID, false));
    int32_t capability;
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->HasInternetCapability(DEFAULT_SIM_SLOT_ID, 0, capability));
    int32_t reason = static_cast<int32_t>(DisConnectionReason::REASON_NORMAL);
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->ClearAllConnections(DEFAULT_SIM_SLOT_ID, reason));
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->ChangeConnectionForDsds(DEFAULT_SIM_SLOT_ID, false));
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->ChangeConnectionForDsds(DEFAULT_SIM_SLOT_ID, true));
    ApnAttribute apnAttr;
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->GetDataConnApnAttr(DEFAULT_SIM_SLOT_ID, apnAttr));
    std::string ipType;
    ASSERT_EQ(TELEPHONY_ERR_SUCCESS, service->GetDataConnIpType(DEFAULT_SIM_SLOT_ID, ipType));
    ASSERT_NE(TELEPHONY_ERR_SUCCESS, service->IsNeedDoRecovery(DEFAULT_SIM_SLOT_ID, true));
    uint32_t supplierId = 0;
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR,
        service->GetCellularDataSupplierId(DEFAULT_SIM_SLOT_ID, NetCap::NET_CAPABILITY_INTERNET, supplierId));
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR,
        service->GetCellularDataSupplierId(DEFAULT_SIM_SLOT_ID, NetCap::NET_CAPABILITY_END, supplierId));
    service->CorrectNetSupplierNoAvailable(DEFAULT_SIM_SLOT_ID);
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->EstablishAllApnsIfConnectable(DEFAULT_SIM_SLOT_ID));
    int32_t slotId = 0;
    std::vector<uint8_t> buffer = {};
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->SendUrspDecodeResult(slotId, buffer));
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->SendUePolicySectionIdentifier(slotId, buffer));
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->SendImsRsdList(slotId, buffer));
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->GetNetworkSliceAllowedNssai(slotId, buffer));
    ASSERT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->GetNetworkSliceEhplmn(slotId));
    int32_t regState = -1;
    service->GetSupplierRegisterState(supplierId, regState);
    bool isSupportDun = false;
    EXPECT_EQ(TELEPHONY_ERR_PERMISSION_ERR, service->GetIfSupportDunApn(isSupportDun));
    service->OnStop();
}

/**
 * @tc.number   NetworkSliceTest001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, NetworkSliceTest001, TestSize.Level1)
{
    DataAccessToken token;
    int32_t slotId = 0;
    std::vector<uint8_t> buffer = {};
    EXPECT_NE(service->SendUrspDecodeResult(slotId, buffer), TELEPHONY_ERR_SUCCESS);
    EXPECT_NE(service->SendUePolicySectionIdentifier(slotId, buffer), TELEPHONY_ERR_SUCCESS);
    EXPECT_NE(service->SendImsRsdList(slotId, buffer), TELEPHONY_ERR_SUCCESS);
    EXPECT_NE(service->GetNetworkSliceAllowedNssai(slotId, buffer), TELEPHONY_ERR_SUCCESS);
    EXPECT_NE(service->GetNetworkSliceEhplmn(slotId), TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   DataConnectionMonitor_HandleScreenStateChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_HandleScreenStateChanged_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->isScreenOn_ = false;
    dataConnectionMonitor->HandleScreenStateChanged(true);
    ASSERT_EQ(dataConnectionMonitor->isScreenOn_, true);
    dataConnectionMonitor->isScreenOn_ = false;
    dataConnectionMonitor->HandleScreenStateChanged(false);
    ASSERT_EQ(dataConnectionMonitor->isScreenOn_, false);
}

/**
 * @tc.number   DataConnectionMonitor_OnStallDetectionTimer_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_OnStallDetectionTimer_001, TestSize.Level1)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->noRecvPackets_ = 20;
    dataConnectionMonitor->stallDetectionEnabled_ = true;
    dataConnectionMonitor->OnStallDetectionTimer();
    ASSERT_EQ(dataConnectionMonitor->noRecvPackets_, 20);
}

/**
 * @tc.number   DataConnectionMonitor_OnStallDetectionTimer_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_OnStallDetectionTimer_002, TestSize.Level1)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->stallDetectionEnabled_ = true;
    dataConnectionMonitor->noRecvPackets_ = 11;
    dataConnectionMonitor->OnStallDetectionTimer();
    dataConnectionMonitor->noRecvPackets_ = 10;
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_REQUEST_CONTEXT_LIST;
    dataConnectionMonitor->OnStallDetectionTimer();
    dataConnectionMonitor->noRecvPackets_ = 0;
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_RADIO_STATUS_RESTART;
    dataConnectionMonitor->OnStallDetectionTimer();
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_RADIO_STATUS_RESTART;
    dataConnectionMonitor->HandleRecovery();
    ASSERT_EQ(dataConnectionMonitor->dataRecoveryState_, RecoveryState::STATE_REQUEST_CONTEXT_LIST);
}

/**
 * @tc.number   DataConnectionMonitor_HandleRecovery_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_HandleRecovery_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_REQUEST_CONTEXT_LIST;
    dataConnectionMonitor->HandleRecovery();
    ASSERT_EQ(dataConnectionMonitor->dataRecoveryState_, RecoveryState::STATE_CLEANUP_CONNECTIONS);
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_REREGISTER_NETWORK;
    dataConnectionMonitor->HandleRecovery();
    ASSERT_EQ(dataConnectionMonitor->dataRecoveryState_, RecoveryState::STATE_RADIO_STATUS_RESTART);
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_RADIO_STATUS_RESTART;
    dataConnectionMonitor->HandleRecovery();
    ASSERT_EQ(dataConnectionMonitor->dataRecoveryState_, RecoveryState::STATE_REQUEST_CONTEXT_LIST);
}

/**
 * @tc.number   DataConnectionMonitor_HandleRecovery_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_HandleRecovery_002, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->dataRecoveryState_ = RecoveryState::STATE_REREGISTER_NETWORK;
    EXPECT_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillRepeatedly([](int32_t slotId, int32_t &psRadioTech) {
            psRadioTech = static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_NR);
            return 0;
        });
    EXPECT_CALL(*mockNetworkSearchManager, IsGsm(_, _))
        .WillRepeatedly([](int32_t slotId, bool &isGsm) {
            isGsm = true;
            return 0;
        });
    dataConnectionMonitor->HandleRecovery();
    ASSERT_EQ(dataConnectionMonitor->dataRecoveryState_, RecoveryState::STATE_RADIO_STATUS_RESTART);
}

/**
 * @tc.number   DataConnectionMonitor_EndNetStatistics_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_EndNetStatistics_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_DOWN;
    dataConnectionMonitor->EndNetStatistics();
    ASSERT_EQ(dataConnectionMonitor->dataFlowType_, CellDataFlowType::DATA_FLOW_TYPE_NONE);
}

/**
 * @tc.number   DataConnectionMonitor_UpdateNetTrafficState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_UpdateNetTrafficState_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    dataConnectionMonitor->updateNetStat_ = true;
    dataConnectionMonitor->UpdateNetTrafficState();
    std::shared_ptr<PreferredNetworkTypeInfo> preferredTypeInfo = std::make_shared<PreferredNetworkTypeInfo>();
    auto event = AppExecFwk::InnerEvent::Get(0, preferredTypeInfo);
    dataConnectionMonitor->SetPreferredNetworkPara(event);
    ASSERT_EQ(dataConnectionMonitor->dataFlowType_, CellDataFlowType::DATA_FLOW_TYPE_NONE);
}

/**
 * @tc.number   DataConnectionMonitor_UpdateDataFlowType_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_UpdateDataFlowType_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    // UpdatePacketData 依赖 netmanager 真实查询蜂窝 iface（GetIfaceName 走 GetNetIdByIdentifier/
    // GetAllNets/GetConnectionProperties 真实 IPC）；无数据连接的环境 iface 为空会直接早退并
    // 保持 DOWN。先探测环境可用性，无蜂窝 iface 时跳过（环境自适应约定，同 rdb 用例）。
    if (!dataConnectionMonitor->trafficManager_->UpdatePacketData()) {
        GTEST_SKIP() << "no cellular iface on this device (netmanager query failed), skip";
    }
    // 有 iface 时取探测到的真实收发包计数作为 previous，使 delta=(0,0) 确定性落在 NONE
    // 分支（原用例硬编码 previous=200/100，仅在 iface 存在且空闲的 CI 环境经 counter-reset
    // 路径偶然成立）。
    int64_t sendPackets = 0;
    int64_t recvPackets = 0;
    dataConnectionMonitor->trafficManager_->GetPacketData(sendPackets, recvPackets);
    dataConnectionMonitor->trafficManager_->sendPackets_ = sendPackets;
    dataConnectionMonitor->trafficManager_->recvPackets_ = recvPackets;
    dataConnectionMonitor->dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_DOWN;
    dataConnectionMonitor->UpdateDataFlowType();
    ASSERT_EQ(static_cast<int32_t>(dataConnectionMonitor->dataFlowType_),
        static_cast<int32_t>(CellDataFlowType::DATA_FLOW_TYPE_NONE));
}

/**
 * @tc.number   DataConnectionMonitor_SetDataFlowType_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DataConnectionMonitor_SetDataFlowType_001, TestSize.Level0)
{
    std::shared_ptr<DataConnectionMonitor> dataConnectionMonitor = std::make_shared<DataConnectionMonitor>(0);
    auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RUN_MONITOR_TASK);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_DATA_CALL_LIST_CHANGED);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_GET_PREFERRED_NETWORK_MODE);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_SET_PREFERRED_NETWORK_MODE);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_OFF);
    dataConnectionMonitor->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_ON);
    dataConnectionMonitor->ProcessEvent(event);
    dataConnectionMonitor->dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_NONE;
    dataConnectionMonitor->SetDataFlowType(CellDataFlowType::DATA_FLOW_TYPE_DOWN);
    ASSERT_EQ(static_cast<int32_t>(dataConnectionMonitor->dataFlowType_),
        static_cast<int32_t>(CellDataFlowType::DATA_FLOW_TYPE_DOWN));
}

/**
 * @tc.number   CellularDataController_SetIntelligenceSwitchEnable_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, CellularDataController_SetIntelligenceSwitchEnable_001, TestSize.Level0)
{
    std::shared_ptr<CellularDataController> cellularDataController = std::make_shared<CellularDataController>(0);
    cellularDataController->cellularDataHandler_ = nullptr;
    int32_t result = cellularDataController->SetIntelligenceSwitchEnable(true);
    ASSERT_EQ(result, TELEPHONY_ERR_LOCAL_PTR_NULL);
}

/**
 * @tc.number   CellularDataController_SetIntelligenceSwitchEnable_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, CellularDataController_SetIntelligenceSwitchEnable_002, TestSize.Level0)
{
    std::shared_ptr<CellularDataController> cellularDataController = std::make_shared<CellularDataController>(0);
    cellularDataController->Init();
    int32_t result = cellularDataController->SetIntelligenceSwitchEnable(false);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.number   CellularDataController_OnAddSystemAbility_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, CellularDataController_OnAddSystemAbility_001, TestSize.Level0)
{
    std::shared_ptr<CellularDataController> cellularDataController = std::make_shared<CellularDataController>(0);
    cellularDataController->systemAbilityListener_ =
        new (std::nothrow) CellularDataController::SystemAbilityStatusChangeListener(0, nullptr);
    if (cellularDataController->systemAbilityListener_ != nullptr) {
        cellularDataController->systemAbilityListener_->OnRemoveSystemAbility(
            COMM_NET_CONN_MANAGER_SYS_ABILITY_ID, "");
        cellularDataController->systemAbilityListener_->OnRemoveSystemAbility(
            COMM_NET_POLICY_MANAGER_SYS_ABILITY_ID, "");
        cellularDataController->systemAbilityListener_->OnRemoveSystemAbility(
            DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, "");
        cellularDataController->systemAbilityListener_->OnRemoveSystemAbility(-1, "");
    }
    ASSERT_EQ(cellularDataController->cellularDataHandler_, nullptr);
}


/**
 * @tc.number   EstablishAllApnsIfConnectableTest001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, EstablishAllApnsIfConnectableTest001, TestSize.Level0)
{
    int32_t slotId = -1;
    EXPECT_EQ(service->EstablishAllApnsIfConnectable(slotId), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   EstablishAllApnsIfConnectableTest002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, EstablishAllApnsIfConnectableTest002, TestSize.Level0)
{
    DataAccessToken token;
    int32_t slotId = -1;
    EXPECT_NE(service->EstablishAllApnsIfConnectable(slotId), TELEPHONY_ERR_FAIL);
}

/**
 * @tc.number   GetActReportInfoTest001
 * @tc.name     test GetDefaultActReportInfo and GetInternalActReportInfo
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetActReportInfoTest001, TestSize.Level0)
{
    ApnActivateReportInfoIpc infoIpc;
    int32_t slotId = -1;
    EXPECT_EQ(service->GetDefaultActReportInfo(slotId, infoIpc), TELEPHONY_ERR_PERMISSION_ERR);
    EXPECT_EQ(service->GetInternalActReportInfo(slotId, infoIpc), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetActReportInfoTest002
 * @tc.name     test GetDefaultActReportInfo and GetInternalActReportInfo
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetActReportInfoTest002, TestSize.Level0)
{
    DataAccessToken token;
    ApnActivateReportInfoIpc infoIpc;
    int32_t slotId = -1;
    EXPECT_EQ(service->GetDefaultActReportInfo(slotId, infoIpc), CELLULAR_DATA_INVALID_PARAM);
    EXPECT_EQ(service->GetInternalActReportInfo(slotId, infoIpc), CELLULAR_DATA_INVALID_PARAM);
}

/**
 * @tc.number   ApnInfoTest001
 * @tc.name     test QueryApnIds, SetPreferApn and QueryAllApnInfo
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, ApnInfoTest001, TestSize.Level0)
{
    ApnInfo apnInfo;
    std::vector<uint32_t> apnIdList;
    std::vector<ApnInfo> allApnInfoList;
    int32_t apnId = 1;
    EXPECT_EQ(service->QueryApnIds(apnInfo, apnIdList), TELEPHONY_ERR_PERMISSION_ERR);
    EXPECT_EQ(service->SetPreferApn(apnId), TELEPHONY_ERR_PERMISSION_ERR);
    EXPECT_EQ(service->QueryAllApnInfo(allApnInfoList), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   ApnInfoTest002
 * @tc.name     test QueryApnIds, SetPreferApn and QueryAllApnInfo
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, ApnInfoTest002, TestSize.Level0)
{
    DataAccessToken token;
    ApnInfo apnInfo;
    std::vector<uint32_t> apnIdList;
    std::vector<ApnInfo> allApnInfoList;
    int32_t apnId = 1;
    EXPECT_EQ(service->QueryApnIds(apnInfo, apnIdList), 0);
    EXPECT_EQ(service->SetPreferApn(apnId), -1);
    EXPECT_EQ(service->QueryAllApnInfo(allApnInfoList), 0);
}

/**
 * @tc.number   GetActiveApnName001
 * @tc.name     test GetActiveApnName
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetActiveApnName001, TestSize.Level0)
{
    std::string apnName;
    EXPECT_EQ(service->GetActiveApnName(apnName), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetActiveApnName002
 * @tc.name     test GetActiveApnName
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetActiveApnName002, TestSize.Level0)
{
    DataAccessToken token;
    std::string apnName;
    EXPECT_EQ(service->GetActiveApnName(apnName), 0);
}

/**
 * @tc.number   ReleaseNet_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, ReleaseNet_001, TestSize.Level1)
{
    NetRequest request;
    request.ident = "1";
    ASSERT_EQ(CELLULAR_DATA_INVALID_PARAM, service->RequestNet(request));
    ASSERT_EQ(CELLULAR_DATA_INVALID_PARAM, service->ReleaseNet(request));
}

/**
 * @tc.number   GetDataConnIpType_PermissionTest_001
 * @tc.name     test GetDataConnIpType without permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetDataConnIpType_PermissionTest_001, TestSize.Level0)
{
    std::string ipType;
    EXPECT_EQ(service->GetDataConnIpType(DEFAULT_SIM_SLOT_ID, ipType), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetDataConnIpType_PermissionTest_002
 * @tc.name     test GetDataConnIpType with permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetDataConnIpType_PermissionTest_002, TestSize.Level0)
{
    DataAccessToken token;
    std::string ipType;
    EXPECT_NE(service->GetDataConnIpType(DEFAULT_SIM_SLOT_ID, ipType), TELEPHONY_ERR_FAIL);
}

/**
 * @tc.number   GetDataRecoveryState_PermissionTest_001
 * @tc.name     test GetDataRecoveryState without permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetDataRecoveryState_PermissionTest_001, TestSize.Level0)
{
    int32_t state = 0;
    EXPECT_EQ(service->GetDataRecoveryState(state), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetDataRecoveryState_PermissionTest_002
 * @tc.name     test GetDataRecoveryState with permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetDataRecoveryState_PermissionTest_002, TestSize.Level0)
{
    DataAccessToken token;
    int32_t state = 0;
    EXPECT_NE(service->GetDataRecoveryState(state), TELEPHONY_ERR_FAIL);
}

/**
 * @tc.number   GetApnState_PermissionTest_001
 * @tc.name     test GetApnState without permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetApnState_PermissionTest_001, TestSize.Level0)
{
    int32_t state = 0;
    EXPECT_EQ(service->GetApnState(DEFAULT_SIM_SLOT_ID, std::string(), state), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetApnState_PermissionTest_002
 * @tc.name     test GetApnState with permission
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetApnState_PermissionTest_002, TestSize.Level0)
{
    DataAccessToken token;
    int32_t state = 0;
    EXPECT_NE(service->GetApnState(DEFAULT_SIM_SLOT_ID, std::string(), state), TELEPHONY_ERR_FAIL);
}

bool g_mockGetBundleNameByUid = true;
std::string g_mockBundleName = "";
bool g_callbackInvoked = false;

int32_t TestSendCellularDataSlotChangeInfo(const char* bundleName, int32_t pid, int32_t slotId)
{
    g_callbackInvoked = true;
    return 0;
}

bool TelephonyPermission::GetBundleNameByUid(int32_t uid, std::string &bundleName)
{
    (void)uid;
    if (g_mockGetBundleNameByUid) {
        bundleName = g_mockBundleName;
        return true;
    }
    return false;
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_SendSlotChangeInfo_Branch1_CallbackNullptr
 * @tc.name     test SetDefaultCellularDataSlotId sendCellularDataSlotChangeInfo_ is nullptr
 * @tc.desc     Branch test - when sendCellularDataSlotChangeInfo_ is nullptr, skip callback
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_SendSlotChangeInfo_CallbackNullptr, TestSize.Level0)
{
    g_mockGetBundleNameByUid = true;
    g_mockBundleName = "test.bundle.name";
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    g_callbackInvoked = false;
    int32_t slotId = 0;
    service->SendSlotChangeInfoToChr(slotId);
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    EXPECT_FALSE(g_callbackInvoked);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_SendSlotChangeInfo_Branch2_BundleNameEmpty
 * @tc.name     test SetDefaultCellularDataSlotId bundleName is empty, use pid+uid
 * @tc.desc     Branch test - when bundleName is empty, construct from pid and uid
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_SendSlotChangeInfo_BundleNameEmpty, TestSize.Level0)
{
    g_mockGetBundleNameByUid = true;
    g_mockBundleName = "";  // empty bundleName
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = TestSendCellularDataSlotChangeInfo;
    g_callbackInvoked = false;
    int32_t slotId = 0;
    service->SendSlotChangeInfoToChr(slotId);
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    EXPECT_TRUE(g_callbackInvoked);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_SendSlotChangeInfo_Branch3_BundleNameNotEmpty
 * @tc.name     test SetDefaultCellularDataSlotId bundleName is not empty, use as-is
 * @tc.desc     Branch test - when bundleName is not empty, pass it directly to callback
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_SendSlotChangeInfo_BundleNameNotEmpty, TestSize.Level0)
{
    g_mockGetBundleNameByUid = true;
    g_mockBundleName = "com.test.bundle";
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = TestSendCellularDataSlotChangeInfo;
    g_callbackInvoked = false;
    int32_t slotId = 0;
    service->SendSlotChangeInfoToChr(slotId);
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    EXPECT_TRUE(g_callbackInvoked);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_SendSlotChangeInfo_Branch4_GetBundleNameFailed
 * @tc.name     test SetDefaultCellularDataSlotId GetBundleNameByUid returns false
 * @tc.desc     Branch test - when GetBundleNameByUid fails, bundleName remains empty, use pid+uid
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_SendSlotChangeInfo_GetBundleNameFailed, TestSize.Level0)
{
    g_mockGetBundleNameByUid = false;  // GetBundleNameByUid returns false
    g_mockBundleName = "";
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = TestSendCellularDataSlotChangeInfo;
    g_callbackInvoked = false;
    int32_t slotId = 0;
    service->SendSlotChangeInfoToChr(slotId);
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    EXPECT_TRUE(g_callbackInvoked);
}

/**
 * @tc.number   EnableCellularData_003
 * @tc.name     non-system app / no controller / EDM force_open / EDM disallow / no policy
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, EnableCellularData_003, TestSize.Level1)
{
    // A) 非系统应用（CheckCallerIsSystemApp mock 默认拒绝）→ ILLEGAL_USE_OF_SYSTEM_API
    EXPECT_EQ(service->EnableCellularData(false), TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API);
    // B) 系统应用 + SET_TELEPHONY_STATE 放行 + 无 controller（本用例先于 InitModule 类用例执行）→ LOCAL_PTR_NULL
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    EXPECT_EQ(service->EnableCellularData(false), TELEPHONY_ERR_LOCAL_PTR_NULL);
    // C) InitModule 建 controller（AddNetSupplier 为本地容器操作）+ EDM force_open 禁关闭 → POLICY_DISABLED
    service->isInitSuccess_ = true;
    service->InitModule();
    std::string origEdm = system::GetParameter("persist.edm.mobile_data_policy", "");
    system::SetParameter("persist.edm.mobile_data_policy", "force_open");
    EXPECT_EQ(service->EnableCellularData(false), TELEPHONY_ERR_POLICY_DISABLED);
    // D) EDM disallow 禁开启 → POLICY_DISABLED
    system::SetParameter("persist.edm.mobile_data_policy", "disallow");
    EXPECT_EQ(service->EnableCellularData(true), TELEPHONY_ERR_POLICY_DISABLED);
    // E) 无策略 → controller->SetCellularDataEnable（handler 未 Init → LOCAL_PTR_NULL）
    system::SetParameter("persist.edm.mobile_data_policy", origEdm);
    EXPECT_EQ(service->EnableCellularData(false), TELEPHONY_ERR_LOCAL_PTR_NULL);
}

/**
 * @tc.number   InitModule_001
 * @tc.name     init module creates controllers per slot with TSTS disabled
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, InitModule_001, TestSize.Level1)
{
    service->isInitSuccess_ = true;
    std::string origTsts = system::GetParameter(PERSIST_TSTS_MODE, TSTS_MODE_DEFAULT_VALUE);
    system::SetParameter(PERSIST_TSTS_MODE, "0");
    service->InitModule();
    system::SetParameter(PERSIST_TSTS_MODE, origTsts);
    // slot0 恒被初始化（SIM_SLOT_COUNT_MD 为系统参数，设备值不定，只断言确定项）
    EXPECT_NE(service->GetCellularDataController(DEFAULT_SIM_SLOT_ID), nullptr);
}

/**
 * @tc.number   InitModule_TSTS_002
 * @tc.name     TSTS mode additionally initializes CELLDATA_SLOT_ID_3 controller
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, InitModule_TSTS_002, TestSize.Level1)
{
    service->isInitSuccess_ = true;
    std::string origTsts = system::GetParameter(PERSIST_TSTS_MODE, TSTS_MODE_DEFAULT_VALUE);
    system::SetParameter(PERSIST_TSTS_MODE, "1");
    service->InitModule();
    system::SetParameter(PERSIST_TSTS_MODE, origTsts);
    // TSTS 分支在主循环外显式 AddNetSupplier(CELLDATA_SLOT_ID_3)，与 slot count 参数无关
    EXPECT_NE(service->GetCellularDataController(CELLDATA_SLOT_ID_3), nullptr);
}

/**
 * @tc.number   DumpWithController_001
 * @tc.name     status/flow dumps take the with-controller path
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, DumpWithController_001, TestSize.Level1)
{
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(0));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    service->InitModule();
    // controller 存在但 handler 未 Init → GetCellularDataState 返回 FAILED、flow 返回 NONE，dump 输出状态行
    EXPECT_NE(service->GetStateMachineCurrentStatusDump(), "");
    EXPECT_NE(service->GetFlowDataInfoDump(), "");
}

/**
 * @tc.number   GetActiveApnName_003
 * @tc.name     permission denied / granted with non-connected controller
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataServiceTest, GetActiveApnName_003, TestSize.Level1)
{
    // A) GET_NETWORK_INFO 默认拒绝 → PERMISSION_ERR
    std::string apnName = "init";
    EXPECT_EQ(service->GetActiveApnName(apnName), TELEPHONY_ERR_PERMISSION_ERR);
    // B) 放行 + 默认卡 0 controller 存在（状态非 CONNECTED）→ 0 且 apnName 被清空
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(0));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    service->InitModule();
    EXPECT_EQ(service->GetActiveApnName(apnName), 0);
    EXPECT_EQ(apnName, "");
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_EarlyExit_V1
 * @tc.name     test SetDefaultCellularDataSlotId with permission denied
 * @tc.desc     系统应用校验通过但 SET_TELEPHONY_STATE 权限拒绝 → PERMISSION_ERR
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_EarlyExit_V1, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    // CheckPermission 未配置 → 默认拒绝
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_EarlyExit_V2
 * @tc.name     test SetDefaultCellularDataSlotId without sim card
 * @tc.desc     权限放行 + 无 simManager（HasSimCard 失败）→ NO_SIM_CARD
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_EarlyExit_V2, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_NO_SIM_CARD);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_EarlyExit_V3
 * @tc.name     test SetDefaultCellularDataSlotId with inactive sim
 * @tc.desc     有 SIM 但未激活 → SLOTID_INVALID
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_EarlyExit_V3, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, HasSimCard(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(true), Return(0)));
    ON_CALL(*mockSim, IsSimActive(_)).WillByDefault(Return(false));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_SLOTID_INVALID);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_EarlyExit_V4
 * @tc.name     test SetDefaultCellularDataSlotId with former slot negative and set failure
 * @tc.desc     旧默认卡为负（仅日志）+ SetDefaultCellularDataSlotId 失败 → 透传失败码
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_EarlyExit_V4, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, HasSimCard(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(true), Return(0)));
    ON_CALL(*mockSim, IsSimActive(_)).WillByDefault(Return(true));
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(-1));
    ON_CALL(*mockSim, SetDefaultCellularDataSlotId(_)).WillByDefault(Return(TELEPHONY_ERR_SLOTID_INVALID));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    // 屏蔽 CHR 上报钩子，避免真机 vendor 扩展真实调用
    TELEPHONY_EXT_WRAPPER.sendCellularDataSlotChangeInfo_ = nullptr;
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_SLOTID_INVALID);
}

/**
 * @tc.number   GetCellularDataState_AllowNoController
 * @tc.name     test GetCellularDataState with permission allowed but no controller
 * @tc.desc     GET_NETWORK_INFO 放行 + 默认卡指向无 controller 的 slot → INVALID_PARAM
 */
HWTEST_F(CellularDataServiceTest, GetCellularDataState_AllowNoController, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    int32_t state = -1;
    EXPECT_EQ(service->GetCellularDataState(state), CELLULAR_DATA_INVALID_PARAM);
}

/**
 * @tc.number   GetCellularDataFlowType_AllowNoController
 * @tc.name     test GetCellularDataFlowType with permission allowed but no controller
 * @tc.desc     GET_NETWORK_INFO 放行 + 默认卡指向无 controller 的 slot → INVALID_PARAM
 */
HWTEST_F(CellularDataServiceTest, GetCellularDataFlowType_AllowNoController, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    int32_t type = -1;
    EXPECT_EQ(service->GetCellularDataFlowType(type), CELLULAR_DATA_INVALID_PARAM);
}

/**
 * @tc.number   EstablishAllApnsIfConnectable_EarlyExit
 * @tc.name     test EstablishAllApnsIfConnectable early exits
 * @tc.desc     权限放行后：无 controller → INVALID_PARAM；有 controller 无 SIM → NO_SIM_CARD；
 *              有 SIM 未激活 → SLOTID_INVALID
 */
HWTEST_F(CellularDataServiceTest, EstablishAllApnsIfConnectable_EarlyExit, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    // 服务初始化标记置位（不依赖 InitModule 类用例的执行顺序），GetCellularDataController 才会查表
    service->isInitSuccess_ = true;
    // 1) slot 99 无 controller → INVALID_PARAM
    EXPECT_EQ(service->EstablishAllApnsIfConnectable(99), CELLULAR_DATA_INVALID_PARAM);
    // 2) slot 99 注入 controller + 无 SIM（simManager 为空 → HasSimCard 失败）→ NO_SIM_CARD
    auto controller = std::make_shared<CellularDataController>(99);
    service->cellularDataControllers_[99] = controller;
    EXPECT_EQ(service->EstablishAllApnsIfConnectable(99), TELEPHONY_ERR_NO_SIM_CARD);
    // 3) 有 SIM 但未激活 → SLOTID_INVALID
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, HasSimCard(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(true), Return(0)));
    ON_CALL(*mockSim, IsSimActive(_)).WillByDefault(Return(false));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    EXPECT_EQ(service->EstablishAllApnsIfConnectable(99), TELEPHONY_ERR_SLOTID_INVALID);
    // 还原：排空事件队列后移除注入的 controller
    TestAsyncUtils::DrainTelEventHandler(controller.get());
    service->cellularDataControllers_.erase(99);
}

/**
 * @tc.number   GetActiveApnName_NoController
 * @tc.name     test GetActiveApnName with permission allowed but no controller
 * @tc.desc     GET_NETWORK_INFO 放行 + 默认卡指向无 controller 的 slot → 返回 0 且 apnName 为空
 */
HWTEST_F(CellularDataServiceTest, GetActiveApnName_NoController, TestSize.Level0)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    std::string apnName = "preset";
    EXPECT_EQ(service->GetActiveApnName(apnName), 0);
    EXPECT_EQ(apnName, "");
}

/**
 * @tc.number   GetCellularDataState_Suspended_001
 * @tc.name     test GetCellularDataState GSM_AND_CALLING suspended matrix
 * @tc.desc     controller(99)+注入 handler（disconnectionReason_/lastCallState_ 直改，
 *              GetPsRadioTech mock 控制制式）覆盖 REASON_GSM_AND_CALLING_ONLY && IsRestrictedMode
 *              判定的各方向：非 GSM 原因 / GSM 原因但非受限 / GSM 原因且受限（SUSPENDED）。
 */
HWTEST_F(CellularDataServiceTest, GetCellularDataState_Suspended_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);

    auto controller = std::make_shared<CellularDataController>(99);
    auto handler = TestAsyncUtils::MakeDrainedHandler(99);
    controller->cellularDataHandler_ = handler;
    service->cellularDataControllers_[99] = controller;

    // A) 普通原因（REASON_NORMAL）→ 不进受限判定 → SUCCESS
    int32_t state = -1;
    handler->disconnectionReason_ = DisConnectionReason::REASON_NORMAL;
    EXPECT_EQ(service->GetCellularDataState(state), TELEPHONY_ERR_SUCCESS);
    // B) GSM 原因但制式非 GSM（LTE）→ IsRestrictedMode false → 正常状态
    handler->disconnectionReason_ = DisConnectionReason::REASON_GSM_AND_CALLING_ONLY;
    ON_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_LTE)), Return(0)));
    EXPECT_EQ(service->GetCellularDataState(state), TELEPHONY_ERR_SUCCESS);
    // C) GSM 原因 + GSM 制式 + 通话中 → 受限 → SUSPENDED
    ON_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_GSM)), Return(0)));
    handler->lastCallState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_ACTIVE);
    EXPECT_EQ(service->GetCellularDataState(state), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(state, static_cast<int32_t>(DataConnectionStatus::DATA_STATE_SUSPENDED));
    // D) GSM 制式但非通话态（IDLE）→ inCall false → 正常状态
    handler->lastCallState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_IDLE);
    EXPECT_EQ(service->GetCellularDataState(state), TELEPHONY_ERR_SUCCESS);

    service->cellularDataControllers_.erase(99);
}

/**
 * @tc.number   GetCellularDataFlowType_Dormant_001
 * @tc.name     test GetCellularDataFlowType GSM_AND_CALLING dormant matrix
 * @tc.desc     同 Suspended 矩阵：受限 → DATA_FLOW_TYPE_DORMANT；普通原因 → 走流类型查询。
 */
HWTEST_F(CellularDataServiceTest, GetCellularDataFlowType_Dormant_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);

    auto controller = std::make_shared<CellularDataController>(99);
    auto handler = TestAsyncUtils::MakeDrainedHandler(99);
    controller->cellularDataHandler_ = handler;
    service->cellularDataControllers_[99] = controller;

    // A) GSM 原因 + GSM 制式 + 通话中 → 受限 → 返回 DORMANT（该函数返回流类型值本身）
    handler->disconnectionReason_ = DisConnectionReason::REASON_GSM_AND_CALLING_ONLY;
    ON_CALL(*mockNetworkSearchManager, GetPsRadioTech(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(static_cast<int32_t>(RadioTech::RADIO_TECHNOLOGY_GSM)), Return(0)));
    handler->lastCallState_ = static_cast<int32_t>(TelCallStatus::CALL_STATUS_ACTIVE);
    int32_t flowType = -1;
    EXPECT_EQ(service->GetCellularDataFlowType(flowType),
        static_cast<int32_t>(CellDataFlowType::DATA_FLOW_TYPE_DORMANT));
    // B) 普通原因 → 走 controller->GetCellularDataFlowType()（handler 未 Init → 默认流类型）
    handler->disconnectionReason_ = DisConnectionReason::REASON_NORMAL;
    EXPECT_GE(service->GetCellularDataFlowType(flowType), 0);

    service->cellularDataControllers_.erase(99);
}

namespace {
// 最小 rdb stub：CreateDataAbilityHelper 返回空 → QueryPreferApn false（对齐 apn_manager_test 模式）
class SvcRdbStub : public CellularDataRdbHelper {
public:
    SvcRdbStub() = default;
    ~SvcRdbStub() = default;
    std::shared_ptr<DataShare::DataShareHelper> CreateDataAbilityHelper(const int waitTime) override
    {
        return nullptr;
    }
};
} // namespace

/**
 * @tc.number   GetActiveApnName_ConnectedRdbNull_001
 * @tc.name     test GetActiveApnName with connected default apn and null rdb helper
 * @tc.desc     注入 CONNECTED default holder（handler Init 后取 default holder 置态）+
 *              rdb 单例注入空 helper stub → 走到 QueryPreferApn 失败早退（L979 真弧）。
 */
HWTEST_F(CellularDataServiceTest, GetActiveApnName_ConnectedRdbNull_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_NETWORK_INFO))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);

    auto controller = std::make_shared<CellularDataController>(99);
    auto handler = TestAsyncUtils::MakeDrainedHandler(99);
    handler->Init();
    ASSERT_NE(handler->apnManager_, nullptr);
    sptr<ApnHolder> defaultHolder = handler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(defaultHolder, nullptr);
    defaultHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    controller->cellularDataHandler_ = handler;
    service->cellularDataControllers_[99] = controller;

    // rdb 单例注入空 helper stub（ScopedSetter 守卫，用例结束还原）
    auto stub = std::make_shared<SvcRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);

    std::string apnName = "preset";
    EXPECT_EQ(service->GetActiveApnName(apnName), 0);
    EXPECT_EQ(apnName, "");

    service->cellularDataControllers_.erase(99);
}

/**
 * @tc.number   GetDataConnIpType_IsNeedDoRecovery_EarlyExit_001
 * @tc.name     test GetDataConnIpType / IsNeedDoRecovery early exits
 * @tc.desc     权限拒绝 → PERMISSION_ERR；权限放行 + slot 99 无 controller → INVALID_PARAM。
 */
HWTEST_F(CellularDataServiceTest, GetDataConnIpType_IsNeedDoRecovery_EarlyExit_001, TestSize.Level1)
{
    std::string ipType = "preset";
    // A) GetDataConnIpType 权限拒绝
    EXPECT_EQ(service->GetDataConnIpType(99, ipType), TELEPHONY_ERR_PERMISSION_ERR);
    // B) 放行 + 无 controller → INVALID_PARAM
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    EXPECT_EQ(service->GetDataConnIpType(99, ipType), CELLULAR_DATA_INVALID_PARAM);
    // C) IsNeedDoRecovery 权限拒绝（SET_TELEPHONY_STATE）
    bool needRecovery = true;
    EXPECT_EQ(service->IsNeedDoRecovery(99, needRecovery), TELEPHONY_ERR_PERMISSION_ERR);
    // D) 放行 + 无 controller → INVALID_PARAM
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    EXPECT_EQ(service->IsNeedDoRecovery(99, needRecovery), CELLULAR_DATA_INVALID_PARAM);
}

/**
 * @tc.number   RequestNetReleaseNet_IdentVariants_001
 * @tc.name     test RequestNet / ReleaseNet ident parsing variants
 * @tc.desc     ident 长度不足 / 前缀后非十进制 / 十进制溢出（ConvertStrToInt 失败）/
 *              合法 simId 但无对应 controller（GetSlotId→-1）→ 全部 INVALID_PARAM。
 */
HWTEST_F(CellularDataServiceTest, RequestNetReleaseNet_IdentVariants_001, TestSize.Level1)
{
    NetRequest request;
    request.capability = 0;
    // A) ident 长度不足（< strlen("simId")）
    request.ident = "ab";
    EXPECT_EQ(service->RequestNet(request), CELLULAR_DATA_INVALID_PARAM);
    EXPECT_EQ(service->ReleaseNet(request), CELLULAR_DATA_INVALID_PARAM);
    // B) 前缀后非十进制
    request.ident = std::string(IDENT_PREFIX) + "abc";
    EXPECT_EQ(service->RequestNet(request), CELLULAR_DATA_INVALID_PARAM);
    EXPECT_EQ(service->ReleaseNet(request), CELLULAR_DATA_INVALID_PARAM);
    // C) 十进制但超 int32 范围 → ConvertStrToInt 失败
    request.ident = std::string(IDENT_PREFIX) + "99999999999";
    EXPECT_EQ(service->RequestNet(request), CELLULAR_DATA_INVALID_PARAM);
    EXPECT_EQ(service->ReleaseNet(request), CELLULAR_DATA_INVALID_PARAM);
    // D) 合法 simId → NetAgent 无注册（GetSlotId→-1）→ 无 controller → INVALID_PARAM
    request.ident = std::string(IDENT_PREFIX) + "5";
    EXPECT_EQ(service->RequestNet(request), CELLULAR_DATA_INVALID_PARAM);
    EXPECT_EQ(service->ReleaseNet(request), CELLULAR_DATA_INVALID_PARAM);
}

/**
 * @tc.number   DispatchEvent_NoController_001
 * @tc.name     test DispatchEvent with no controller
 * @tc.desc     slot 99 无 controller → 早退返回。
 */
HWTEST_F(CellularDataServiceTest, DispatchEvent_NoController_001, TestSize.Level1)
{
    service->isInitSuccess_ = true;
    auto event = AppExecFwk::InnerEvent::Get(0);
    service->DispatchEvent(99, event);
    EXPECT_EQ(service->GetCellularDataController(99), nullptr);
}

/**
 * @tc.number   GetCellularDataSupplierId_CapabilityBounds_001
 * @tc.name     test GetCellularDataSupplierId capability bounds
 * @tc.desc     capability 低于 MMS / 高于 SNSSAI6 → INVALID_PARAM；范围内 → GetSupplierId → SUCCESS。
 */
HWTEST_F(CellularDataServiceTest, GetCellularDataSupplierId_CapabilityBounds_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::GET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    uint32_t supplierId = 0;
    // A) capability 过小
    EXPECT_EQ(service->GetCellularDataSupplierId(0, static_cast<uint64_t>(NetCap::NET_CAPABILITY_MMS) - 1,
        supplierId), CELLULAR_DATA_INVALID_PARAM);
    // B) capability 过大
    EXPECT_EQ(service->GetCellularDataSupplierId(0, static_cast<uint64_t>(NetCap::NET_CAPABILITY_SNSSAI6) + 1,
        supplierId), CELLULAR_DATA_INVALID_PARAM);
    // C) 范围内（MMS）→ 走 GetSupplierId → SUCCESS
    EXPECT_EQ(service->GetCellularDataSupplierId(0, static_cast<uint64_t>(NetCap::NET_CAPABILITY_MMS), supplierId),
        TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   CorrectNetSupplierNoAvailable_001
 * @tc.name     test CorrectNetSupplierNoAvailable controller null and connected state
 * @tc.desc     无 controller → INVALID_PARAM；controller@99 + default holder CONNECTED → FAIL。
 */
HWTEST_F(CellularDataServiceTest, CorrectNetSupplierNoAvailable_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    // A) 无 controller → INVALID_PARAM
    EXPECT_EQ(service->CorrectNetSupplierNoAvailable(99), CELLULAR_DATA_INVALID_PARAM);
    // B) controller@99 + default CONNECTED → FAIL（不再下探 UpdateNetworkInfo）
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(99));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto controller = std::make_shared<CellularDataController>(99);
    auto handler = TestAsyncUtils::MakeDrainedHandler(99);
    handler->Init();
    sptr<ApnHolder> defaultHolder = handler->apnManager_->GetApnHolder(DATA_CONTEXT_ROLE_DEFAULT);
    ASSERT_NE(defaultHolder, nullptr);
    defaultHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    controller->cellularDataHandler_ = handler;
    service->cellularDataControllers_[99] = controller;
    EXPECT_EQ(service->CorrectNetSupplierNoAvailable(99), TELEPHONY_ERR_FAIL);
    service->cellularDataControllers_.erase(99);
}

/**
 * @tc.number   EstablishAllApnsIfConnectable_SimInactive_001
 * @tc.name     test EstablishAllApnsIfConnectable with inactive sim
 * @tc.desc     controller@99 + HasSimCard true + IsSimActive false → SLOTID_INVALID。
 */
HWTEST_F(CellularDataServiceTest, EstablishAllApnsIfConnectable_SimInactive_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    service->isInitSuccess_ = true;
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, HasSimCard(_, _)).WillByDefault(DoAll(SetArgReferee<1>(true), Return(0)));
    ON_CALL(*mockSim, IsSimActive(_)).WillByDefault(Return(false));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    auto controller = std::make_shared<CellularDataController>(99);
    auto handler = TestAsyncUtils::MakeDrainedHandler(99);
    controller->cellularDataHandler_ = handler;
    service->cellularDataControllers_[99] = controller;
    EXPECT_EQ(service->EstablishAllApnsIfConnectable(99), TELEPHONY_ERR_SLOTID_INVALID);
    service->cellularDataControllers_.erase(99);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_FullChain_001
 * @tc.name     test SetDefaultCellularDataSlotId full mocked chain
 * @tc.desc     系统应用+权限放行 + HasSimCard/IsSimActive true + 旧卡-1（L503）+
 *              SetDefault 返回错误（L508）/成功（L511）两变体。
 */
HWTEST_F(CellularDataServiceTest, SetDefaultCellularDataSlotId_FullChain_001, TestSize.Level1)
{
    auto permissionMock = MockTelephonyPermission::GetOrCreateMockTelephonyPermission();
    ON_CALL(*permissionMock, CheckCallerIsSystemApp()).WillByDefault(Return(true));
    ON_CALL(*permissionMock, CheckPermission(Eq(std::string(Permission::SET_TELEPHONY_STATE))))
        .WillByDefault(Return(true));
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetInternalSlotIdForDefaultData(_)).WillByDefault(Return(0));
    ON_CALL(*mockSim, HasSimCard(_, _)).WillByDefault(DoAll(SetArgReferee<1>(true), Return(0)));
    ON_CALL(*mockSim, IsSimActive(_)).WillByDefault(Return(true));
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(-1));
    ON_CALL(*mockSim, SetDefaultCellularDataSlotId(_)).WillByDefault(Return(TELEPHONY_ERR_SLOTID_INVALID));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    // A) 全链通 + SetDefault 失败 → 透传错误码
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_SLOTID_INVALID);
    // B) SetDefault 成功 → SUCCESS
    ON_CALL(*mockSim, SetDefaultCellularDataSlotId(_)).WillByDefault(Return(TELEPHONY_ERR_SUCCESS));
    EXPECT_EQ(service->SetDefaultCellularDataSlotId(0), TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetDefaultCellularDataSlotId_External_001
 * @tc.name     test GetDefaultCellularDataSlotId internal and external slot branches
 * @tc.desc     mock 默认卡 -1（< 分布式起始值 → 直透）/ 5（≥ 起始值 → 走外部 slot 换算）。
 */
HWTEST_F(CellularDataServiceTest, GetDefaultCellularDataSlotId_External_001, TestSize.Level1)
{
    auto mockSim = std::make_shared<MockSimManager>();
    ON_CALL(*mockSim, GetExternalSlotIdForDefaultData(_)).WillByDefault(Return(7));
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    // A) 内部 slotId -1 → 直透
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(Return(-1));
    int32_t slotId = -100;
    EXPECT_EQ(service->GetDefaultCellularDataSlotId(slotId), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(slotId, -1);
    // B) 内部 slotId 5（≥ DISTRIBUTED_MULTI_DEVICE_SLOTID_START）→ 外部换算
    ON_CALL(*mockSim, GetDefaultCellularDataSlotId()).WillByDefault(
        Return(DISTRIBUTED_MULTI_DEVICE_SLOTID_START + 1));
    slotId = -100;
    EXPECT_EQ(service->GetDefaultCellularDataSlotId(slotId), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(slotId, 7);
}

/**
 * @tc.number   Dump_FdVariants_001
 * @tc.name     test Dump with fd and args variants
 * @tc.desc     fd<0 → ERR_FAIL；合法 fd + --help 参数 → 帮助文本；未打开高 fd → dprintf 失败。
 */
HWTEST_F(CellularDataServiceTest, Dump_FdVariants_001, TestSize.Level1)
{
    std::vector<std::u16string> helpArgs = { u"cellular_data", u"--help" };
    // A) fd<0 → 早退 ERR_FAIL
    EXPECT_EQ(service->Dump(-1, helpArgs), TELEPHONY_ERR_FAIL);
    // B) 合法 fd + --help → ShowHelp 路径 + dprintf 成功
    EXPECT_EQ(service->Dump(1, helpArgs), TELEPHONY_ERR_SUCCESS);
    // C) 未打开的高 fd → dprintf 返回负值 → ERR_FAIL
    EXPECT_EQ(service->Dump(100000, helpArgs), TELEPHONY_ERR_FAIL);
    // D) 空参数 → ShowCellularDataInfo 路径
    std::vector<std::u16string> emptyArgs;
    EXPECT_EQ(service->Dump(1, emptyArgs), TELEPHONY_ERR_SUCCESS);
}
} // namespace Telephony
} // namespace OHOS