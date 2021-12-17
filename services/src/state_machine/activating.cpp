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

#include "activating.h"

#include "hril_data_parcel.h"
#include "telephony_log_wrapper.h"

#include "cellular_data_event_code.h"
#include "inactive.h"
#include "ril_adapter_utils.h"

namespace OHOS {
namespace Telephony {
void Activating::StateBegin()
{
    TELEPHONY_LOGI("Enter activating state");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return;
    }
    isActive_ = true;
    shareStateMachine->SetCurrentState(sptr<State>(this));
}

void Activating::StateEnd()
{
    TELEPHONY_LOGI("Exit activating state");
    isActive_ = false;
}

bool Activating::RilActivatePdpContextDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    auto resultInfo = event->GetSharedObject<SetupDataCallResultInfo>();
    if (resultInfo == nullptr) {
        TELEPHONY_LOGE("result info is null");
        return RilErrorResponse(event);
    }
    TELEPHONY_LOGI("callDone active: %{public}d flag: %{public}d, cid: %{public}d", resultInfo->active,
        resultInfo->flag, resultInfo->cid);
    if (resultInfo->active == 0) {
        auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
        inActive->SetDeActiveApnTypeId(shareStateMachine->apnId_);
        shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
        return true;
    }
    shareStateMachine->SetCid(resultInfo->cid);
    if (shareStateMachine->cdConnectionManager_ != nullptr) {
        shareStateMachine->cdConnectionManager_->AddActiveConnectionByCid(stateMachine_.lock());
    } else {
        TELEPHONY_LOGE("cdConnectionManager is null");
    }
    if (shareStateMachine->cellularDataHandler_ != nullptr) {
        shareStateMachine->cellularDataHandler_->SendEvent(
            CellularDataEventCode::MSG_ESTABLISH_DATA_CONNECTION_COMPLETE, resultInfo);
    } else {
        TELEPHONY_LOGE("cellularDataHandler is null");
    }
    shareStateMachine->TransitionTo(shareStateMachine->activeState_);
    return true;
}

bool Activating::RilErrorResponse(const AppExecFwk::InnerEvent::Pointer &event)
{
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    auto rilInfo = event->GetSharedObject<HRilRadioResponseInfo>();
    if (rilInfo == nullptr) {
        TELEPHONY_LOGE("SetupDataCallResultInfo and HRilRadioResponseInfo is null");
        return false;
    }
    TELEPHONY_LOGI("HRilRadioResponseInfo flag:%{public}d error:%{public}d", rilInfo->flag, rilInfo->error);
    Inactive *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
    switch (rilInfo->error) {
        case HRilErrType::HRIL_ERR_GENERIC_FAILURE:
        case HRilErrType::HRIL_ERR_CMD_SEND_FAILURE:
            inActive->SetReason(REASON_RETRY_CONNECTION);
            TELEPHONY_LOGI("Handle supported error responses and retry the connection.");
            break;
        case HRilErrType::HRIL_ERR_INVALID_RESPONSE:
        case HRilErrType::HRIL_ERR_INVALID_PARAMETER:
        case HRilErrType::HRIL_ERR_MEMORY_FULL:
        case HRilErrType::HRIL_ERR_CMD_NO_CARRIER:
            inActive->SetReason(REASON_CLEAR_CONNECTION);
            TELEPHONY_LOGI("Handle the supported error response and clear the connection.");
            break;
        default: {
            TELEPHONY_LOGE("Handle the unsupported error response");
            break;
        }
    }
    inActive->SetDeActiveApnTypeId(shareStateMachine->apnId_);
    shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
    return true;
}

bool Activating::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return false;
    }
    bool retVal = false;
    auto eventCode = event->GetInnerEventId();
    switch (eventCode) {
        case CellularDataEventCode::MSG_SM_DRS_OR_RAT_CHANGED:
            [[fallthrough]];
        case CellularDataEventCode::MSG_SM_CONNECT:
            TELEPHONY_LOGI("Activating::MSG_SM_CONNECT");
            shareStateMachine->DeferEvent(std::move(event));
            retVal = PROCESSED;
            break;
        case ObserverHandler::RADIO_RIL_SETUP_DATA_CALL: {
            retVal = RilActivatePdpContextDone(event);
            break;
        }
        case CellularDataEventCode::MSG_SM_GET_LAST_FAIL_DONE:
            shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
            retVal = PROCESSED;
            break;
        default:
            TELEPHONY_LOGE("eventCode:%{public}d goto default", eventCode);
            break;
    }
    return retVal;
}
} // namespace Telephony
} // namespace OHOS