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

#include "active.h"

#include "telephony_log_wrapper.h"

#include "inactive.h"

namespace OHOS {
namespace Telephony {
void Active::StateBegin()
{
    TELEPHONY_LOGI("Enter active state");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return;
    }
    isActive_ = true;
    shareStateMachine->SetCurrentState(sptr<State>(this));
}

void Active::StateEnd()
{
    TELEPHONY_LOGI("Exit active state");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return;
    }
    isActive_ = false;
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(shareStateMachine->GetSlotId(), shareStateMachine->GetCapability());
    if (shareStateMachine->netSupplierInfo_ != nullptr) {
        shareStateMachine->netSupplierInfo_->isAvailable_ = false;
        netAgent.UpdateNetSupplierInfo(supplierId, shareStateMachine->netSupplierInfo_);
    }
}

bool Active::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    bool retVal = false;
    auto eventCode = event->GetInnerEventId();
    auto it = eventIdFunMap_.find(eventCode);
    if (it != eventIdFunMap_.end()) {
        return (this->*(it->second))(event);
    }
    return retVal;
}

bool Active::ProcessConnectDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::MSG_SM_CONNECT");
    return PROCESSED;
}

bool Active::ProcessDisconnectDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::MSG_SM_DISCONNECT");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    std::unique_ptr<DataDisconnectParams> object = event->GetUniqueObject<DataDisconnectParams>();
    DisConnectionReason reason = object->GetReason();
    auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
    inActive->SetReason(reason);
    shareStateMachine->FreeConnection(*object);
    shareStateMachine->TransitionTo(shareStateMachine->disconnectingState_);
    return PROCESSED;
}

bool Active::ProcessDisconnectAllDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::MSG_SM_DISCONNECT_ALL");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    std::unique_ptr<DataDisconnectParams> object = event->GetUniqueObject<DataDisconnectParams>();
    DisConnectionReason reason = object->GetReason();
    auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
    inActive->SetReason(reason);
    shareStateMachine->FreeConnection(*object);
    shareStateMachine->TransitionTo(shareStateMachine->disconnectingState_);
    return PROCESSED;
}

bool Active::ProcessLostConnection(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::EVENT_LOST_CONNECTION");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
    inActive->SetDeActiveApnTypeId(shareStateMachine->apnId_);
    shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
    return PROCESSED;
}

bool Active::ProcessDataConnectionRoamOn(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::EVENT_DATA_CONNECTION_ROAM_ON");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(shareStateMachine->GetSlotId(), shareStateMachine->GetCapability());
    netAgent.UpdateNetSupplierInfo(supplierId, shareStateMachine->netSupplierInfo_);
    return PROCESSED;
}

bool Active::ProcessDataConnectionRoamOff(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::EVENT_DATA_CONNECTION_ROAM_OFF");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(shareStateMachine->GetSlotId(), shareStateMachine->GetCapability());
    netAgent.UpdateNetSupplierInfo(supplierId, shareStateMachine->netSupplierInfo_);
    return PROCESSED;
}

bool Active::ProcessBwRefreshResponse(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::EVENT_BW_REFRESH_RESPONSE");
    return PROCESSED;
}

bool Active::ProcessDataConnectionVoiceCallStartedOrEnded(const AppExecFwk::InnerEvent::Pointer &event)
{
    TELEPHONY_LOGI("Active::EVENT_DATA_CONNECTION_VOICE_CALL_ENDED or STARTED");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(shareStateMachine->GetSlotId(), shareStateMachine->GetCapability());
    netAgent.UpdateNetSupplierInfo(supplierId, shareStateMachine->netSupplierInfo_);
    return PROCESSED;
}
} // namespace Telephony
} // namespace OHOS
