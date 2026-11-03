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

#ifndef MOCK_CELLULAR_DATA_MANAGER_H
#define MOCK_CELLULAR_DATA_MANAGER_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "icellular_data_manager.h"

namespace OHOS {
namespace Telephony {

/*
 * ICellularDataManager（SA 4007 IDL 接口）的 gmock 桩。
 * 用于单测中注入 CellularDataClient::proxy_，实现"客户端测客户端"：
 * 只验证客户端封装（参数传递/返回值/缓存逻辑），不做真实 IPC。
 */
class MockCellularDataManager : public ICellularDataManager {
public:
    MockCellularDataManager() = default;
    ~MockCellularDataManager() = default;

    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
    MOCK_METHOD(ErrCode, IsCellularDataEnabled, (bool &dataEnabled), (override));
    MOCK_METHOD(ErrCode, EnableCellularData, (bool enable), (override));
    MOCK_METHOD(ErrCode, EnableIntelligenceSwitch, (bool enable), (override));
    MOCK_METHOD(ErrCode, GetCellularDataState, (int32_t &state), (override));
    MOCK_METHOD(ErrCode, IsCellularDataRoamingEnabled, (int32_t slotId, bool &dataRoamingEnabled), (override));
    MOCK_METHOD(ErrCode, EnableCellularDataRoaming, (int32_t slotId, bool enable), (override));
    MOCK_METHOD(ErrCode, HandleApnChanged, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, GetDefaultCellularDataSlotId, (int32_t &slotId), (override));
    MOCK_METHOD(ErrCode, GetDefaultCellularDataSimId, (int32_t &simId), (override));
    MOCK_METHOD(ErrCode, SetDefaultCellularDataSlotId, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, GetCellularDataFlowType, (int32_t &type), (override));
    MOCK_METHOD(ErrCode, HasInternetCapability, (int32_t slotId, int32_t cid, int32_t &capability), (override));
    MOCK_METHOD(ErrCode, ClearAllConnections, (int32_t slotId, int32_t reason), (override));
    MOCK_METHOD(ErrCode, RegisterSimAccountCallback, (const sptr<SimAccountCallback> &callbackparam), (override));
    MOCK_METHOD(ErrCode, UnregisterSimAccountCallback, (const sptr<SimAccountCallback> &callbackparam), (override));
    MOCK_METHOD(ErrCode, GetDataConnApnAttr, (int32_t slotId, ApnAttribute &apnAttr), (override));
    MOCK_METHOD(ErrCode, GetDataConnIpType, (int32_t slotId, std::string &ipType), (override));
    MOCK_METHOD(ErrCode, GetApnState, (int32_t slotId, const std::string &apnType, int32_t &state), (override));
    MOCK_METHOD(ErrCode, GetDataRecoveryState, (int32_t &state), (override));
    MOCK_METHOD(ErrCode, IsNeedDoRecovery, (int32_t slotId, bool needDoRecovery), (override));
    MOCK_METHOD(ErrCode, InitCellularDataController, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, EstablishAllApnsIfConnectable, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, GetCellularDataSupplierId, (int32_t slotId, uint64_t capability, uint32_t &supplierId),
        (override));
    MOCK_METHOD(ErrCode, CorrectNetSupplierNoAvailable, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, GetSupplierRegisterState, (uint32_t supplierId, int32_t &regState), (override));
    MOCK_METHOD(ErrCode, GetIfSupportDunApn, (bool &isSupportDun), (override));
    MOCK_METHOD(ErrCode, GetDefaultActReportInfo, (int32_t slotId, ApnActivateReportInfoIpc &info), (override));
    MOCK_METHOD(ErrCode, GetInternalActReportInfo, (int32_t slotId, ApnActivateReportInfoIpc &info), (override));
    MOCK_METHOD(ErrCode, QueryApnIds, (const ApnInfo &apnInfo, std::vector<uint32_t> &apnIdList), (override));
    MOCK_METHOD(ErrCode, SetPreferApn, (int32_t apnId), (override));
    MOCK_METHOD(ErrCode, QueryAllApnInfo, (std::vector<ApnInfo> &apnInfoList), (override));
    MOCK_METHOD(ErrCode, SendUrspDecodeResult, (int32_t slotId, const std::vector<uint8_t> &buffer), (override));
    MOCK_METHOD(ErrCode, SendUePolicySectionIdentifier, (int32_t slotId, const std::vector<uint8_t> &buffer),
        (override));
    MOCK_METHOD(ErrCode, SendImsRsdList, (int32_t slotId, const std::vector<uint8_t> &buffer), (override));
    MOCK_METHOD(ErrCode, GetNetworkSliceAllowedNssai, (int32_t slotId, const std::vector<uint8_t> &buffer),
        (override));
    MOCK_METHOD(ErrCode, GetNetworkSliceEhplmn, (int32_t slotId), (override));
    MOCK_METHOD(ErrCode, GetActiveApnName, (std::string &apnName), (override));
};
} // namespace Telephony
} // namespace OHOS
#endif // MOCK_CELLULAR_DATA_MANAGER_H
