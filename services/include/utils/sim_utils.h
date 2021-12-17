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

#ifndef SIM_UTILS_H
#define SIM_UTILS_H

#include "event_handler.h"
#include "inner_event.h"
#include "i_net_conn_service.h"

#include "core_manager.h"

namespace OHOS {
namespace Telephony {
class SimUtils {
public:
    static int32_t GetSimState(int32_t slotId);
    static std::u16string GetSimIccId(const int32_t slotId);
    static int32_t GetDefaultCellularDataSlotId();
    static int32_t SetDefaultCellularDataSlotId(int32_t slotId);
    static int32_t GetSimNum();

private:
    SimUtils() = default;
    ~SimUtils() = default;
};
} // namespace Telephony
} // namespace OHOS
#endif // SIM_UTILS_H
