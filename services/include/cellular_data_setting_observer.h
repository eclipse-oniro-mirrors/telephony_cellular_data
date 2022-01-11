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

#ifndef CELLULAR_DATA_SETTING_OBSERVER_H
#define CELLULAR_DATA_SETTING_OBSERVER_H

#include "data_ability_observer_stub.h"

#include "cellular_data_handler.h"

namespace OHOS {
namespace Telephony {
class CellularDataSettingObserver : public AAFwk::DataAbilityObserverStub {
public:
    CellularDataSettingObserver(std::shared_ptr<CellularDataHandler> &cellularDataHandler);
    ~CellularDataSettingObserver();
    void OnChange() override;

private:
    std::shared_ptr<CellularDataHandler> cellularDataHandler_;
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_DATA_SETTING_OBSERVER_H