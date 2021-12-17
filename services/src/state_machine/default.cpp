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

#include "default.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
void Default::StateBegin()
{
    TELEPHONY_LOGI("Enter default state");
    isActive_ = true;
}

void Default::StateEnd()
{
    TELEPHONY_LOGI("Exit default state");
    isActive_ = false;
}

bool Default::StateProcess(const AppExecFwk::InnerEvent::Pointer &event)
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
    auto it = eventIdFunMap_.find(eventCode);
    if (it != eventIdFunMap_.end()) {
        return (this->*(it->second))(event);
    }
    return retVal;
}

bool Default::ProcessConnectDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    TELEPHONY_LOGI("Default::MSG_SM_CONNECT");
    return false;
}

bool Default::ProcessDisconnectDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("The state machine pointer is null");
        return false;
    }
    TELEPHONY_LOGI("The data connection is disconnected by default");
    shareStateMachine->DeferEvent(std::move(event));
    return true;
}

bool Default::ProcessDisconnectAllDone(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    auto shareStateMachine = stateMachine_.lock();
    if (shareStateMachine == nullptr) {
        TELEPHONY_LOGE("The state machine pointer is null");
        return false;
    }
    TELEPHONY_LOGI("All data connections are disconnected by default");
    shareStateMachine->DeferEvent(std::move(event));
    return true;
}

bool Default::ProcessDataConnectionDrsOrRatChanged(const AppExecFwk::InnerEvent::Pointer &event)
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
    TELEPHONY_LOGI("The RAT changes by default");
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(shareStateMachine->GetSlotId(), shareStateMachine->GetCapability());
    netAgent.UpdateNetSupplierInfo(supplierId, shareStateMachine->netSupplierInfo_);
    netAgent.UpdateNetLinkInfo(supplierId, shareStateMachine->netLinkInfo_);
    return false;
}

bool Default::ProcessDataConnectionRoamOn(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    TELEPHONY_LOGI("Default::EVENT_DATA_CONNECTION_ROAM_ON");
    return false;
}

bool Default::ProcessDataConnectionRoamOff(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return false;
    }
    TELEPHONY_LOGI("Default::EVENT_DATA_CONNECTION_ROAM_OFF");
    return false;
}
} // namespace Telephony
} // namespace OHOS
