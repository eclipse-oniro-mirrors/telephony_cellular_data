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

#ifndef CELLULAR_DATA_SERVICE_H
#define CELLULAR_DATA_SERVICE_H

#include "iremote_object.h"
#include "nocopyable.h"
#include "singleton.h"
#include "system_ability.h"

#include "cellular_data_service_stub.h"
#include "cellular_data_constant.h"
#include "cellular_data_controller.h"
#include "traffic_management.h"
#include "sim_switcher.h"

namespace OHOS {
namespace Telephony {
enum class ServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING,
};

class CellularDataService : public SystemAbility, public CellularDataServiceStub {
    DECLARE_DELAYED_SINGLETON(CellularDataService)
    DECLARE_SYSTEM_ABILITY(CellularDataService)

public:
    /**
     * service Start
     */
    void OnStart() override;
    /**
     * service OnStop
     */
    void OnStop() override;
    int32_t Dump(std::int32_t fd, const std::vector<std::u16string>& args) override;
    std::string GetBindTime();
    std::string GetCellularDataSlotIdDump();
    std::string GetStateMachineCurrentStatusDump();
    std::string GetFlowDataInfoDump();
    int32_t IsCellularDataEnabled() override;
    int32_t EnableCellularData(bool enable) override;
    int32_t GetCellularDataState() override;
    int32_t IsCellularDataRoamingEnabled(const int32_t slotId) override;
    int32_t EnableCellularDataRoaming(const int32_t slotId, bool enable) override;
    int32_t ReleaseNet(const std::string ident, const uint64_t capability) override;
    int32_t RequestNet(const std::string ident, const uint64_t capability) override;
    int32_t HandleApnChanged(const int32_t slotId, std::string apns) override;
    int32_t GetDefaultCellularDataSlotId() override;
    int32_t SetDefaultCellularDataSlotId(const int32_t slotId) override;
    int32_t GetCellularDataFlowType() override;
    void RegisterAllNetSpecifier();
    void DispatchEvent(int32_t slotId, const AppExecFwk::InnerEvent::Pointer &event);
    int32_t StrategySwitch(bool enable) override;

private:
    void WaitCoreServiceToInit();
    bool Init();
    void InitModule();
    bool CheckParamValid(const int32_t slotId);
    void UnRegisterAllNetSpecifier();

private:
    std::map<int32_t, std::shared_ptr<CellularDataController>> cellularDataControllers_;
    std::unique_ptr<SIMSwitcher> simSwitcher_;
    std::shared_ptr<AppExecFwk::EventRunner> eventLoop_;
    bool registerToService_;
    int64_t bindTime_ = 0L;
    ServiceRunningState state_;
    static constexpr HiviewDFX::HiLogLabel LOG_LABEL = {LOG_CORE, LOG_DOMAIN, "CellularDataService"};
    constexpr static uint32_t CONNECT_MAX_TRY_COUNT = 120;
    constexpr static uint32_t CONNECT_SERVICE_WAIT_TIME = 1000; // ms
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_DATA_SERVICE_H
