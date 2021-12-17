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

#include "state_notification.h"

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
StateNotification StateNotification::stateNotification_;
StateNotification &StateNotification::GetInstance()
{
    return stateNotification_;
}

void StateNotification::UpdateCellularDataConnectState(int32_t slotId, ApnProfileState dataState, int32_t networkType)
{
    sptr<ITelephonyStateNotify> proxy = GetProxy();
    if (proxy != nullptr) {
        proxy->UpdateCellularDataConnectState(slotId, CellularDataStateAdapter(dataState), networkType);
    }
}

void StateNotification::OnUpDataFlowtype(int32_t slotId, CellDataFlowType flowType)
{
    sptr<ITelephonyStateNotify> proxy = GetProxy();
    if (proxy != nullptr) {
        TELEPHONY_LOGI("call ITelephonyStateNotify UpdateCellularDataFlow");
    } else {
        TELEPHONY_LOGE("UpdateCellularDataFlow proxy is null");
    }
}

void StateNotification::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    if (remote == nullptr) {
        TELEPHONY_LOGE("OnRemoteDied failed, remote is nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(mutexProxy_);
    if (proxy_ == nullptr) {
        TELEPHONY_LOGE("OnRemoteDied proxy_ is nullptr");
        return;
    }
    auto serviceRemote = proxy_->AsObject();
    if ((serviceRemote != nullptr) && (serviceRemote == remote.promote())) {
        serviceRemote->RemoveDeathRecipient(deathRecipient_);
        proxy_ = nullptr;
    }
}

sptr<ITelephonyStateNotify> StateNotification::GetProxy()
{
    std::lock_guard<std::mutex> lock(mutexProxy_);
    if (proxy_ != nullptr) {
        return proxy_;
    }
    sptr<ISystemAbilityManager> sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        TELEPHONY_LOGE("Failed to get system ability manager");
        return nullptr;
    }
    sptr<IRemoteObject> obj = sam->CheckSystemAbility(TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID);
    if (obj == nullptr) {
        TELEPHONY_LOGE("Failed to get state register service");
        return nullptr;
    }
    std::unique_ptr<RegisterServiceDeathRecipient> recipient =
        std::make_unique<RegisterServiceDeathRecipient>(*this);
    if (recipient == nullptr) {
        TELEPHONY_LOGE("recipient is null");
        return nullptr;
    }
    sptr<IRemoteObject::DeathRecipient> dr(recipient.release());
    if ((obj->IsProxyObject()) && (!obj->AddDeathRecipient(dr))) {
        TELEPHONY_LOGE("Failed to add death recipient");
        return nullptr;
    }
    proxy_ = iface_cast<ITelephonyStateNotify>(obj);
    deathRecipient_ = dr;
    TELEPHONY_LOGI("Succeed to connect state register service %{public}d", proxy_ == nullptr);
    return proxy_;
}
} // namespace Telephony
} // namespace OHOS