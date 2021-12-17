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

#ifndef CELLULAR_DATA_HANDLER_H
#define CELLULAR_DATA_HANDLER_H

#include <atomic>
#include <memory>

#include "common_event_manager.h"
#include "event_handler.h"
#include "inner_event.h"

#include "call_manager_inner_type.h"
#include "hril_data_parcel.h"
#include "tel_profile_util.h"

#include "apn_manager.h"
#include "cellular_data_event_code.h"
#include "cellular_data_state_machine.h"
#include "data_switch_settings.h"
#include "state_notification.h"

namespace OHOS {
namespace Telephony {
class CellularDataHandler : public AppExecFwk::EventHandler {
public:
    explicit CellularDataHandler(const std::shared_ptr<AppExecFwk::EventRunner> &runner, int32_t slotId);
    ~CellularDataHandler();
    void Init();
    void StartStallDetectionTimer();
    void StopStallDetectionTimer();
    bool ReleaseNet(const NetRequest &request);
    bool RequestNet(const NetRequest &request);
    void ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event) override;
    bool SetCellularDataEnable(bool userDataEnabled);
    bool IsCellularDataEnabled() const;
    bool IsCellularDataRoamingEnabled() const;
    bool SetCellularDataRoamingEnabled(bool dataRoamingEnabled);
    ApnProfileState GetCellularDataState() const;
    void ClearConnection(sptr<ApnHolder> &apnHolder, DisConnectionReason reason);
    void EstablishAllApnsIfConnectable();
    void ClearAllConnections(DisConnectionReason reason);
    sptr<ApnManager> GetApnManager() const;
    int32_t GetSlotId() const;
    int32_t HandleApnChanged(const std::string &apns);
    void HandleApnChanged(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleCallStateUpdate(const AppExecFwk::InnerEvent::Pointer &event);
    int32_t GetCellularDataFlowType();
    void ClearAllConnectionsFormerSlot();
    void ConnectDataNeWork();
    void SetPolicyDataOn(bool enable);
    bool IsRestrictedMode() const;
    int32_t GetDisConnectionReason();

private:
    std::shared_ptr<CellularDataStateMachine> CreateCellularDataConnect();
    std::shared_ptr<CellularDataStateMachine> FindIdleCellularDataConnection() const;
    void AttemptEstablishDataConnection(sptr<ApnHolder> &apnHolder);
    bool EstablishDataConnection(sptr<ApnHolder> &apnHolder, int32_t radioTech);
    void RadioPsConnectionAttached(const AppExecFwk::InnerEvent::Pointer &event);
    void RadioPsConnectionDetached(const AppExecFwk::InnerEvent::Pointer &event);
    void RoamingStateOn(const AppExecFwk::InnerEvent::Pointer &event);
    void RoamingStateOff(const AppExecFwk::InnerEvent::Pointer &event);
    void PsRadioEmergencyStateOpen(const AppExecFwk::InnerEvent::Pointer &event);
    void PsRadioEmergencyStateClose(const AppExecFwk::InnerEvent::Pointer &event);
    void EstablishDataConnectionComplete(const AppExecFwk::InnerEvent::Pointer &event);
    void DisconnectDataComplete(const AppExecFwk::InnerEvent::Pointer &event);
    void MsgEstablishDataConnection(const AppExecFwk::InnerEvent::Pointer &event);
    void MsgRequestNetwork(const AppExecFwk::InnerEvent::Pointer &event);
    void CheckAndUpdateNetInfo(
        sptr<ApnHolder> &apnHolder, std::shared_ptr<SetupDataCallResultInfo> &infos) const;
    void HandleSettingSwitchChanged(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleVoiceCallChanged(int32_t state);
    void HandleSimStateOrRecordsChanged(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleRadioStateChanged(const AppExecFwk::InnerEvent::Pointer &event);
    void PsDataRatChanged(const AppExecFwk::InnerEvent::Pointer &event);

private:
    std::vector<std::shared_ptr<CellularDataStateMachine>> intStateMachineMap_;
    sptr<ApnManager> apnManager_;
    std::unique_ptr<DataSwitchSettings> dataSwitchSettings_;
    sptr<DataConnectionManager> connectionManager_;
    std::u16string lastIccID_;
    int32_t lastCallState_ = (int32_t)TelCallStatus::CALL_STATE_RELEASED;
    const int32_t slotId_;
    int32_t disconnectionReason_ = REASON_NORMAL;
    std::shared_ptr<AppExecFwk::EventRunner> stateMachineEventLoop_;

    using Fun = void (CellularDataHandler::*)(const AppExecFwk::InnerEvent::Pointer &event);
    std::map<uint32_t, Fun> eventIdMap_ {
        {ObserverHandler::ObserverHandlerId::RADIO_PS_CONNECTION_ATTACHED,
            &CellularDataHandler::RadioPsConnectionAttached},
        {ObserverHandler::ObserverHandlerId::RADIO_PS_CONNECTION_DETACHED,
            &CellularDataHandler::RadioPsConnectionDetached},
        {ObserverHandler::ObserverHandlerId::RADIO_PS_ROAMING_OPEN, &CellularDataHandler::RoamingStateOn},
        {ObserverHandler::ObserverHandlerId::RADIO_PS_ROAMING_CLOSE, &CellularDataHandler::RoamingStateOff},
        {CellularDataEventCode::MSG_ESTABLISH_DATA_CONNECTION_COMPLETE,
            &CellularDataHandler::EstablishDataConnectionComplete},
        {CellularDataEventCode::MSG_DISCONNECT_DATA_COMPLETE, &CellularDataHandler::DisconnectDataComplete},
        {CellularDataEventCode::MSG_ESTABLISH_DATA_CONNECTION, &CellularDataHandler::MsgEstablishDataConnection},
        {CellularDataEventCode::MSG_SETTING_SWITCH, &CellularDataHandler::HandleSettingSwitchChanged},
        {CellularDataEventCode::MSG_REQUEST_NETWORK, &CellularDataHandler::MsgRequestNetwork},
        {ObserverHandler::ObserverHandlerId::RADIO_STATE_CHANGED, &CellularDataHandler::HandleRadioStateChanged},
        {ObserverHandler::ObserverHandler::RADIO_SIM_STATE_CHANGE,
            &CellularDataHandler::HandleSimStateOrRecordsChanged},
        {ObserverHandler::ObserverHandler::RADIO_SIM_RECORDS_LOADED,
            &CellularDataHandler::HandleSimStateOrRecordsChanged},
        {ObserverHandler::ObserverHandlerId::RADIO_PS_RAT_CHANGED, &CellularDataHandler::PsDataRatChanged},
        {CellularDataEventCode::MSG_APN_CHANGED, &CellularDataHandler::HandleApnChanged},
        {ObserverHandler::ObserverHandlerId::RADIO_CALL_STATUS_INFO, &CellularDataHandler::HandleCallStateUpdate}
    };
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_DATA_HANDLER_H