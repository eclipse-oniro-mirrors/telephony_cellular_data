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

#include "inactive.h"

#include "telephony_log_wrapper.h"

#include "apn_manager.h"
#include "cellular_data_event_code.h"

namespace OHOS {
namespace Telephony {
void Inactive::StateBegin()
{
    TELEPHONY_LOGI("Enter inactive state");
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("shareStateMachine is null");
        return;
    }
    isActive_ = true;
    if (deActiveApnTypeId_ != ERROR_APN_ID) {
        std::string apnType = ApnManager::FindApnNameByApnId(deActiveApnTypeId_);
        std::unique_ptr<DataDisconnectParams> object = std::make_unique<DataDisconnectParams>(apnType, reason_);
        auto event = AppExecFwk::InnerEvent::Get(CellularDataEventCode::MSG_DISCONNECT_DATA_COMPLETE, object);
        if (shareStateMachine->cellularDataHandler_ != nullptr) {
            shareStateMachine->cellularDataHandler_->SendEvent(event);
        }
        deActiveApnTypeId_ = ERROR_APN_ID;
        reason_ = REASON_RETRY_CONNECTION;
    }
    shareStateMachine->SetCurrentState(sptr<State>(this));
    if (shareStateMachine->cdConnectionManager_ != nullptr) {
        shareStateMachine->cdConnectionManager_->RemoveActiveConnectionByCid(shareStateMachine->GetCid());
    }
}

void Inactive::StateEnd()
{
    TELEPHONY_LOGI("Exit inactive state");
    isActive_ = false;
}

bool Inactive::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
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
        case CellularDataEventCode::MSG_SM_CONNECT: {
            TELEPHONY_LOGI("Inactive::MSG_SM_CONNECT");
            shareStateMachine->DoConnect(*(event->GetUniqueObject<DataConnectionParams>()));
            shareStateMachine->TransitionTo(shareStateMachine->activatingState_);
            retVal = PROCESSED;
            break;
        }
        case CellularDataEventCode::MSG_SM_DISCONNECT: {
            TELEPHONY_LOGI("Inactive::MSG_SM_DISCONNECT");
            retVal = PROCESSED;
            break;
        }
        case CellularDataEventCode::MSG_SM_DISCONNECT_ALL: {
            TELEPHONY_LOGI("Inactive::MSG_SM_DISCONNECT_ALL");
            retVal = PROCESSED;
            break;
        }
        default:
            TELEPHONY_LOGE("StateProcess handle nothing!");
            break;
    }
    return retVal;
}

const std::weak_ptr<CellularDataStateMachine> &Inactive::GetStateMachine() const
{
    return stateMachine_;
}

void Inactive::SetStateMachine(const std::weak_ptr<CellularDataStateMachine> &stateMachine)
{
    stateMachine_ = stateMachine;
}

int32_t Inactive::GetDeActiveApnTypeId() const
{
    return deActiveApnTypeId_;
}

void Inactive::SetDeActiveApnTypeId(int32_t deActiveApnTypeId)
{
    deActiveApnTypeId_ = deActiveApnTypeId;
}

void Inactive::SetReason(DisConnectionReason reason)
{
    reason_ = reason;
}
} // namespace Telephony
} // namespace OHOS