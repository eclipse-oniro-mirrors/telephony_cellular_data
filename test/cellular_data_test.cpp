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

#include <string>
#include <unistd.h>

#include "gtest/gtest.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "cellular_data_constant.h"
#include "cellular_data_error.h"
#include "cellular_data_types.h"
#include "core_manager.h"
#include "i_cellular_data_manager.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

class CellularDataTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    virtual void SetUp();
    virtual void TearDown();
    static int32_t IsCellularDataEnabledTest();
    static int32_t EnableCellularDataTest(bool enable);
    static int32_t GetCellularDataStateTest();
    static int32_t IsCellularDataRoamingEnabledTest(int32_t slotId);
    static int32_t EnableCellularDataRoamingTest(int32_t slotId, bool enable);
    static int32_t ReleaseNetTest(std::string ident, uint64_t capability);
    static int32_t RequestNetTest(std::string ident, uint64_t capability);
    static int32_t GetDefaultCellularDataSlotIdTest();
    static int32_t SetDefaultCellularDataSlotIdTest(int32_t slotId);
    static int32_t GetCellularDataFlowTypeTest();
    static sptr<ICellularDataManager> GetProxy();
    static int32_t HandleApnChangedTest(int32_t slotId, std::string apns);

public:
    static sptr<ICellularDataManager> proxy_;
    static const int SLEEP_TIME = 2;
};

sptr<ICellularDataManager> CellularDataTest::proxy_;

void CellularDataTest::TearDownTestCase()
{}

void CellularDataTest::SetUp()
{}

void CellularDataTest::TearDown()
{}

void CellularDataTest::SetUpTestCase()
{
    proxy_ = GetProxy();
    ASSERT_TRUE(proxy_ != nullptr);
    // Set the default slot
    auto result = proxy_->SetDefaultCellularDataSlotId(CoreManager::DEFAULT_SLOT_ID);
    ASSERT_TRUE(result == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto enable = proxy_->EnableCellularData(true);
    ASSERT_TRUE(enable == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
}

int32_t CellularDataTest::IsCellularDataRoamingEnabledTest(int32_t slotId)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    int32_t result = proxy_->IsCellularDataRoamingEnabled(slotId);
    return result;
}

int32_t CellularDataTest::IsCellularDataEnabledTest()
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    int32_t result = proxy_->IsCellularDataEnabled();
    return result;
}

int32_t CellularDataTest::EnableCellularDataTest(bool enable)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    auto result = proxy_->EnableCellularData(enable);
    return result;
}

int32_t CellularDataTest::GetCellularDataStateTest()
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    auto result = proxy_->GetCellularDataState();
    return result;
}

int32_t CellularDataTest::EnableCellularDataRoamingTest(int32_t slotId, bool enable)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->EnableCellularDataRoaming(slotId, enable);
}

int32_t CellularDataTest::ReleaseNetTest(std::string ident, uint64_t capability)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->ReleaseNet(std::move(ident), capability);
}

int32_t CellularDataTest::RequestNetTest(std::string ident, uint64_t capability)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->RequestNet(std::move(ident), capability);
}

int32_t CellularDataTest::GetDefaultCellularDataSlotIdTest()
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->GetDefaultCellularDataSlotId();
}

int32_t CellularDataTest::SetDefaultCellularDataSlotIdTest(int32_t slotId)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->SetDefaultCellularDataSlotId(slotId);
}

int32_t CellularDataTest::GetCellularDataFlowTypeTest()
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->GetCellularDataFlowType();
}

sptr<ICellularDataManager> CellularDataTest::GetProxy()
{
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        return nullptr;
    }
    sptr<IRemoteObject> remote = systemAbilityMgr->CheckSystemAbility(TELEPHONY_CELLULAR_DATA_SYS_ABILITY_ID);
    if (remote) {
        sptr<ICellularDataManager> dataManager = iface_cast<ICellularDataManager>(remote);
        return dataManager;
    }
    return nullptr;
}

int32_t CellularDataTest::HandleApnChangedTest(int32_t slotId, std::string apns)
{
    if (proxy_ == nullptr) {
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return proxy_->HandleApnChanged(slotId, apns);
}

HWTEST_F(CellularDataTest, GetProxy_Test, TestSize.Level1)
{
    CellularDataTest::proxy_ = CellularDataTest::GetProxy();
    ASSERT_FALSE(CellularDataTest::proxy_ == nullptr);
}

HWTEST_F(CellularDataTest, IsCellularDataEnabled_Test, TestSize.Level1)
{
    auto result = CellularDataTest::IsCellularDataEnabledTest();
    ASSERT_TRUE(result >= static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_DISABLED));
}

HWTEST_F(CellularDataTest, DefaultCellularDataSlotId_Test, TestSize.Level2)
{
    int32_t result = CellularDataTest::GetDefaultCellularDataSlotIdTest();
    ASSERT_TRUE(result >= 0 && result <= CoreManager::SLOT_ID2);
    result = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    ASSERT_TRUE(result == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    // Multiple cards will need to be optimized again
    result = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID - 1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
}

HWTEST_F(CellularDataTest, EnableCellularData_Test, TestSize.Level2)
{
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        int32_t disabled = CellularDataTest::EnableCellularDataTest(false);
        ASSERT_TRUE(disabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    } else {
        int32_t result = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(result == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    }
}

HWTEST_F(CellularDataTest, DataRoamingState_ValidSlot_Test_01, TestSize.Level2)
{
    // slot1 enable data roaming
    auto enabled = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID, true);
    ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::DEFAULT_SLOT_ID);
    ASSERT_TRUE(result == static_cast<int32_t>(RoamingSwitchCode::CELLULAR_DATA_ROAMING_ENABLED));

    // slot1 close
    auto enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID, false);
    ASSERT_TRUE(enable == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::DEFAULT_SLOT_ID);
    ASSERT_TRUE(result == static_cast<int32_t>(RoamingSwitchCode::CELLULAR_DATA_ROAMING_DISABLED));

    // At present, multiple card problems, the subsequent need to continue to deal with
    enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2, true);
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);

    enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2, false);
    // At present, multiple card problems, the subsequent need to continue to deal with
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
}

HWTEST_F(CellularDataTest, EnableCellularDataRoaming_ValidSlot_Test_01, TestSize.Level2)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto isDataRoaming = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::DEFAULT_SLOT_ID);
    if (isDataRoaming == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        auto result = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID, false);
        ASSERT_TRUE(result == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    } else {
        auto result = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID, true);
        ASSERT_TRUE(result == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    }
    // At present, multiple card problems, the subsequent need to continue to deal with
    isDataRoaming = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2);
    if (isDataRoaming == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        auto result = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2, false);
        ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    } else {
        auto result = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2, true);
        ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    }
}

HWTEST_F(CellularDataTest, GetCellularDataState_ValidityTest_01, TestSize.Level2)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID);
        uint64_t capability = 2;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(request == static_cast<int32_t>(RequestNetCode::REQUEST_SUCCESS));
        // check connection state
        auto result = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTED));
        auto release = CellularDataTest::ReleaseNetTest(ident, capability);
        ASSERT_TRUE(release == static_cast<int32_t>(ReleaseNetCode::RELEASE_SUCCESS));
        sleep(SLEEP_TIME);
        result = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_ValidSlot_Test_01, TestSize.Level2)
{
    // first to turn on cellular data switch
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto enabled = CellularDataTest::EnableCellularDataTest(true);
    ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
    if (enabledSwitch) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID);
        uint64_t capability = 1;
        CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        ASSERT_TRUE(request == static_cast<int32_t>(RequestNetCode::REQUEST_SUCCESS));
        sleep(SLEEP_TIME);
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTED));
        auto result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(result == static_cast<int32_t>(ReleaseNetCode::RELEASE_SUCCESS));
        state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    } else {
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_ValidSlot_Test_02, TestSize.Level2)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto enabled = CellularDataTest::EnableCellularDataTest(true);
    ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
    if (enabledSwitch) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID);
        uint64_t capability = 1;
        // first disconnect default connection
        CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        capability = 2;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(request == static_cast<int32_t>(RequestNetCode::REQUEST_SUCCESS));
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        auto result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(result == static_cast<int32_t>(ReleaseNetCode::RELEASE_SUCCESS));
        state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    } else {
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_ValidSlot_Test_03, TestSize.Level2)
{
    // Multiple cards will need to be optimized again
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2);
            uint64_t capability = 1;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_ValidSlot_Test_04, TestSize.Level2)
{
    // Multiple cards will need to be optimized again
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2);
            uint64_t capability = 2;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, EnableCellularDataRoaming_InValidSlot_Test_02, TestSize.Level3)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    auto isDataRoaming = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2 + 1);
    ASSERT_TRUE(isDataRoaming == CELLULAR_DATA_INVALID_PARAM);
    isDataRoaming = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID1 - 1);
    ASSERT_TRUE(isDataRoaming == CELLULAR_DATA_INVALID_PARAM);
}

HWTEST_F(CellularDataTest, DataRoamingState_InValidSlot_Test_01, TestSize.Level3)
{
    // invalid slot turn on data roaming
    auto enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID -1, true);
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    auto result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::DEFAULT_SLOT_ID -1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2 + 1, true);
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2 + 1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    // invalid slot disable roaming
    enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::DEFAULT_SLOT_ID -1, false);
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::DEFAULT_SLOT_ID -1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
    enable = CellularDataTest::EnableCellularDataRoamingTest(CoreManager::SLOT_ID2 + 1, false);
    ASSERT_TRUE(enable == CELLULAR_DATA_INVALID_PARAM);
    result = CellularDataTest::IsCellularDataRoamingEnabledTest(CoreManager::SLOT_ID2 + 1);
    ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidCapability_Test_01, TestSize.Level3)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto enabled = CellularDataTest::EnableCellularDataTest(true);
    ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
    if (enabledSwitch) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID);
        uint64_t capability = 1;
        auto result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        capability = -1;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        ASSERT_TRUE(request == static_cast<int32_t>(RequestNetCode::REQUEST_FAILED));
        sleep(SLEEP_TIME);
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state != static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTED));
        result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(result == static_cast<int32_t>(ReleaseNetCode::RELEASE_FAILED));
    } else {
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidCapability_Test_02, TestSize.Level3)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID);
    auto enabled = CellularDataTest::EnableCellularDataTest(true);
    ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
    auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
    if (enabledSwitch) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID);
        uint64_t capability = 1;
        auto result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        capability = 3;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        ASSERT_TRUE(request == static_cast<int32_t>(RequestNetCode::REQUEST_FAILED));
        sleep(SLEEP_TIME);
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state != static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTED));
        result = CellularDataTest::ReleaseNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(result == static_cast<int32_t>(ReleaseNetCode::RELEASE_FAILED));
    } else {
        auto state = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidCapability_Test_03, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2);
            uint64_t capability = 3;
            sleep(SLEEP_TIME);
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidCapability_Test_04, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2);
            uint64_t capability = -1;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidSlot_Test_01, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID1 -1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID1 - 1);
            uint64_t capability = 1;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidSlot_Test_02, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID1 - 1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID1 - 1);
            uint64_t capability = 2;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidSlot_Test_03, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2 + 1);
            uint64_t capability = 1;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_InValidSlot_Test_04, TestSize.Level3)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2 + 1);
            uint64_t capability = 2;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            sleep(SLEEP_TIME);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            sleep(SLEEP_TIME);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, GetCellularDataState_Double_Invalid_Test01, TestSize.Level4)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID - 1);
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    int32_t result  = static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID - 1);
        uint64_t capability = 3;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
        result = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    } else {
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, GetCellularDataState_Double_Invalid_Test02, TestSize.Level4)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    int32_t result  = static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2 + 1);
        uint64_t capability = 3;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
        sleep(SLEEP_TIME);
        result = CellularDataTest::GetCellularDataStateTest();
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    } else {
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, GetCellularDataState_Double_Invalid_Test03, TestSize.Level4)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::DEFAULT_SLOT_ID - 1);
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    int32_t result  = static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::DEFAULT_SLOT_ID - 1);
        uint64_t capability = -1;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
    } else {
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, GetCellularDataState_Double_Invalid_Test04, TestSize.Level4)
{
    CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    auto enabled = CellularDataTest::IsCellularDataEnabledTest();
    int32_t result  = static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
    if (enabled == static_cast<int32_t>(DataSwitchCode::CELLULAR_DATA_ENABLED)) {
        std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2 + 1);
        uint64_t capability = -1;
        auto request = CellularDataTest::RequestNetTest(ident, capability);
        sleep(SLEEP_TIME);
        ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
    } else {
        ASSERT_TRUE(result == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_DoubleValid_Test_01, TestSize.Level4)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID2 + 1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID2 + 1);
            uint64_t capability = -1;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}

HWTEST_F(CellularDataTest, RequestAndReleaseNet_DoubleValid_Test_02, TestSize.Level4)
{
    auto slotSet = CellularDataTest::SetDefaultCellularDataSlotIdTest(CoreManager::SLOT_ID1 - 1);
    if (slotSet != CELLULAR_DATA_INVALID_PARAM) {
        auto enabled = CellularDataTest::EnableCellularDataTest(true);
        ASSERT_TRUE(enabled == static_cast<int32_t>(DataRespondCode::SET_SUCCESS));
        auto enabledSwitch = CellularDataTest::IsCellularDataEnabledTest();
        if (enabledSwitch) {
            std::string ident = IDENT_PREFIX + std::to_string(CoreManager::SLOT_ID1 - 1);
            uint64_t capability = 3;
            auto request = CellularDataTest::RequestNetTest(ident, capability);
            ASSERT_TRUE(request == CELLULAR_DATA_INVALID_PARAM);
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
            auto result = CellularDataTest::ReleaseNetTest(ident, capability);
            ASSERT_TRUE(result == CELLULAR_DATA_INVALID_PARAM);
        } else {
            auto state = CellularDataTest::GetCellularDataStateTest();
            ASSERT_TRUE(state == static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED));
        }
    }
}
} // namespace Telephony
} // namespace OHOS