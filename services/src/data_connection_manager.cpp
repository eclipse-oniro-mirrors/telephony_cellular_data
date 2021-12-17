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

#include "data_connection_manager.h"

#include "hril_data_parcel.h"
#include "observer_handler.h"
#include "telephony_log_wrapper.h"

#include "cellular_data_event_code.h"
#include "cellular_data_handler.h"
#include "cellular_data_state_machine.h"
#include "ril_adapter_utils.h"

namespace OHOS {
namespace Telephony {
DataConnectionManager::DataConnectionManager(const std::shared_ptr<AppExecFwk::EventRunner> &runner, int32_t slotId)
    : StateMachine(runner), slotId_(slotId)
{
    connectionMonitor_ = std::make_shared<DataConnectionMonitor>(runner, slotId);
    ccmDefaultState_ = std::make_unique<CcmDefaultState>(*this, "CcmDefaultState").release();
    if (connectionMonitor_ == nullptr || ccmDefaultState_ == nullptr) {
        TELEPHONY_LOGE("connectionMonitor_ or ccmDefaultState_ is null");
        return;
    }
    StateMachine::SetOriginalState(ccmDefaultState_);
    StateMachine::Start();
}

DataConnectionManager::~DataConnectionManager() = default;

void DataConnectionManager::AddConnectionStateMachine(const std::shared_ptr<CellularDataStateMachine> &stateMachine)
{
    if (stateMachine != nullptr) {
        stateMachines_.push_back(stateMachine);
    }
}

void DataConnectionManager::RemoveConnectionStateMachine(const std::shared_ptr<CellularDataStateMachine> &stateMachine)
{
    if (stateMachine == nullptr) {
        TELEPHONY_LOGE("stateMachine is null");
        return;
    }
    for (auto iter = stateMachines_.begin(); iter != stateMachines_.end(); iter++) {
        if (*iter.base() == stateMachine) {
            stateMachines_.erase(iter);
            break;
        }
    }
}

void DataConnectionManager::AddActiveConnectionByCid(const std::shared_ptr<CellularDataStateMachine> &stateMachine)
{
    cidActiveConnectionMap_[stateMachine->GetCid()] = stateMachine;
}

std::shared_ptr<CellularDataStateMachine> DataConnectionManager::GetActiveConnectionByCid(int32_t cid) const
{
    auto it = cidActiveConnectionMap_.find(cid);
    if (it != cidActiveConnectionMap_.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<int32_t, std::shared_ptr<CellularDataStateMachine>> DataConnectionManager::GetActiveConnection() const
{
    return cidActiveConnectionMap_;
}

void DataConnectionManager::RemoveActiveConnectionByCid(int32_t cid)
{
    if (cidActiveConnectionMap_.find(cid) != cidActiveConnectionMap_.end()) {
        cidActiveConnectionMap_.erase(cid);
    }
}

void DataConnectionManager::StartStallDetectionTimer(std::shared_ptr<AppExecFwk::EventHandler> cellularDataHandler)
{
    if (connectionMonitor_ != nullptr) {
        connectionMonitor_->StartStallDetectionTimer(cellularDataHandler);
    }
}

void DataConnectionManager::StopStallDetectionTimer()
{
    if (connectionMonitor_ != nullptr) {
        connectionMonitor_->StopStallDetectionTimer();
    }
}

void DataConnectionManager::RegisterRadioObserver()
{
    RilAdapterUtils::RegisterRilConnected(
        slotId_, stateMachineEventHandler_, ObserverHandler::RADIO_CONNECTED, nullptr);
    RilAdapterUtils::PdpContextListUpdated(
        slotId_, stateMachineEventHandler_, ObserverHandler::RADIO_DATA_CALL_LIST_CHANGED, nullptr);
}

void DataConnectionManager::UnRegisterRadioObserver() const
{
    RilAdapterUtils::UnRegisterRilConnected(slotId_, ObserverHandler::RADIO_CONNECTED);
    RilAdapterUtils::UnPdpContextListUpdated(slotId_, ObserverHandler::RADIO_DATA_CALL_LIST_CHANGED);
}

void CcmDefaultState::StateBegin()
{
    connectManager_.RegisterRadioObserver();
}

void CcmDefaultState::StateEnd()
{
    connectManager_.UnRegisterRadioObserver();
}

bool CcmDefaultState::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    int32_t id = event->GetInnerEventId();
    switch (id) {
        case ObserverHandler::RADIO_CONNECTED:
            TELEPHONY_LOGI("Radio is connected");
            break;
        case ObserverHandler::RADIO_DATA_CALL_LIST_CHANGED:
            RadioDataCallListChanged(event);
            break;
        default:
            TELEPHONY_LOGE("StateProcess handle nothing!");
            return false;
    }
    return true;
}

void CcmDefaultState::RadioDataCallListChanged(const AppExecFwk::InnerEvent::Pointer &event)
{
    auto infos = event->GetSharedObject<DataCallResultList>();
    if (infos == nullptr) {
        TELEPHONY_LOGE("setupDataCallResultInfo is null");
        return;
    }
    UpdateNetworkInfo(event);
    std::vector<std::shared_ptr<CellularDataStateMachine>> retryDataConnection;
    std::map<int32_t, std::shared_ptr<CellularDataStateMachine>> idActiveConnectionMap =
        connectManager_.GetActiveConnection();
    for (const auto &it : idActiveConnectionMap) {
        bool isPush = true;
        if (it.second == nullptr) {
            TELEPHONY_LOGI("The activation item is null(%{public}d)", it.first);
            continue;
        }
        int32_t cid = it.second->GetCid();
        for (size_t i = 0; i < infos->dcList.size(); ++i) {
            if (infos->dcList[i].cid == cid) {
                isPush = false;
                break;
            }
        }
        if (isPush) {
            TELEPHONY_LOGI("cid:%{public}d add to retry", it.first);
            retryDataConnection.push_back(it.second);
        }
    }
    for (auto &it : retryDataConnection) {
        if (it == nullptr) {
            TELEPHONY_LOGI(" retryDataConnection is null");
            continue;
        }
        auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_SM_LOST_CONNECTION);
        it->SendEvent(event);
    }
}

void CcmDefaultState::UpdateNetworkInfo(const AppExecFwk::InnerEvent::Pointer &event)
{
    auto infos = event->GetSharedObject<DataCallResultList>();
    if (infos == nullptr) {
        TELEPHONY_LOGE("dataCallResultList is null");
        return;
    }
    for (auto &it : infos->dcList) {
        auto dataConnect = connectManager_.GetActiveConnectionByCid(it.cid);
        if (dataConnect == nullptr) {
            TELEPHONY_LOGE("get active connection by cid is :=  %{public}d flag:=  %{public}d ", it.cid, it.flag);
            continue;
        }
        dataConnect->UpdateNetworkInfo(it);
    }
}

void DataConnectionManager::BeginNetStatistics()
{
    if (connectionMonitor_ != nullptr) {
        connectionMonitor_->BeginNetStatistics();
    }
}

void DataConnectionManager::EndNetStatistics()
{
    if (connectionMonitor_ != nullptr) {
        connectionMonitor_->EndNetStatistics();
    }
}

int32_t DataConnectionManager::GetSlotId() const
{
    return slotId_;
}

int32_t DataConnectionManager::GetDataFlowType()
{
    if (connectionMonitor_ == nullptr) {
        TELEPHONY_LOGE("connection monitor is null");
        return static_cast<int32_t>(CellDataFlowType::DATA_FLOW_TYPE_NONE);
    }
    CellDataFlowType flowType = connectionMonitor_->GetDataFlowType();
    return static_cast<int>(flowType);
}
} // namespace Telephony
} // namespace OHOS
