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

#include "disconnecting.h"

#include "hril_data_parcel.h"
#include "telephony_log_wrapper.h"

#include "cellular_data_event_code.h"
#include "inactive.h"

namespace OHOS {
namespace Telephony {
void Disconnecting::StateBegin()
{
    TELEPHONY_LOGI("Enter disconnecting state");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return;
    }
    isActive_ = true;
    shareStateMachine->SetCurrentState(sptr<State>(this));
}

void Disconnecting::StateEnd()
{
    TELEPHONY_LOGI("Disconnecting::exit");
    isActive_ = false;
}

bool Disconnecting::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
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
    uint32_t eventCode = event->GetInnerEventId();
    switch (eventCode) {
        case CellularDataEventCode::MSG_SM_CONNECT:
            TELEPHONY_LOGI("Disconnecting::MSG_SM_CONNECT");
            shareStateMachine->DeferEvent(std::move(event));
            retVal = PROCESSED;
            break;
        case ObserverHandler::RADIO_RIL_DEACTIVATE_DATA_CALL: {
            auto resultInfo = event->GetSharedObject<SetupDataCallResultInfo>();
            if (resultInfo == nullptr) {
                auto rilInfo = event->GetSharedObject<HRilRadioResponseInfo>();
                if (rilInfo == nullptr) {
                    TELEPHONY_LOGE("SetupDataCallResultInfo and HRilRadioResponseInfo is null");
                    retVal = PROCESSED;
                    break;
                }
                auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
                inActive->SetDeActiveApnTypeId(rilInfo->flag);
                shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
                retVal = PROCESSED;
                break;
            }
            auto *inActive = static_cast<Inactive *>(shareStateMachine->inActiveState_.GetRefPtr());
            inActive->SetDeActiveApnTypeId(resultInfo->flag);
            shareStateMachine->TransitionTo(shareStateMachine->inActiveState_);
            retVal = PROCESSED;
            break;
        }
        default:
            TELEPHONY_LOGE("disconnecting StateProcess do nothing!");
            break;
    }
    return retVal;
}
} // namespace Telephony
} // namespace OHOS
