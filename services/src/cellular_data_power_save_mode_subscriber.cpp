/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#include "cellular_data_power_save_mode_subscriber.h"

#include "telephony_types.h"
#include "telephony_log_wrapper.h"
#include "cellular_data_handler.h"

namespace OHOS {
namespace Telephony {
constexpr int32_t USER_OPERATION = 1;
static constexpr int64_t REPLY_COMMON_EVENT_DELAY = 3 * 1000;
void CellularDataPowerSaveModeSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    strAsyncCommonEvent_ = this->GoAsyncCommonEvent();
    if (action == ENTER_STR_TELEPHONY_NOTIFY) {
        TELEPHONY_LOGI("Enter str mode");
        OnHandleEnterStrEvent(action);
    } else if (action == EXIT_STR_TELEPHONY_NOTIFY) {
        TELEPHONY_LOGI("Exit str mode");
        FinishTelePowerCommonEvent();
        int32_t reason = data.GetCode();
        // process only in user operation
        if (reason == USER_OPERATION) {
            OnHandleExitStrEvent(action);
        }
    }
}

void CellularDataPowerSaveModeSubscriber::OnHandleEnterStrEvent(std::string &action)
{
    // except same msg
    if (action != lastMsg_) {
        uint32_t eventId = CellularDataEventCode::MSG_TIMEOUT_TO_REPLY_COMMON_EVENT;
        auto powerSaveModeCellularDataHandler = powerSaveModeCellularDataHandler_.lock();
        if (powerSaveModeCellularDataHandler != nullptr) {
            SetPowerSaveModeFlag(true);
            powerSaveModeCellularDataHandler->SendEvent(eventId, 0, REPLY_COMMON_EVENT_DELAY);
            powerSaveModeCellularDataHandler->ClearAllConnections(DisConnectionReason::REASON_CLEAR_CONNECTION);
        }
    } else {
        FinishTelePowerCommonEvent();
        TELEPHONY_LOGE("Recv same msg");
    }
    lastMsg_ = ENTER_STR_TELEPHONY_NOTIFY;
}

void CellularDataPowerSaveModeSubscriber::OnHandleExitStrEvent(std::string &action)
{
    if (action != lastMsg_) {
        auto powerSaveModeCellularDataHandler = powerSaveModeCellularDataHandler_.lock();
        if (powerSaveModeCellularDataHandler != nullptr) {
            SetPowerSaveModeFlag(false);
            powerSaveModeCellularDataHandler->EstablishAllApnsIfConnectable();
        }
    } else {
        TELEPHONY_LOGE("Recv same msg");
    }
    lastMsg_ = EXIT_STR_TELEPHONY_NOTIFY;
}

bool CellularDataPowerSaveModeSubscriber::FinishTelePowerCommonEvent()
{
    bool replyRet = false;
    if (strAsyncCommonEvent_ != nullptr) {
        replyRet = strAsyncCommonEvent_->FinishCommonEvent();
        TELEPHONY_LOGI("FinishTelePowerCommonEvent replyRet = %{public}d", replyRet);
    } else {
        TELEPHONY_LOGE("strAsyncCommonEvent_ is nullptr");
    }
    return replyRet;
}

bool CellularDataPowerSaveModeSubscriber::GetPowerSaveModeFlag()
{
    return powerSaveModeFlag_;
}

void CellularDataPowerSaveModeSubscriber::SetPowerSaveModeFlag(bool value)
{
    TELEPHONY_LOGI("Set power save mode flag: %{public}d", value);
    powerSaveModeFlag_ = value;
}
}  // namespace Telephony
}  // namespace OHOS
