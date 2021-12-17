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

#include "sim_switcher.h"

namespace OHOS {
namespace Telephony {
SIMSwitcher::SIMSwitcher() = default;

SIMSwitcher::~SIMSwitcher() = default;

void SIMSwitcher::Activate() {}

void SIMSwitcher::DeActivate() {}

void SIMSwitcher::ProcessDefaultDataChanged() {}

void SIMSwitcher::OnActiveSimSwitch() {}
} // namespace Telephony
} // namespace OHOS