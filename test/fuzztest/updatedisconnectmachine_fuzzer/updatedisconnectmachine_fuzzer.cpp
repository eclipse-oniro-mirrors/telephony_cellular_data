/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "updatedisconnectmachine_fuzzer.h"

#define private public
#define protected public

#include "adddatatoken_fuzzer.h"
#include "default.h"
#include "disconnecting.h"
#include "statemachine_fuzzer.h"
#include "common_event_manager.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
using namespace OHOS::Telephony;
using namespace AppExecFwk;
using namespace OHOS::EventFwk;
constexpr int32_t SLOT_NUM_MAX = 3;
constexpr int32_t EVENT_ID_MAX = 255;

void UpdateDisconnectMachineFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    int32_t slotId = fdp.ConsumeIntegralInRange<int32_t>(0, SLOT_NUM_MAX);
    static std::vector<std::shared_ptr<StateMachineFuzzer>> machines(SLOT_NUM_MAX + 1);
    static std::vector<std::shared_ptr<CellularDataStateMachine>> cellularMachines(SLOT_NUM_MAX + 1);
    if (machines[slotId] == nullptr) {
        machines[slotId] = std::make_shared<StateMachineFuzzer>();
        cellularMachines[slotId] = machines[slotId]->CreateCellularDataConnect(slotId);
        if (cellularMachines[slotId] == nullptr) {
            return;
        }
        cellularMachines[slotId]->Init();
    }
    std::shared_ptr<CellularDataStateMachine> cellularMachine = cellularMachines[slotId];
    if (cellularMachine == nullptr) {
        return;
    }
    auto disconnecting =
        std::make_shared<Disconnecting>(std::weak_ptr<CellularDataStateMachine>(cellularMachine), "Disconnecting");
    auto defaultStatus = std::make_shared<Default>(std::weak_ptr<CellularDataStateMachine>(cellularMachine), "Default");

    if (disconnecting == nullptr || defaultStatus == nullptr) {
        TELEPHONY_LOGE("memory allocation failed");
        return;
    }

    std::int32_t intValue = fdp.ConsumeIntegralInRange<int32_t>(0, EVENT_ID_MAX);
    std::unique_ptr<uint8_t> object = std::make_unique<uint8_t>(*data);
    if (object == nullptr) {
        return;
    }

    InnerEvent::Pointer event = InnerEvent::Get(intValue, object);
    disconnecting->StateProcess(event);
    disconnecting->ProcessDisconnectTimeout(event);

    defaultStatus->StateProcess(event);
    defaultStatus->ProcessConnectDone(event);
    defaultStatus->ProcessDisconnectAllDone(event);
    defaultStatus->ProcessDataConnectionDrsOrRatChanged(event);
    defaultStatus->ProcessDataConnectionRoamOn(event);
    defaultStatus->ProcessDataConnectionRoamOff(event);
}

void UpdateDisconnectMachineWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    UpdateDisconnectMachineFuzz(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddDataTokenFuzzer token;
    /* Run your code on data */
    OHOS::UpdateDisconnectMachineWithMyAPI(data, size);
    return 0;
}
