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

#ifndef STATE_NOTIFICATION_H
#define STATE_NOTIFICATION_H

#include <mutex>

#include "refbase.h"

#include "i_telephony_state_notify.h"

#include "cellular_data_constant.h"

namespace OHOS {
namespace Telephony {
class StateNotification {
public:
    static StateNotification &GetInstance();
    void UpdateCellularDataConnectState(int32_t slotId, ApnProfileState dataState, int32_t networkType);
    void OnUpDataFlowtype(int32_t slotId, CellDataFlowType flowType);

private:
    class RegisterServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit RegisterServiceDeathRecipient(StateNotification &client) : client_(client) {}
        ~RegisterServiceDeathRecipient() override = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override
        {
            client_.OnRemoteDied(remote);
        }

    private:
        StateNotification &client_;
    };

    StateNotification() = default;
    ~StateNotification() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote);
    sptr<ITelephonyStateNotify> GetProxy();

private:
    static StateNotification stateNotification_;
    std::mutex mutexProxy_;
    sptr<ITelephonyStateNotify> proxy_ {nullptr};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {nullptr};
};
} // namespace Telephony
} // namespace OHOS
#endif // STATE_NOTIFICATION_H
