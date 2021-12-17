/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "cellular_data_service_stub.h"

#include <string_ex.h>

#include "ipc_skeleton.h"

#include "telephony_errors.h"
#include "telephony_log_wrapper.h"

#include "cellular_data_controller.h"
#include "cellular_data_service.h"

namespace OHOS {
namespace Telephony {
CellularDataServiceStub::CellularDataServiceStub() = default;

CellularDataServiceStub::~CellularDataServiceStub() = default;

int32_t CellularDataServiceStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::u16string myDescriptor = CellularDataServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    // NetManager has no transport description
    if ((code != (uint32_t)ICellularDataManager::FuncCode::REQUEST_CELLULAR_DATA) &&
        (code != (uint32_t)ICellularDataManager::FuncCode::RELEASE_CELLULAR_DATA) &&
        (code != (uint32_t)ICellularDataManager::FuncCode::STRATEGY_SWITCH) && (myDescriptor != remoteDescriptor)) {
        TELEPHONY_LOGE("descriptor check fail!");
        return TELEPHONY_ERR_DESCRIPTOR_MISMATCH;
    }
    auto it = eventIdFunMap_.find(code);
    if (it != eventIdFunMap_.end()) {
        return (this->*(it->second))(data, reply);
    } else {
        TELEPHONY_LOGE("event code is not exist");
    }
    if (code == (uint32_t)ICellularDataManager::FuncCode::ADD_CELLULAR_DATA_OBSERVER ||
        code == (uint32_t)ICellularDataManager::FuncCode::REMOVE_CELLULAR_DATA_OBSERVER) {
        ProcessSmsRespOrNotify(code, data, reply, option);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

void CellularDataServiceStub::ProcessSmsRespOrNotify(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto itFunc = DataObserverIdMap_.find(code);
    if (itFunc != DataObserverIdMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            (this->*memberFunc)(code, data, reply, option);
        }
    }
}

int32_t CellularDataServiceStub::OnIsCellularDataEnabledInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t result = IsCellularDataEnabled();
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnEnableCellularDataInner(MessageParcel &data, MessageParcel &reply)
{
    bool enable = data.ReadBool();
    int32_t result = EnableCellularData(enable);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnGetCellularDataStateInner(MessageParcel &data, MessageParcel &reply)
{
    auto result = GetCellularDataState();
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnIsCellularDataRoamingEnabledInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t slotId = data.ReadInt32();
    int32_t result = IsCellularDataRoamingEnabled(slotId);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return (int32_t)result;
}

int32_t CellularDataServiceStub::OnEnableCellularDataRoamingInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t slotId = data.ReadInt32();
    bool enable = data.ReadBool();
    int32_t result = EnableCellularDataRoaming(slotId, enable);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnAddCellularDataObserverInner(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t CellularDataServiceStub::OnRemoveCellularDataObserverInner(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &opt)
{
    return IPCObjectStub::OnRemoteRequest(code, data, reply, opt);
}

int32_t CellularDataServiceStub::OnReleaseNetInner(MessageParcel &data, MessageParcel &reply)
{
    std::string ident = data.ReadString();
    uint64_t capability = data.ReadUint64();
    int32_t result = ReleaseNet(ident, capability);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnRequestNetInner(MessageParcel &data, MessageParcel &reply)
{
    std::string ident = data.ReadString();
    uint64_t capability = data.ReadUint64();
    int32_t result = RequestNet(ident, capability);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnHandleApnChangedInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t slotId = data.ReadInt32();
    std::string apn = data.ReadString();
    int32_t result = HandleApnChanged(slotId, apn);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnGetDefaultCellularDataSlotIdInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t result = GetDefaultCellularDataSlotId();
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnSetDefaultCellularDataSlotIdInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t slotId = data.ReadInt32();
    int32_t result = SetDefaultCellularDataSlotId(slotId);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnGetCellularDataFlowTypeInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t result = GetCellularDataFlowType();
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}

int32_t CellularDataServiceStub::OnStrategySwitchInner(MessageParcel &data, MessageParcel &reply)
{
    bool enable = data.ReadBool();
    int32_t result = StrategySwitch(enable);
    if (!reply.WriteBool(result)) {
        TELEPHONY_LOGE("fail to write parcel");
        return TELEPHONY_ERR_WRITE_REPLY_FAIL;
    }
    return result;
}
} // namespace Telephony
} // namespace OHOS