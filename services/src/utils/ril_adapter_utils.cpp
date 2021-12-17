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

#include "ril_adapter_utils.h"

namespace OHOS {
namespace Telephony {
/**
 * Define and activate PDP context
 * from 3GPP TS 27.007 V4.3.0 10.1.1 and 10.1.10 (2001-12)
 *
 * @param slotId card slot identification
 * @param radioTechnology radio access point
 * @param dataProfile
 * @param isRoaming Is the network roaming state
 * @param allowRoaming whether the roaming switch is on
 * @param response send a response after the request
 */
void RilAdapterUtils::ActivatePdpContext(ActivateInfo activeInfo, int32_t radioTechnology,
    ITelRilManager::CellularDataProfile &dataProfile, const AppExecFwk::InnerEvent::Pointer &response)
{
    int32_t slotId = activeInfo.slotId;
    bool isRoaming = activeInfo.isRoaming;
    bool allowRoaming = activeInfo.allowRoaming;
    TELEPHONY_LOGI("slotId is := %{public}d  is roaming := %{public}d allow Roaming is := %{public}d",
        slotId, isRoaming, allowRoaming);
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->ActivatePdpContext(radioTechnology, std::move(dataProfile), isRoaming, allowRoaming, response);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

/**
 * deactivate the specified PDP context
 * from 3GPP TS 27.007 V4.3.0 10.1.10 (2001-12)
 *
 * @param slotId card slot identification
 * @param cid context ID, uniquely identifies this call
 * @param reason reason to deactivate the data request
 * @param response send a response after the request
 */
void RilAdapterUtils::DeactivatePdpContext(
    int32_t slotId, int32_t cid, int32_t reason, const AppExecFwk::InnerEvent::Pointer &response)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->DeactivatePdpContext(cid, reason, response);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::GetPdpContextList(int32_t slotId, const AppExecFwk::InnerEvent::Pointer &response)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->GetPdpContextList(response);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::RegisterRilConnected(
    int32_t slotId, const std::shared_ptr<AppExecFwk::EventHandler> &handler, int what, void *obj)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->RegisterCoreNotify(handler, what, nullptr);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::PdpContextListUpdated(
    int32_t slotId, const std::shared_ptr<AppExecFwk::EventHandler> &handler, int what, void *obj)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->RegisterCoreNotify(handler, what, nullptr);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::UnRegisterRilConnected(int32_t slotId, int what)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->UnRegisterCoreNotify(nullptr, what);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::UnPdpContextListUpdated(int32_t slotId, int what)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->UnRegisterCoreNotify(nullptr, what);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::GetPreferredNetworkPara(int32_t slotId, const AppExecFwk::InnerEvent::Pointer &response)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->GetPreferredNetwork(response);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}

void RilAdapterUtils::SetPreferredNetworkPara(
    int32_t slotId, int32_t preferredNetworkType, const AppExecFwk::InnerEvent::Pointer &response)
{
    auto core = CoreManager::GetInstance().getCore(slotId);
    if (core != nullptr) {
        core->SetPreferredNetwork(preferredNetworkType, response);
    } else {
        TELEPHONY_LOGE("core is null slotId:%{public}d", slotId);
    }
}
} // namespace Telephony
} // namespace OHOS