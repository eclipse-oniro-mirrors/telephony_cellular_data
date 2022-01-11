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

#ifndef TELEPHONY_CELLULAR_DATA_SETTINGS_RDB_HELPER_H
#define TELEPHONY_CELLULAR_DATA_SETTINGS_RDB_HELPER_H

#include <memory>
#include <singleton.h>
#include <utility>

#include "abs_shared_result_set.h"
#include "data_ability_helper.h"
#include "data_ability_predicates.h"
#include "iservice_registry.h"
#include "result_set.h"
#include "system_ability_definition.h"
#include "uri.h"
#include "values_bucket.h"

namespace OHOS {
namespace Telephony {
class CellularDataSettingsRdbHelper : public DelayedSingleton<CellularDataSettingsRdbHelper> {
    DECLARE_DELAYED_SINGLETON(CellularDataSettingsRdbHelper);
public:
    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     */
    void RegisterSettingsObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);

    /**
     * Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     */
    void UnRegisterSettingsObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);

    /**
     * Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     */
    void NotifyChange(const Uri &uri);
    int GetValue(Uri &uri, const std::string &column);
    void PutValue(Uri &uri, const std::string &column, int value);
    std::shared_ptr<AppExecFwk::DataAbilityHelper> CreateDataAbilityHelper();

private:
    std::shared_ptr<AppExecFwk::DataAbilityHelper> settingHelper_;
};
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_CELLULAR_DATA_SETTINGS_RDB_HELPER_H
