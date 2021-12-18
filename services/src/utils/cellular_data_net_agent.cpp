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

#include "cellular_data_net_agent.h"

#include <cinttypes>

#include "net_conn_client.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace NetManagerStandard;

CellularDataNetAgent::CellularDataNetAgent() = default;
CellularDataNetAgent::~CellularDataNetAgent() = default;

bool CellularDataNetAgent::RegisterNetSupplier()
{
    bool flag = false;
    for (auto &netSupplier : netSuppliers_) {
        auto netManager = DelayedSingleton<NetConnClient>::GetInstance();
        if (netManager == nullptr) {
            TELEPHONY_LOGE("NetConnClient is null");
            return false;
        }
        int32_t result = netManager->RegisterNetSupplier(
            netSupplier.netType, IDENT_PREFIX + std::to_string(netSupplier.slotId), netSupplier.capabilities);
        if (IPC_PROXY_ERR != result && result > 0) {
            TELEPHONY_LOGI("Register network successful");
            flag = true;
            netSupplier.supplierId = result;
        }
    }
    return flag;
}

void CellularDataNetAgent::UnregisterNetSupplier()
{
    for (auto netSupplier : netSuppliers_) {
        auto netManager = DelayedSingleton<NetConnClient>::GetInstance();
        if (netManager == nullptr) {
            TELEPHONY_LOGE("NetConnClient is null");
            return;
        }
        int32_t result = netManager->UnregisterNetSupplier(netSupplier.supplierId);
        TELEPHONY_LOGI("Unregister network result:%{public}d", result);
    }
    netSuppliers_.clear();
}

void CellularDataNetAgent::UpdateNetSupplierInfo(
    int32_t supplierId, sptr<NetManagerStandard::NetSupplierInfo> &netSupplierInfo)
{
    auto netManager = DelayedSingleton<NetConnClient>::GetInstance();
    if (netManager == nullptr) {
        TELEPHONY_LOGE("NetConnClient is null");
        return;
    }
    int32_t result = netManager->UpdateNetSupplierInfo(supplierId, netSupplierInfo);
    TELEPHONY_LOGI("Update network result:%{public}d", result);
}

void CellularDataNetAgent::UpdateNetLinkInfo(int32_t supplierId, sptr<NetManagerStandard::NetLinkInfo> &netLinkInfo)
{
    auto netManager = DelayedSingleton<NetConnClient>::GetInstance();
    if (netManager == nullptr) {
        TELEPHONY_LOGE("NetConnClient is null");
        return;
    }
    int32_t result = netManager->UpdateNetLinkInfo(supplierId, netLinkInfo);
    TELEPHONY_LOGI("result:%{public}d", result);
}

void CellularDataNetAgent::AddNetSupplier(const NetSupplier &netSupplier)
{
    netSuppliers_.push_back(netSupplier);
}

void CellularDataNetAgent::ClearNetSupplier()
{
    netSuppliers_.clear();
}

int32_t CellularDataNetAgent::GetSupplierId(const int32_t slotId, uint64_t capability) const
{
    for (const auto &netSupplier : netSuppliers_) {
        if (netSupplier.slotId == slotId && ((netSupplier.capabilities & capability) != 0)) {
            TELEPHONY_LOGI(
                "find supplierId %{public}d capability:%{public}" PRIu64"", netSupplier.supplierId, capability);
            return netSupplier.supplierId;
        }
    }
    return 0;
}

void CellularDataNetAgent::UpdateNetCapabilities(const int32_t slotId, uint64_t capability)
{
    for (auto &netSupplier : netSuppliers_) {
        if (netSupplier.slotId == slotId) {
            if (netSupplier.supplierId > 0) {
                auto netManager = DelayedSingleton<NetConnClient>::GetInstance();
                if (netManager == nullptr) {
                    TELEPHONY_LOGE("NetConnClient is null");
                    return;
                }
                int32_t result = netManager->UpdateNetCapabilities(netSupplier.supplierId, capability);
                if (IPC_PROXY_ERR != result) {
                    netSupplier.capabilities = capability;
                }
                TELEPHONY_LOGI("update %{public}d capability:%{public}" PRIu64" result:%{public}d",
                    netSupplier.supplierId, capability, result);
            } else {
                TELEPHONY_LOGE("update %{public}d slot:%{public}d not register", netSupplier.supplierId, slotId);
            }
            return;
        }
    }
    TELEPHONY_LOGE("update slot:%{public}d capabilities not find", slotId);
}
} // namespace Telephony
} // namespace OHOS