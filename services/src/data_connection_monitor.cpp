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

#include "data_connection_monitor.h"

#include "cellular_data_types.h"
#include "hril_network_parcel.h"
#include "telephony_log_wrapper.h"

#include "cellular_data_event_code.h"
#include "cellular_data_handler.h"
#include "network_search_utils.h"
#include "ril_adapter_utils.h"

namespace OHOS {
namespace Telephony {
DataConnectionMonitor::DataConnectionMonitor(const std::shared_ptr<AppExecFwk::EventRunner> &runner, int32_t slotId)
    : AppExecFwk::EventHandler(runner), slotId_(slotId)
{
    trafficManager_ = std::make_unique<TrafficManagement>();
    stallDetectionTrafficManager_ = std::make_unique<TrafficManagement>();
}

DataConnectionMonitor::~DataConnectionMonitor()
{
    RemoveAllEvents();
}

void DataConnectionMonitor::StartStallDetectionTimer(std::shared_ptr<AppExecFwk::EventHandler> &cellularDataHandler)
{
    if (!cellularDataHandler_.lock()) {
        cellularDataHandler_ = std::static_pointer_cast<CellularDataHandler>(cellularDataHandler);
    }
    TELEPHONY_LOGI("start stall detection");
    stallDetectionEnabled = true;
    if (!HasInnerEvent(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID) && stallDetectionEnabled) {
        auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID);
        SendEvent(event, DEFAULT_STALL_DETECTION_PERIOD, Priority::LOW);
    }
}

void DataConnectionMonitor::OnStallDetectionTimer()
{
    TELEPHONY_LOGI("on stall detection");
    UpdateFlowInfo();
    if (noRecvPackets_ > RECOVERY_TRIGGER_PACKET) {
        HandleRecovery();
        noRecvPackets_ = 0;
    }
    if (!HasInnerEvent(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID) && stallDetectionEnabled) {
        auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID);
        SendEvent(event, DEFAULT_STALL_DETECTION_PERIOD, Priority::LOW);
    }
}

void DataConnectionMonitor::StopStallDetectionTimer()
{
    TELEPHONY_LOGI("stop stall detection");
    stallDetectionEnabled = false;
    RemoveEvent(CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID);
}

void DataConnectionMonitor::UpdateFlowInfo()
{
    if (stallDetectionTrafficManager_ == nullptr) {
        TELEPHONY_LOGE("stallDetectionTrafficManager_ is null");
        return;
    }
    int64_t previousSentPackets = 0;
    int64_t previousRecvPackets = 0;
    int64_t currentSentPackets = 0;
    int64_t currentRecvPackets = 0;
    stallDetectionTrafficManager_->GetPacketData(previousSentPackets, previousRecvPackets);
    stallDetectionTrafficManager_->UpdatePacketData();
    stallDetectionTrafficManager_->GetPacketData(currentSentPackets, currentRecvPackets);
    int64_t sentPackets = currentSentPackets - previousSentPackets;
    int64_t recvPackets = currentRecvPackets - previousRecvPackets;
    if (sentPackets > 0 && recvPackets == 0) {
        noRecvPackets_ += sentPackets;
    } else if ((sentPackets > 0 && recvPackets > 0) || (sentPackets == 0 && recvPackets > 0)) {
        noRecvPackets_ = 0;
        dataRecoveryState_ = RecoveryState::STATE_REQUEST_CONTEXT_LIST;
    } else {
        TELEPHONY_LOGE("Update Flow Info nothing to do");
    }
}

void DataConnectionMonitor::HandleRecovery()
{
    auto cellularDataHandler = cellularDataHandler_.lock();
    if (cellularDataHandler == nullptr) {
        TELEPHONY_LOGE("cellularDataHandler is null");
        return;
    }
    TELEPHONY_LOGI("Handle recovery");
    switch (dataRecoveryState_) {
        case RecoveryState::STATE_REQUEST_CONTEXT_LIST: {
            TELEPHONY_LOGI("Handle Recovery: get data call list");
            dataRecoveryState_ = RecoveryState::STATE_CLEANUP_CONNECTIONS;
            GetPdpContextList();
            break;
        }
        case RecoveryState::STATE_CLEANUP_CONNECTIONS:
            TELEPHONY_LOGI("Handle Recovery: cleanup connections");
            dataRecoveryState_ = RecoveryState::STATE_REREGISTER_NETWORK;
            cellularDataHandler->ClearAllConnections(REASON_RETRY_CONNECTION);
            break;
        case RecoveryState::STATE_REREGISTER_NETWORK:
            TELEPHONY_LOGI("Handle Recovery: re-register network");
            dataRecoveryState_ = RecoveryState::STATE_RADIO_STATUS_RESTART;
            GetPreferredNetworkPara();
            break;
        case RecoveryState::STATE_RADIO_STATUS_RESTART:
            TELEPHONY_LOGI("Handle Recovery: radio restart");
            dataRecoveryState_ = RecoveryState::STATE_REQUEST_CONTEXT_LIST;
            cellularDataHandler->ClearAllConnections(REASON_RETRY_CONNECTION);
            SetRadioState(static_cast<int32_t>(ModemPowerState::CORE_SERVICE_POWER_OFF), ObserverHandler::RADIO_OFF);
            break;
        default:
            TELEPHONY_LOGE("Handle Recovery is falsie");
            break;
    }
}

void DataConnectionMonitor::BeginNetStatistics()
{
    updateNetStat_ = true;
    UpdateNetTrafficState();
}

void DataConnectionMonitor::EndNetStatistics()
{
    RemoveEvent(CellularDataEventCode::MSG_RUN_MONITOR_TASK);
    updateNetStat_ = false;
    dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_NONE;
}

void DataConnectionMonitor::UpdateNetTrafficState()
{
    if (!HasInnerEvent(CellularDataEventCode::MSG_RUN_MONITOR_TASK) && updateNetStat_) {
        UpdateDataFlowType();
        auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_RUN_MONITOR_TASK);
        SendEvent(event, DEFAULT_NET_STATISTICS_PERIOD);
    }
}

void DataConnectionMonitor::GetPdpContextList()
{
    auto event = AppExecFwk::InnerEvent::Get(ObserverHandler::RADIO_DATA_CALL_LIST_CHANGED);
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    event->SetOwner(shared_from_this());
    RilAdapterUtils::GetPdpContextList(slotId_, event);
}

void DataConnectionMonitor::SetRadioState(const int32_t &radioState, const int32_t &eventCode)
{
    auto event = AppExecFwk::InnerEvent::Get(eventCode);
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    event->SetOwner(shared_from_this());
    NetworkSearchUtils::SetRadioState(slotId_, radioState, 0, event);
}

void DataConnectionMonitor::GetPreferredNetworkPara()
{
    auto event = AppExecFwk::InnerEvent::Get(ObserverHandler::RADIO_GET_PREFERRED_NETWORK_MODE);
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    event->SetOwner(shared_from_this());
    RilAdapterUtils::GetPreferredNetworkPara(slotId_, event);
}

void DataConnectionMonitor::SetPreferredNetworkPara(const AppExecFwk::InnerEvent::Pointer &event)
{
    auto preferredNetworkInfo = event->GetSharedObject<PreferredNetworkTypeInfo>();
    int32_t networkType = preferredNetworkInfo->preferredNetworkType;
    auto responseEvent = AppExecFwk::InnerEvent::Get(ObserverHandler::RADIO_SET_PREFERRED_NETWORK_MODE);
    if (responseEvent == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    responseEvent->SetOwner(shared_from_this());
    RilAdapterUtils::SetPreferredNetworkPara(slotId_, networkType, responseEvent);
}

void DataConnectionMonitor::UpdateDataFlowType()
{
    if (trafficManager_ == nullptr) {
        TELEPHONY_LOGE("trafficManager is null");
        return;
    }
    int64_t previousSentPackets = 0;
    int64_t previousRecvPackets = 0;
    int64_t currentSentPackets = 0;
    int64_t currentRecvPackets = 0;
    trafficManager_->GetPacketData(previousSentPackets, previousRecvPackets);
    trafficManager_->UpdatePacketData();
    trafficManager_->GetPacketData(currentSentPackets, currentRecvPackets);
    int64_t sentPackets = currentSentPackets - previousSentPackets;
    int64_t recvPackets = currentRecvPackets - previousRecvPackets;
    CellDataFlowType previousDataFlowType = dataFlowType_;
    if (previousSentPackets != 0 || previousRecvPackets != 0) {
        if (sentPackets > 0 && recvPackets == 0) {
            dataFlowType_ =  CellDataFlowType::DATA_FLOW_TYPE_UP;
        } else if (sentPackets == 0 && recvPackets > 0) {
            dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_DOWN;
        } else if (sentPackets > 0 && recvPackets > 0) {
            dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_UP_DOWN;
        } else {
            dataFlowType_ = CellDataFlowType::DATA_FLOW_TYPE_NONE;
        }
    }
    if (previousDataFlowType != dataFlowType_) {
        StateNotification::GetInstance().OnUpDataFlowtype(slotId_, dataFlowType_);
    }
}

CellDataFlowType DataConnectionMonitor::GetDataFlowType()
{
    return dataFlowType_;
}

void DataConnectionMonitor::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    auto eventID = event->GetInnerEventId();
    switch (eventID) {
        case CellularDataEventCode::MSG_RUN_MONITOR_TASK: {
            UpdateNetTrafficState();
            break;
        }
        case CellularDataEventCode::MSG_STALL_DETECTION_EVENT_ID:
            OnStallDetectionTimer();
            break;
        case ObserverHandler::RADIO_DATA_CALL_LIST_CHANGED:
            TELEPHONY_LOGI("radio call list changed complete");
            break;
        case ObserverHandler::RADIO_GET_PREFERRED_NETWORK_MODE:
            SetPreferredNetworkPara(event);
            break;
        case ObserverHandler::RADIO_SET_PREFERRED_NETWORK_MODE:
            TELEPHONY_LOGI("set preferred network mode complete");
            break;
        case ObserverHandler::RADIO_OFF:
            SetRadioState(static_cast<int32_t>(ModemPowerState::CORE_SERVICE_POWER_ON), ObserverHandler::RADIO_ON);
            break;
        case ObserverHandler::RADIO_ON:
            TELEPHONY_LOGI("set radio state on complete");
            break;
        default:
            TELEPHONY_LOGI("connection monitor ProcessEvent code = %{public}u", eventID);
            break;
    }
}
} // namespace Telephony
} // namespace OHOS