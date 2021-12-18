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

#ifndef CELLULAR_DATA_SERVICE_STUB_H
#define CELLULAR_DATA_SERVICE_STUB_H

#include <map>

#include "iremote_object.h"
#include "iremote_stub.h"

#include "i_cellular_data_manager.h"

namespace OHOS {
namespace Telephony {
class CellularDataServiceStub : public IRemoteStub<ICellularDataManager> {
public:
    CellularDataServiceStub();
    ~CellularDataServiceStub();
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void ProcessSmsRespOrNotify(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);

private:
    int32_t OnIsCellularDataEnabledInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnEnableCellularDataInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetCellularDataStateInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsCellularDataRoamingEnabledInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnEnableCellularDataRoamingInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnReleaseNetInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnRequestNetInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnHandleApnChangedInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetDefaultCellularDataSlotIdInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnSetDefaultCellularDataSlotIdInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetCellularDataFlowTypeInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnStrategySwitchInner(MessageParcel &data, MessageParcel &reply);
    int32_t OnAddCellularDataObserverInner(
          uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int32_t OnRemoveCellularDataObserverInner(
          uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);

private:
    using Fun = int32_t (CellularDataServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, Fun> eventIdFunMap_ {
        {(uint32_t)ICellularDataManager::FuncCode::IS_CELLULAR_DATA_ENABLED,
             &CellularDataServiceStub::OnIsCellularDataEnabledInner},
        {(uint32_t)ICellularDataManager::FuncCode::ENABLE_CELLULAR_DATA,
             &CellularDataServiceStub::OnEnableCellularDataInner},
        {(uint32_t)ICellularDataManager::FuncCode::GET_CELLULAR_DATA_STATE,
             &CellularDataServiceStub::OnGetCellularDataStateInner},
        {(uint32_t)ICellularDataManager::FuncCode::IS_DATA_ROAMING_ENABLED,
             &CellularDataServiceStub::OnIsCellularDataRoamingEnabledInner},
        {(uint32_t)ICellularDataManager::FuncCode::ENABLE_DATA_ROAMING,
             &CellularDataServiceStub::OnEnableCellularDataRoamingInner},
        {(uint32_t)ICellularDataManager::FuncCode::REQUEST_CELLULAR_DATA, &CellularDataServiceStub::OnRequestNetInner},
        {(uint32_t)ICellularDataManager::FuncCode::RELEASE_CELLULAR_DATA, &CellularDataServiceStub::OnReleaseNetInner},
        {(uint32_t)ICellularDataManager::FuncCode::STRATEGY_SWITCH, &CellularDataServiceStub::OnStrategySwitchInner},
        {(uint32_t)ICellularDataManager::FuncCode::APN_DATA_CHANGED,
             &CellularDataServiceStub::OnHandleApnChangedInner},
        {(uint32_t)ICellularDataManager::FuncCode::GET_DEFAULT_SLOT_ID,
             &CellularDataServiceStub::OnGetDefaultCellularDataSlotIdInner},
        {(uint32_t)ICellularDataManager::FuncCode::SET_DEFAULT_SLOT_ID,
             &CellularDataServiceStub::OnSetDefaultCellularDataSlotIdInner},
        {(uint32_t)ICellularDataManager::FuncCode::GET_FLOW_TYPE_ID,
             &CellularDataServiceStub::OnGetCellularDataFlowTypeInner},
    };
    using DataObserver = int32_t (CellularDataServiceStub::*)(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    std::map<uint32_t, DataObserver> DataObserverIdMap_ {
        {(uint32_t)ICellularDataManager::FuncCode::ADD_CELLULAR_DATA_OBSERVER,
             &CellularDataServiceStub::OnAddCellularDataObserverInner},
        {(uint32_t)ICellularDataManager::FuncCode::REMOVE_CELLULAR_DATA_OBSERVER,
            &CellularDataServiceStub::OnRemoveCellularDataObserverInner},
    };
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_DATA_SERVICE_STUB_H
