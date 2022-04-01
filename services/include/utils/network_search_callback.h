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

#ifndef NETWORK_SEARCH_CALLBACK_H
#define NETWORK_SEARCH_CALLBACK_H

#include <string>

#include "network_search_callback_base.h"

namespace OHOS {
namespace Telephony {
class NetworkSearchCallback : public NetworkSearchCallBackBase {
public:
    bool HasInternetCapability(int32_t slotId, int32_t cId);
    void ClearCellularDataConnections(int32_t slotId);
};
} // namespace Telephony
} // namespace OHOS
#endif // NETWORK_SEARCH_CALLBACK_H
