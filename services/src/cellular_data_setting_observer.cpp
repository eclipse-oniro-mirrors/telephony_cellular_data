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

#include "cellular_data_setting_observer.h"
#include "cellular_data_constant.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CellularDataSettingObserver::CellularDataSettingObserver(std::shared_ptr<CellularDataHandler> &cellularDataHandler)
    : cellularDataHandler_(cellularDataHandler)
{}

CellularDataSettingObserver::~CellularDataSettingObserver() = default;

void CellularDataSettingObserver::OnChange(NativePreferences::Preferences &preferences, const std::string &key)
{
    auto util = DelayedSingleton<Telephony::TelProfileUtil>::GetInstance();
    if (util == nullptr) {
        TELEPHONY_LOGE("util is null");
        return;
    }
}
} // namespace Telephony
} // namespace OHOS