/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define private public
#define protected public

#include "apn_holder.h"
#include "apn_manager.h"
#include "cellular_data_rdb_helper.h"
#include "cellular_data_state_machine.h"
#include "cellular_data_client.h"
#include "cellular_data_constant.h"
#include "core_manager_inner.h"
#include "data_connection_manager.h"
#include "gtest/gtest.h"
#include "mock/mock_sim_manager.h"
#include "mock/mock_network_search.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "tel_event_handler.h"
#include "pdp_profile_data.h"
#include "telephony_ext_wrapper.h"
#include "utils/scoped_setter.h"
#include "utils/test_async_utils.h"
#include <map>

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgReferee;

class ApnManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<ApnManager> apnManager;
};
void ApnManagerTest::SetUpTestCase() {}

void ApnManagerTest::TearDownTestCase() {}

void ApnManagerTest::SetUp()
{
    apnManager = std::make_shared<ApnManager>();
}

void ApnManagerTest::TearDown()
{
    apnManager.reset();
}

class StateMachineTest : public TelEventHandler {
public:
    StateMachineTest() : TelEventHandler("StateMachineTest") {}
    ~StateMachineTest()
    {
        // 用例结束先停机排空 FFRT 队列再析构，规避 TelEventQueue 生命周期竞态（DT Signal 6）
        if (cellularDataStateMachine_ != nullptr) {
            if (cellularDataStateMachine_->cdConnectionManager_ != nullptr) {
                TestAsyncUtils::DrainStateMachine(cellularDataStateMachine_->cdConnectionManager_.get());
            }
            TestAsyncUtils::DrainStateMachine(cellularDataStateMachine_.get());
            cellularDataStateMachine_ = nullptr;
        }
        TestAsyncUtils::DrainTelEventHandler(this);
    }
    std::shared_ptr<CellularDataStateMachine> CreateCellularDataStateMachine(int32_t slotId);

public:
    std::shared_ptr<CellularDataStateMachine> cellularDataStateMachine_ = nullptr;
};

std::shared_ptr<CellularDataStateMachine> StateMachineTest::CreateCellularDataStateMachine(int32_t slotId)
{
    if (cellularDataStateMachine_ != nullptr) {
        return cellularDataStateMachine_;
    }
    auto connectionManager = std::make_shared<DataConnectionManager>(slotId);
    if (connectionManager == nullptr) {
        return nullptr;
    }
    connectionManager->Init();
    cellularDataStateMachine_ = std::make_shared<CellularDataStateMachine>(
        connectionManager, std::static_pointer_cast<TelEventHandler>(shared_from_this()));
    return cellularDataStateMachine_;
}

/**
 * @tc.number   FindApnNameByApnId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnNameByApnId_001, Function | MediumTest | Level1)
{
    int32_t id = 1;
    std::string result = apnManager->FindApnNameByApnId(id);
    ASSERT_EQ(result, DATA_CONTEXT_ROLE_DEFAULT);
}

/**
 * @tc.number   FindApnNameByApnId_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnNameByApnId_002, Function | MediumTest | Level1)
{
    int32_t id = 2;
    std::string result = apnManager->FindApnNameByApnId(id);
    ASSERT_EQ(result, DATA_CONTEXT_ROLE_MMS);
}

/**
 * @tc.number   FindApnNameByApnId_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnNameByApnId_003, Function | MediumTest | Level1)
{
    std::string result = apnManager->FindApnNameByApnId(DATA_CONTEXT_ROLE_BIP_ID);
    ASSERT_EQ(result, DATA_CONTEXT_ROLE_BIP);
}

/**
 * @tc.number   FindApnIdByCapability_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_001, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET;
    int32_t expected = DATA_CONTEXT_ROLE_DEFAULT_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_002, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_MMS;
    int32_t expected = DATA_CONTEXT_ROLE_MMS_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_003, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNAL_DEFAULT;
    int32_t expected = DATA_CONTEXT_ROLE_INTERNAL_DEFAULT_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_004, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_IA;
    int32_t expected = DATA_CONTEXT_ROLE_IA_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_005, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_XCAP;
    int32_t expected = DATA_CONTEXT_ROLE_XCAP_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_006
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_006, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SUPL;
    int32_t expected = DATA_CONTEXT_ROLE_SUPL_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_007
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_007, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_DUN;
    int32_t expected = DATA_CONTEXT_ROLE_DUN_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_008
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_008, Function | MediumTest | Level1)
{
    uint64_t capability = 100;
    int32_t expected = DATA_CONTEXT_ROLE_INVALID_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_009, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_BIP;
    int32_t expected = DATA_CONTEXT_ROLE_BIP_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_010, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI1;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI1_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_011, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI2;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI2_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_012, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI3;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI3_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_013, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI4;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI4_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_014, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI5;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI5_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindApnIdByCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindApnIdByCapability_015, Function | MediumTest | Level1)
{
    uint64_t capability = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI6;
    int32_t expected = DATA_CONTEXT_ROLE_SNSSAI6_ID;
    int32_t actual = apnManager->FindApnIdByCapability(capability);
    ASSERT_EQ(actual, expected);
}

/**
 * @tc.number   FindBestCapability_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_001, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SUPL;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SUPL);
}

/**
 * @tc.number   FindBestCapability_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_002, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_DUN;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_DUN);
}

/**
 * @tc.number   FindBestCapability_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_003, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_XCAP;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_XCAP);
}

/**
 * @tc.number   FindBestCapability_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_004, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_IA;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_IA);
}

/**
 * @tc.number   FindBestCapability_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_005, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_BIP;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_BIP);
}

/**
 * @tc.number   CreateMvnoApnItems_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, CreateMvnoApnItems_001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::string mcc = "460";
    std::string mnc = "00";
    int32_t result = apnManager->CreateMvnoApnItems(slotId, mcc, mnc);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.number   IsPreferredApnUserEdited_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsPreferredApnUserEdited_001, Function | MediumTest | Level1)
{
    auto preferId = 1;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->attr_.profileId_ = preferId;
    defaultApnItem->attr_.isEdited_ = true;
    apnManager->allApnItem_.push_back(defaultApnItem);
    apnManager->preferId_ = preferId;
    ASSERT_TRUE(apnManager->IsPreferredApnUserEdited());
}

/**
 * @tc.number   IsPreferredApnUserEdited_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsPreferredApnUserEdited_002, Function | MediumTest | Level1)
{
    auto preferId = 1;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->attr_.profileId_ = preferId;
    defaultApnItem->attr_.isEdited_ = false;
    apnManager->allApnItem_.push_back(defaultApnItem);
    apnManager->preferId_ = preferId;
    ASSERT_FALSE(apnManager->IsPreferredApnUserEdited());
}

/**
 * @tc.number   IsPreferredApnUserEdited_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsPreferredApnUserEdited_003, Function | MediumTest | Level1)
{
    auto preferId = 2;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->attr_.profileId_ = 3;
    defaultApnItem->attr_.isEdited_ = true;
    apnManager->allApnItem_.push_back(defaultApnItem);
    apnManager->preferId_ = preferId;
    ASSERT_FALSE(apnManager->IsPreferredApnUserEdited());
}

/**
 * @tc.number   IsDataConnectionNotUsed_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsDataConnectionNotUsed_001, Function | MediumTest | Level1)
{
    std::shared_ptr<CellularDataStateMachine> stateMachine = nullptr;
    bool result = apnManager->IsDataConnectionNotUsed(stateMachine);
    ASSERT_FALSE(result);
}

/**
 * @tc.number   IsDataConnectionNotUsed_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsDataConnectionNotUsed_002, Function | MediumTest | Level1)
{
    std::shared_ptr<StateMachineTest> machine = std::make_shared<StateMachineTest>();
    auto stateMachine = machine->CreateCellularDataStateMachine(0);
    apnManager->apnHolders_.push_back(nullptr);
    bool result = apnManager->IsDataConnectionNotUsed(stateMachine);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   IsDataConnectionNotUsed_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsDataConnectionNotUsed_003, Function | MediumTest | Level1)
{
    std::shared_ptr<StateMachineTest> machine = std::make_shared<StateMachineTest>();
    auto stateMachine = machine->CreateCellularDataStateMachine(0);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    apnHolder->SetCellularDataStateMachine(stateMachine);
    apnManager->apnHolders_.push_back(apnHolder);
    bool result = apnManager->IsDataConnectionNotUsed(stateMachine);
    ASSERT_FALSE(result);
}

/**
 * @tc.number   IsDataConnectionNotUsed_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsDataConnectionNotUsed_004, Function | MediumTest | Level1)
{
    std::shared_ptr<StateMachineTest> machine = std::make_shared<StateMachineTest>();
    auto stateMachine = machine->CreateCellularDataStateMachine(0);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    apnHolder->SetCellularDataStateMachine(nullptr);
    apnManager->apnHolders_.push_back(apnHolder);
    bool result = apnManager->IsDataConnectionNotUsed(stateMachine);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   IsDataConnectionNotUsed_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsDataConnectionNotUsed_005, Function | MediumTest | Level1)
{
    std::shared_ptr<StateMachineTest> machine = std::make_shared<StateMachineTest>();
    auto stateMachine = machine->CreateCellularDataStateMachine(0);
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    apnHolder->SetCellularDataStateMachine(stateMachine);
    apnManager->apnHolders_.push_back(apnHolder);
    machine->cellularDataStateMachine_ = nullptr;
    auto stateMachine_1 = machine->CreateCellularDataStateMachine(0);
    bool result = apnManager->IsDataConnectionNotUsed(stateMachine_1);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   MakeSpecificApnItem_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, MakeSpecificApnItem_001, Function | MediumTest | Level1)
{
    auto preferId = 1;
    apnManager->preferId_ = preferId;
    PdpProfile pdpProfile;
    pdpProfile.profileId = preferId;
    pdpProfile.apnTypes = "";
    std::vector<PdpProfile> apnVec;
    apnVec.push_back(pdpProfile);
    bool result = apnManager->MakeSpecificApnItem(apnVec, 0);
    ASSERT_EQ(result, 1);
}

/**
 * @tc.number   MakeSpecificApnItem_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, MakeSpecificApnItem_002, Function | MediumTest | Level1)
{
    auto preferId = 1;
    apnManager->preferId_ = preferId;
    PdpProfile pdpProfile;
    pdpProfile.profileId = preferId;
    pdpProfile.apnTypes = "default";
    std::vector<PdpProfile> apnVec;
    apnVec.push_back(pdpProfile);
    bool result = apnManager->MakeSpecificApnItem(apnVec, 0);
    ASSERT_EQ(result, 1);
}

/**
 * @tc.number   MakeSpecificApnItem_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, MakeSpecificApnItem_003, Function | MediumTest | Level1)
{
    auto preferId = 1;
    apnManager->preferId_ = 2;
    PdpProfile pdpProfile;
    pdpProfile.profileId = preferId;
    pdpProfile.apnTypes = "default";
    std::vector<PdpProfile> apnVec;
    apnVec.push_back(pdpProfile);
    bool result = apnManager->MakeSpecificApnItem(apnVec, 0);
    ASSERT_EQ(result, 1);
}

/**
 * @tc.number   HasAnyConnectedState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_001, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnHolder>> apnHolders;
    apnManager->apnHolders_ = apnHolders;
    bool result = apnManager->HasAnyConnectedState();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   HasAnyConnectedState_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_002, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnHolder>> apnHolders;
    sptr<ApnHolder> apnHolder = nullptr;
    apnHolders.push_back(apnHolder);
    apnManager->apnHolders_ = apnHolders;
    bool result = apnManager->HasAnyConnectedState();
    ASSERT_EQ(result, false);
}

/**
 * @tc.number   HasAnyConnectedState_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_003, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnHolder>> apnHolders;
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->SetApnState(ApnProfileState::PROFILE_STATE_CONNECTED);
    apnHolders.push_back(apnHolder);
    apnManager->apnHolders_ = apnHolders;
    bool result = apnManager->HasAnyConnectedState();
    ASSERT_EQ(result, true);
}

/**
 * @tc.number   HasAnyConnectedState_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_004, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnHolder>> apnHolders;
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->SetApnState(ApnProfileState::PROFILE_STATE_DISCONNECTING);
    apnHolders.push_back(apnHolder);
    apnManager->apnHolders_ = apnHolders;
    bool result = apnManager->HasAnyConnectedState();
    ASSERT_EQ(result, true);
}

/**
 * @tc.number   GetRilAttachApn_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_001, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnItem>> allApnItem;
    apnManager->allApnItem_ = allApnItem;
    ASSERT_EQ(apnManager->GetRilAttachApn(), nullptr);
}

/**
 * @tc.number   GetRilAttachApn_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_002, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    allApnItem.push_back(defaultApnItem);
    apnManager->allApnItem_ = allApnItem;
    ASSERT_NE(apnManager->GetRilAttachApn(), nullptr);
}

/**
 * @tc.number   GetRilAttachApn_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_003, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_IA);
    allApnItem.push_back(apnItem);
    apnManager->allApnItem_ = allApnItem;
    ASSERT_NE(apnManager->GetRilAttachApn(), nullptr);
}

/**
 * @tc.number   GetRilAttachApn_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_004, Function | MediumTest | Level1)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> apnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS);
    allApnItem.push_back(apnItem);
    apnManager->allApnItem_ = allApnItem;
    ASSERT_NE(apnManager->GetRilAttachApn(), nullptr);
}

/**
 * @tc.number   ReleaseDataConnection_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ReleaseDataConnection_001, TestSize.Level0)
{
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->cellularDataStateMachine_ = nullptr;
    apnHolder->ReleaseDataConnection();
    ASSERT_EQ(apnHolder->cellularDataStateMachine_, nullptr);
}

/**
 * @tc.number   ReleaseDataConnection_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ReleaseDataConnection_002, TestSize.Level0)
{
    std::shared_ptr<StateMachineTest> machine = std::make_shared<StateMachineTest>();
    auto stateMachine = machine->CreateCellularDataStateMachine(0);
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->SetCellularDataStateMachine(stateMachine);
    apnHolder->ReleaseDataConnection();
    ASSERT_NE(apnHolder->cellularDataStateMachine_, nullptr);
}

/**
 * @tc.number   RequestCellularData_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, RequestCellularData_001, TestSize.Level0)
{
    NetRequest netRequest;
    netRequest.capability = 0;
    netRequest.ident = "ident";
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->RequestCellularData(netRequest);
    ASSERT_EQ(apnHolder->dataCallEnabled_, true);
}

/**
 * @tc.number   RequestCellularData_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, RequestCellularData_002, TestSize.Level0)
{
    NetRequest netRequest;
    netRequest.capability = 0;
    netRequest.ident = "ident";
    NetRequest netRequest1;
    netRequest1.capability = 0;
    netRequest1.ident = "abc";
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->RequestCellularData(netRequest);
    ASSERT_EQ(apnHolder->dataCallEnabled_, true);
}

/**
 * @tc.number   RequestCellularData_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, RequestCellularData_003, TestSize.Level0)
{
    NetRequest netRequest;
    netRequest.capability = 0;
    netRequest.ident = "ident";
    NetRequest netRequest1;
    netRequest1.capability = 1;
    netRequest1.ident = "ident";
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->RequestCellularData(netRequest);
    ASSERT_EQ(apnHolder->dataCallEnabled_, true);
}

/**
 * @tc.number   RequestCellularData_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, RequestCellularData_004, TestSize.Level0)
{
    NetRequest netRequest;
    netRequest.capability = 1;
    netRequest.ident = "ident";
    sptr<ApnHolder> apnHolder = new ApnHolder("", 0);
    apnHolder->RequestCellularData(netRequest);
    ASSERT_EQ(apnHolder->dataCallEnabled_, true);
}

/**
 * @tc.number   IsCompatibleApnItem_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsCompatibleApnItem_001, TestSize.Level0)
{
    sptr<ApnItem> newApnItem = nullptr;
    sptr<ApnItem> oldApnItem = nullptr;
    bool roamingState = false;
    bool result = ApnHolder::IsCompatibleApnItem(newApnItem, oldApnItem, roamingState);
    ASSERT_FALSE(result);
}

/**
 * @tc.number   IsCompatibleApnItem_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsCompatibleApnItem_002, TestSize.Level0)
{
    sptr<ApnItem> newApnItem = new ApnItem();
    sptr<ApnItem> oldApnItem = new ApnItem();
    std::strcmp(newApnItem->attr_.roamingProtocol_, "test_protocol");
    std::strcmp(newApnItem->attr_.roamingProtocol_, "test_protocol");
    bool roamingState = true;
    bool result = ApnHolder::IsCompatibleApnItem(newApnItem, oldApnItem, roamingState);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   IsCompatibleApnItem_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsCompatibleApnItem_003, TestSize.Level0)
{
    sptr<ApnItem> newApnItem = new ApnItem();
    sptr<ApnItem> oldApnItem = new ApnItem();
    std::strcmp(newApnItem->attr_.roamingProtocol_, "test_protocol");
    std::strcmp(newApnItem->attr_.roamingProtocol_, "test_protocol");
    bool roamingState = false;
    bool result = ApnHolder::IsCompatibleApnItem(newApnItem, oldApnItem, roamingState);
    ASSERT_TRUE(result);
}

/**
 * @tc.number   GetNextRetryApnItem001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->matchedApns_.clear();
    EXPECT_EQ(connectionRetryPolicy->GetNextRetryApnItem(), nullptr);

    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(true);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    connectionRetryPolicy->currentApnIndex_ = 0;
    EXPECT_EQ(connectionRetryPolicy->GetNextRetryApnItem(), nullptr);
    defaultApnItem->MarkBadApn(false);
    EXPECT_NE(connectionRetryPolicy->GetNextRetryApnItem(), nullptr);
}

/**
 * @tc.number   GetNextRetryApnItem002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->currentApnIndex_ = 10;
    EXPECT_EQ(connectionRetryPolicy->currentApnIndex_, 10);
}

/**
 * @tc.number   GetNextRetryApnItem_003
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_003, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->currentApnIndex_ = -1;
    EXPECT_EQ(connectionRetryPolicy->currentApnIndex_, -1);
}

/**
 * @tc.number   GetNextRetryApnItem_004
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_004, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(true);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    connectionRetryPolicy->currentApnIndex_ = 0;
    EXPECT_EQ(connectionRetryPolicy->currentApnIndex_, 0);
}

/**
 * @tc.number   GetNextRetryApnItem_005
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_005, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(false);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    connectionRetryPolicy->tryCount_ = 10;
    connectionRetryPolicy->maxCount_ = 0;
    connectionRetryPolicy->currentApnIndex_ = 0;
    EXPECT_EQ(connectionRetryPolicy->currentApnIndex_, 0);
}

/**
 * @tc.number   GetNextRetryApnItem_006
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_006, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(false);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    connectionRetryPolicy->tryCount_ = -1;
    connectionRetryPolicy->maxCount_ = 0;
    connectionRetryPolicy->currentApnIndex_ = 0;
    EXPECT_EQ(connectionRetryPolicy->currentApnIndex_, 0);
}

/**
 * @tc.number   GetNextRetryApnItem_007
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_007, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->matchedApns_.clear();
    connectionRetryPolicy->currentApnIndex_ = 0;
    EXPECT_EQ(connectionRetryPolicy->GetNextRetryApnItem(), nullptr);
}

/**
 * @tc.number   OnPropChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, OnPropChanged_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->OnPropChanged(nullptr, nullptr, nullptr);
    EXPECT_EQ(connectionRetryPolicy->isPropOn_, true);
    connectionRetryPolicy->OnPropChanged("persist.telephony.retrystrategy.allow", nullptr, nullptr);
    EXPECT_EQ(connectionRetryPolicy->isPropOn_, true);
    connectionRetryPolicy->OnPropChanged("persist.telephony.retrystrategy.allow", "true", nullptr);
    EXPECT_EQ(connectionRetryPolicy->isPropOn_, true);
    connectionRetryPolicy->OnPropChanged("fakeKey", nullptr, nullptr);
    EXPECT_EQ(connectionRetryPolicy->isPropOn_, true);
    connectionRetryPolicy->OnPropChanged("fakeKey", "true", nullptr);
    EXPECT_EQ(connectionRetryPolicy->isPropOn_, true);
    connectionRetryPolicy->OnPropChanged("persist.telephony.setupfail.delay", "3000", nullptr);
    EXPECT_EQ(connectionRetryPolicy->defaultSetupFailDelay_, 3000);
    connectionRetryPolicy->OnPropChanged("persist.telephony.setupfail.delay", "ABC", nullptr);
    EXPECT_EQ(connectionRetryPolicy->defaultSetupFailDelay_, 3000);
    connectionRetryPolicy->OnPropChanged("persist.telephony.modemdend.delay", "1000", nullptr);
    EXPECT_EQ(connectionRetryPolicy->defaultModemDendDelay_, 1000);
    connectionRetryPolicy->OnPropChanged("persist.telephony.modemdend.delay", "ABNC", nullptr);
    EXPECT_EQ(connectionRetryPolicy->defaultModemDendDelay_, 1000);
}

/**
 * @tc.number   IsAllBadApn_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsAllBadApn_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->matchedApns_.clear();
    EXPECT_TRUE(connectionRetryPolicy->IsAllBadApn());
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(false);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    EXPECT_FALSE(connectionRetryPolicy->IsAllBadApn());
    defaultApnItem->MarkBadApn(true);
    EXPECT_TRUE(connectionRetryPolicy->IsAllBadApn());
}

/**
 * @tc.number   GetNextRetryDelay_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryDelay_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->matchedApns_.clear();
    auto delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0,
        RetryScene::RETRY_SCENE_OTHERS, true);
    EXPECT_GE(delay, 0);
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(true);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0, RetryScene::RETRY_SCENE_OTHERS,
        true);
    EXPECT_GE(delay, 0);
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT, 0, 0,
        RetryScene::RETRY_SCENE_OTHERS, true);
    EXPECT_GE(delay, 60);
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT, 0, 0,
        RetryScene::RETRY_SCENE_OTHERS, false);
    EXPECT_GE(delay, 5);
}

/**
 * @tc.number   GetNextRetryDelay_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetNextRetryDelay_002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->matchedApns_.clear();
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    defaultApnItem->MarkBadApn(false);
    connectionRetryPolicy->matchedApns_.push_back(defaultApnItem);
    auto delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0,
        RetryScene::RETRY_SCENE_OTHERS, true);
    EXPECT_GE(delay, 0);
    connectionRetryPolicy->isPropOn_ = false;
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0, RetryScene::RETRY_SCENE_OTHERS,
        true);
    EXPECT_GE(delay, 0);
    connectionRetryPolicy->isPropOn_ = true;
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0, RetryScene::RETRY_SCENE_OTHERS,
        true);
    EXPECT_GE(delay, 0);
    delay = connectionRetryPolicy->GetNextRetryDelay(DATA_CONTEXT_ROLE_MMS, 0, 0, RetryScene::RETRY_SCENE_OTHERS,
        true);
    EXPECT_GE(delay, 0);
}

/**
 * @tc.number   SetMatchedApns_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, SetMatchedApns_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    std::vector<sptr<ApnItem>> matchedApns = {};
    connectionRetryPolicy->SetMatchedApns(matchedApns);
    EXPECT_EQ(connectionRetryPolicy->GetMatchedApns().size(), 0);
}

/**
 * @tc.number   GetRandomDelay_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetRandomDelay_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    EXPECT_GE(connectionRetryPolicy->GetRandomDelay(), 0);
    EXPECT_LE(connectionRetryPolicy->GetRandomDelay(), 2000);
}

/**
 * @tc.number   ConvertPdpErrorToDisconnReason_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ConvertPdpErrorToDisconnReason_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    auto res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_OPERATOR_DETERMINED_BARRING);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_MISSING_OR_UNKNOWN_APN);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_UNKNOWN_PDP_ADDR_OR_TYPE);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_USER_VERIFICATION);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_ACTIVATION_REJECTED_GGSN);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_SERVICE_OPTION_NOT_SUPPORTED);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
}

/**
 * @tc.number   ConvertPdpErrorToDisconnReason_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ConvertPdpErrorToDisconnReason_002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    auto res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_NSAPI_ALREADY_USED);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_IPV4_ONLY_ALLOWED);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_IPV6_ONLY_ALLOWED);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_PROTOCOL_ERRORS);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_RETRY);
    EXPECT_EQ(res, DisConnectionReason::REASON_RETRY_CONNECTION);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_NORMAL);
    EXPECT_EQ(res, DisConnectionReason::REASON_NORMAL);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_GSM_AND_CALLING_ONLY);
    EXPECT_EQ(res, DisConnectionReason::REASON_GSM_AND_CALLING_ONLY);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_CLEAR_CONNECTION);
    EXPECT_EQ(res, DisConnectionReason::REASON_CLEAR_CONNECTION);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_CHANGE_CONNECTION);
    EXPECT_EQ(res, DisConnectionReason::REASON_CHANGE_CONNECTION);
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_PERMANENT_REJECT);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
}

/**
@tc.number ConvertPdpErrorToDisconnReason_003
@tc.name test function branch
@tc.desc Function test
*/
HWTEST_F(ApnManagerTest, ConvertPdpErrorToDisconnReason_003, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->isPropOn_ = false;
    auto res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_PERMANENT_REJECT);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
    connectionRetryPolicy->isPropOn_ = true;
    res = connectionRetryPolicy->ConvertPdpErrorToDisconnReason(
        PdpErrorReason::PDP_ERR_TO_PERMANENT_REJECT);
    EXPECT_EQ(res, DisConnectionReason::REASON_PERMANENT_REJECT);
}

/**

@tc.number RestartRadioIfRequired_001
@tc.name test function branch
@tc.desc Function test
*/
HWTEST_F(ApnManagerTest, RestartRadioIfRequired_003, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->isPropOn_ = true;
    int32_t failCause = 65536;
    connectionRetryPolicy->RestartRadioIfRequired(failCause, 0);
    connectionRetryPolicy->isPropOn_ = false;
    connectionRetryPolicy->RestartRadioIfRequired(failCause, 0);
    EXPECT_FALSE(connectionRetryPolicy->isPropOn_);
}

/**
 * @tc.number   InitialRetryCountValue_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, InitialRetryCountValue_001, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> connectionRetryPolicy = std::make_shared<ConnectionRetryPolicy>();
    connectionRetryPolicy->InitialRetryCountValue();
    EXPECT_EQ(connectionRetryPolicy->tryCount_, 0);
    connectionRetryPolicy->isPropOn_ = true;
    connectionRetryPolicy->InitialRetryCountValue();
    EXPECT_EQ(connectionRetryPolicy->tryCount_, 0);
    connectionRetryPolicy->isPropOn_ = false;
    connectionRetryPolicy->InitialRetryCountValue();
    EXPECT_EQ(connectionRetryPolicy->tryCount_, 0);
}

/**
 * @tc.number   EnableCellularDataRoaming_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, EnableCellularDataRoaming_001, TestSize.Level0)
{
    int32_t result = CellularDataClient::GetInstance().EnableCellularDataRoaming(0, true);
    EXPECT_NE(result, true);
}

/**
 * @tc.number   HasInternetCapability_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HasInternetCapability_001, TestSize.Level0)
{
    int32_t result = CellularDataClient::GetInstance().HasInternetCapability(0, 0);
    EXPECT_EQ(result, false);
}

/**
 * @tc.number   HandleApnChanged_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, HandleApnChanged_001, TestSize.Level0)
{
    int32_t result = CellularDataClient::GetInstance().HandleApnChanged(0);
    EXPECT_NE(result, true);
}

/**
 * @tc.number   UpdateDefaultCellularDataSlotId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, UpdateDefaultCellularDataSlotId_001, TestSize.Level0)
{
    int32_t result = CellularDataClient::GetInstance().UpdateDefaultCellularDataSlotId();
    EXPECT_GE(result, 0);
}

/**
 * @tc.name  : ApnItem_CanDealWithType_001
 * @tc.number: ApnItemTest_001
 * @tc.desc  : Test when the type matches with the apnTypes_ then CanDealWithType returns true
 */
HWTEST_F(ApnManagerTest, ApnItem_CanDealWithType_001, TestSize.Level0)
{
    ApnItem apnItem;
    apnItem.apnTypes_.push_back("default");
    EXPECT_TRUE(apnItem.CanDealWithType("default"));
}

/**
 * @tc.name  : ApnItem_CanDealWithType_002
 * @tc.number: ApnItemTest_002
 * @tc.desc  : Test when the type is DATA_CONTEXT_ROLE_INTERNAL_DEFAULT
 * and apnTypes_ contains DATA_CONTEXT_ROLE_DEFAULT then CanDealWithType returns true
 */
HWTEST_F(ApnManagerTest, ApnItem_CanDealWithType_002, TestSize.Level0)
{
    ApnItem apnItem;
    apnItem.apnTypes_.push_back(DATA_CONTEXT_ROLE_DEFAULT);
    EXPECT_TRUE(apnItem.CanDealWithType(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT));
}

/**
 * @tc.name  : ApnItem_CanDealWithType_003
 * @tc.number: ApnItemTest_003
 * @tc.desc  : Test when the type is not DATA_CONTEXT_ROLE_IA and
 * apnTypes_ contains DATA_CONTEXT_ROLE_ALL then CanDealWithType returns true
 */
HWTEST_F(ApnManagerTest, ApnItem_CanDealWithType_003, TestSize.Level0)
{
    ApnItem apnItem;
    apnItem.apnTypes_.push_back(DATA_CONTEXT_ROLE_ALL);
    EXPECT_TRUE(apnItem.CanDealWithType("not_ia"));
}

/**
 * @tc.name  : ApnItem_CanDealWithType_004
 * @tc.number: ApnItemTest_004
 * @tc.desc  : Test when the type does not match with the apnTypes_ then CanDealWithType returns false
 */
HWTEST_F(ApnManagerTest, ApnItem_CanDealWithType_004, TestSize.Level0)
{
    ApnItem apnItem;
    apnItem.apnTypes_.push_back("default");
    EXPECT_FALSE(apnItem.CanDealWithType("other"));
}

/**
 * @tc.name  : ApnManager_FindApnHolderById_001
 * @tc.number: ApnItemTest_004
 * @tc.desc  : Test when the type does not match with the apnTypes_ then CanDealWithType returns false
 */
HWTEST_F(ApnManagerTest, ApnManager_FindApnHolderById_001, TestSize.Level0)
{
    apnManager->apnIdApnHolderMap_.clear();
    EXPECT_EQ(apnManager->FindApnHolderById(0), nullptr);
    apnManager->AddApnHolder("default", 10);
    EXPECT_EQ(apnManager->FindApnTypeByApnName("abc"), static_cast<uint64_t>(ApnTypes::NONETYPE));
}

/**
 * @tc.name  : ApnItem_IsSimilarPdpProfile_001
 * @tc.number: ApnItemTest_005
 */
HWTEST_F(ApnManagerTest, ApnItem_IsSimilarPdpProfile_001, TestSize.Level0)
{
    PdpProfile p1;
    PdpProfile p2;
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.apn = "1";
    p2.apn = "2";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.apn = "";
    p2.apn = "";
    p1.proxyIpAddress = "1";
    p2.proxyIpAddress = "";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.proxyIpAddress = "";
    p2.proxyIpAddress = "1";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.proxyIpAddress = "1";
    p2.proxyIpAddress = "2";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.proxyIpAddress = "";
    p2.proxyIpAddress = "";
    p1.mvnoType = "1";
    p2.mvnoType = "";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
}

/**
 * @tc.name  : ApnItem_IsSimilarPdpProfile_002
 * @tc.number: ApnItemTest_007
 */
HWTEST_F(ApnManagerTest, ApnItem_IsSimilarPdpProfile_002, TestSize.Level0)
{
    PdpProfile p1;
    PdpProfile p2;
    p1.mvnoMatchData = "1";
    p2.mvnoMatchData = "";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.mvnoMatchData = "";
    p2.mvnoMatchData = "";
    p1.homeUrl = "1";
    p2.homeUrl = "";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.homeUrl = "";
    p2.homeUrl = "1";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.homeUrl = "1";
    p2.homeUrl = "2";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.homeUrl = "";
    p2.homeUrl = "";
    p1.mmsIpAddress = "1";
    p2.mmsIpAddress = "";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.mmsIpAddress = "";
    p2.mmsIpAddress = "1";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    p1.mmsIpAddress = "1";
    p2.mmsIpAddress = "2";
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
}

/**
 * @tc.name  : ApnItem_IsSimilarProtocol_001
 * @tc.number: ApnItemTest_006
 */
HWTEST_F(ApnManagerTest, ApnItem_IsSimilarProtocol_001, TestSize.Level0)
{
    std::string newProtocol;
    std::string oldProtocol;
    EXPECT_TRUE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
    newProtocol = "IPV4V6";
    oldProtocol = "IP";
    EXPECT_TRUE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
    newProtocol = "IPV4V6";
    oldProtocol = "IPV6";
    EXPECT_TRUE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
    newProtocol = "IP";
    oldProtocol = "IPV4V6";
    EXPECT_TRUE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
    newProtocol = "IPV6";
    oldProtocol = "IPV4V6";
    EXPECT_TRUE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
    newProtocol = "IPV6";
    oldProtocol = "IPV4";
    EXPECT_FALSE(ApnItem::IsSimilarProtocol(newProtocol, oldProtocol));
}

/**
 * @tc.name  : ApnManager_TryMergeSimilarPdpProfile_001
 * @tc.number: ApnManagerTest_005
 */
HWTEST_F(ApnManagerTest, ApnManager_TryMergeSimilarPdpProfile_001, TestSize.Level0)
{
    auto preferId = 1;
    apnManager->preferId_ = preferId;
    PdpProfile pdpProfile;
    pdpProfile.profileId = preferId;
    pdpProfile.apnTypes = "";
    std::vector<PdpProfile> apnVec;
    apnVec.push_back(pdpProfile);
    apnManager->TryMergeSimilarPdpProfile(apnVec);
    EXPECT_EQ(apnVec.size(), 1);
    PdpProfile pdpProfile2 = pdpProfile;
    apnVec.push_back(pdpProfile2);
    apnManager->TryMergeSimilarPdpProfile(apnVec);
    EXPECT_EQ(apnVec.size(), 1);
    PdpProfile pdpProfile3 = pdpProfile;
    pdpProfile3.apnTypes = "default";
    apnVec.push_back(pdpProfile3);
    apnManager->TryMergeSimilarPdpProfile(apnVec);
    EXPECT_EQ(apnVec.size(), 1);
}

/**
 * @tc.name  : ApnManager_TryMergeSimilarPdpProfile_002
 * @tc.number: ApnManagerTest_005
 */
HWTEST_F(ApnManagerTest, ApnManager_TryMergeSimilarPdpProfile_002, TestSize.Level1)
{
    uint64_t expected = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET;
    int32_t apnId = DATA_CONTEXT_ROLE_DEFAULT_ID;
    uint64_t actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNAL_DEFAULT;
    apnId = DATA_CONTEXT_ROLE_MMS_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_LE(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_INTERNAL_DEFAULT;
    apnId = DATA_CONTEXT_ROLE_INTERNAL_DEFAULT_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_IA;
    apnId = DATA_CONTEXT_ROLE_IA_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_XCAP;
    apnId = DATA_CONTEXT_ROLE_XCAP_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SUPL;
    apnId = DATA_CONTEXT_ROLE_SUPL_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_DUN;
    apnId = DATA_CONTEXT_ROLE_DUN_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_BIP;
    apnId = DATA_CONTEXT_ROLE_BIP_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_END;
    apnId = -1;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
}

/**
 * @tc.name  : ApnItem_IsSimilarProperty_001
 * @tc.number: ApnItemTest_006
 */
HWTEST_F(ApnManagerTest, ApnItem_IsSimilarProperty_001, TestSize.Level0)
{
    std::string newProtocol;
    std::string oldProtocol;
    EXPECT_TRUE(ApnItem::IsSimilarProperty(newProtocol, oldProtocol));
    newProtocol = "IPV4V6";
    oldProtocol = "IPV4V6";
    EXPECT_TRUE(ApnItem::IsSimilarProperty(newProtocol, oldProtocol));
    newProtocol = "";
    oldProtocol = "IPV6";
    EXPECT_TRUE(ApnItem::IsSimilarProperty(newProtocol, oldProtocol));
    newProtocol = "IP";
    oldProtocol = "";
    EXPECT_TRUE(ApnItem::IsSimilarProperty(newProtocol, oldProtocol));
    newProtocol = "IP";
    oldProtocol = "IPV6";
    EXPECT_FALSE(ApnItem::IsSimilarProperty(newProtocol, oldProtocol));
}
 
/**
 * @tc.number   FetchBipApns_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FetchBipApns_001, TestSize.Level0)
{
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apnManager->allApnItem_.push_back(defaultApnItem);
    std::vector<sptr<ApnItem>> bipApnList;
    apnManager->FetchBipApns(bipApnList);
    EXPECT_GE(bipApnList.size(), 0);
    sptr<ApnItem> bipApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_BIP);
    apnManager->allApnItem_.push_back(bipApnItem);
    bipApnList.clear();
    apnManager->FetchBipApns(bipApnList);
    EXPECT_GE(bipApnList.size(), 0);
}

/**
 * @tc.number   FindBestCapability_006
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_006, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI1;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI1);
}

/**
 * @tc.number   FindBestCapability_007
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_007, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI2;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI2);
}

/**
 * @tc.number   FindBestCapability_008
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_008, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI3;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI3);
}

/**
 * @tc.number   FindBestCapability_009
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_009, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI4;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI4);
}

/**
 * @tc.number   FindBestCapability_010
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_010, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI5;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI5);
}

/**
 * @tc.number   FindBestCapability_011
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindBestCapability_011, Function | MediumTest | Level1)
{
    uint64_t capabilities = 1L << NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI6;
    NetManagerStandard::NetCap netCap = apnManager->FindBestCapability(capabilities);
    ASSERT_EQ(netCap, NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI6);
}

/**
 * @tc.number   AddApnHolder_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, AddApnHolder_001, TestSize.Level0)
{
    apnManager->apnIdApnHolderMap_.clear();
    EXPECT_EQ(apnManager->FindApnHolderById(0), nullptr);
    std::string apnType = "hello";
    int32_t priority = 123;
    EXPECT_EQ(apnManager->FindApnIdByApnName(apnType), DATA_CONTEXT_ROLE_INVALID_ID);
    apnManager->AddApnHolder(apnType, priority);
    EXPECT_EQ(apnManager->FindApnTypeByApnName("abc"), static_cast<uint64_t>(ApnTypes::NONETYPE));
}

/**
 * @tc.number   ReportApnInfo_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ReportApnInfo_001, TestSize.Level0)
{
    int32_t slotId = 123;
    PdpProfile apnData;
    apnManager->ReportApnInfo(slotId, apnData);
    EXPECT_EQ(apnData.apnTypes, "");
    apnData.apnTypes = "hello";
    apnManager->ReportApnInfo(slotId, apnData);
}

/**
 * @tc.number   GetOverallApnState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetOverallApnState_001, TestSize.Level0)
{
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    apnHolder->apnState_ = ApnProfileState::PROFILE_STATE_FAILED;
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(apnHolder);
    auto result = apnManager->GetOverallApnState();
    EXPECT_EQ(result, ApnProfileState::PROFILE_STATE_FAILED);
}

/**
 * @tc.number   GetOverallDefaultApnState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, GetOverallDefaultApnState_001, TestSize.Level0)
{
    sptr<ApnHolder> apnHolder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    apnHolder->apnState_ = ApnProfileState::PROFILE_STATE_FAILED;
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(apnHolder);
    auto result = apnManager->GetOverallDefaultApnState();
    EXPECT_EQ(result, ApnProfileState::PROFILE_STATE_IDLE);
}

/**
 * @tc.number   FindCapabilityByApnId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, FindCapabilityByApnId_001, TestSize.Level1)
{
    uint64_t expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI1;
    int32_t apnId = DATA_CONTEXT_ROLE_SNSSAI1_ID;
    uint64_t actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI2;
    apnId = DATA_CONTEXT_ROLE_SNSSAI2_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI3;
    apnId = DATA_CONTEXT_ROLE_SNSSAI3_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI4;
    apnId = DATA_CONTEXT_ROLE_SNSSAI4_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI5;
    apnId = DATA_CONTEXT_ROLE_SNSSAI5_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
    expected = NetManagerStandard::NetCap::NET_CAPABILITY_SNSSAI6;
    apnId = DATA_CONTEXT_ROLE_SNSSAI6_ID;
    actual = apnManager->FindCapabilityByApnId(apnId);
    EXPECT_EQ(actual, expected);
}

bool CreateDcApnItemExtMock(int32_t slotId, sptr<ApnItem> &apnItem)
{
    return true;
}

/**
 * @tc.number   ApnItem_CreateAllApnItemByDatabaseTest_001
 * @tc.name     test create dc apn item scene
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnItem_CreateAllApnItemByDatabaseTest_001, TestSize.Level0)
{
    TELEPHONY_EXT_WRAPPER.createAllApnItemExt_ = nullptr;
    TELEPHONY_EXT_WRAPPER.createAllApnItemExt_ = CreateDcApnItemExtMock;
    std::string errMsg = "";
    int32_t result = apnManager->CreateAllApnItemByDatabase(0, errMsg);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.number   ApnItem_CreateAllApnItemByDatabaseTest_002
 * @tc.name     test create normal apn item
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnItem_CreateAllApnItemByDatabaseTest_002, TestSize.Level0)
{
    std::string errMsg = "";
    int32_t result = apnManager->CreateAllApnItemByDatabase(0, errMsg);
    EXPECT_EQ(result, 1);
}
 
HWTEST_F(ApnManagerTest, ApnItem_PushApnItem_001, TestSize.Level0)
{
    auto apnManager = std::make_shared<ApnManager>();
    int32_t slotId = 0;
    sptr<ApnItem> defaultApnItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    int32_t ret = apnManager->PushApnItem(0, slotId, defaultApnItem);
    EXPECT_EQ(ret, 1);
    sptr<ApnItem> defaultApnItem2 = nullptr;
    ret = apnManager->PushApnItem(0, slotId, defaultApnItem2);
    EXPECT_EQ(ret, 0);
}
 
HWTEST_F(ApnManagerTest, ApnItem_GetPreferId_001, TestSize.Level0)
{
    auto apnManager = std::make_shared<ApnManager>();
    int32_t slotId = -1;
    std::string errMsg = "";
    bool ret = apnManager->GetPreferId(slotId, errMsg);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   IsPreferIdChangedByMerge_001
 * @tc.name     test preferIdChangedByMerge_ is false by default and true after merge matches preferId
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsPreferIdChangedByMerge_001, Function | MediumTest | Level1)
{
    EXPECT_FALSE(apnManager->IsPreferIdChangedByMerge());
    apnManager->preferId_ = 100;
    PdpProfile newProfile;
    newProfile.profileId = 200;
    newProfile.apnTypes = "default";
    PdpProfile oldProfile;
    oldProfile.profileId = 100;
    oldProfile.apnTypes = "mms";
    apnManager->MergePdpProfile(newProfile, oldProfile);
    EXPECT_TRUE(apnManager->IsPreferIdChangedByMerge());
    EXPECT_EQ(apnManager->preferId_, 200);
}

/**
 * @tc.number   IsPreferIdChangedByMerge_002
 * @tc.name     test preferIdChangedByMerge_ remains false when preferId does not match oldProfile
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, IsPreferIdChangedByMerge_002, Function | MediumTest | Level1)
{
    apnManager->preferId_ = 100;
    PdpProfile newProfile;
    newProfile.profileId = 200;
    newProfile.apnTypes = "default";
    PdpProfile oldProfile;
    oldProfile.profileId = 300;
    oldProfile.apnTypes = "mms";
    apnManager->MergePdpProfile(newProfile, oldProfile);
    EXPECT_FALSE(apnManager->IsPreferIdChangedByMerge());
    EXPECT_EQ(apnManager->preferId_, 100);
}

/**
 * @tc.number   CreateAllApnItemByDatabase_001
 * @tc.name     test CreateAllApnItemByDatabase with empty operator numeric
 * @tc.desc     Function test: sim operator numeric is empty, apn creation stops with error message
 */
HWTEST_F(ApnManagerTest, CreateAllApnItemByDatabase_001, Function | MediumTest | Level1)
{
    auto mockSim = std::make_shared<MockSimManager>();
    EXPECT_CALL(*mockSim, GetSimOperatorNumeric(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(u""), Return(0)));
    EXPECT_CALL(*mockSim, IsCTSimCard(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(0)));
    // 真机上 vendor ext 钩子已加载会使 CreateAllApnItemByDatabase 提前返回、走不到 sim
    // 查询；经守卫临时屏蔽并在用例结束还原原钩子，保证本用例只依赖自建 mock
    ScopedSetter<TelephonyExtWrapper::CREATE_ALL_APN_ITEM_EXT> createAllApnGuard(TELEPHONY_EXT_WRAPPER.createAllApnItemExt_, nullptr);
    ScopedSetter<TelephonyExtWrapper::CREATE_DC_APN_ITEM_EXT> createDcApnGuard(TELEPHONY_EXT_WRAPPER.createDcApnItemExt_, nullptr);
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    std::string errMsg;
    EXPECT_EQ(apnManager->CreateAllApnItemByDatabase(0, errMsg), 0);
    EXPECT_EQ(errMsg, "numeric is empty");
}

/**
 * @tc.number   CreateAllApnItemByDatabase_002
 * @tc.name     test CreateAllApnItemByDatabase with invalid sim id
 * @tc.desc     Function test: GetPreferId fails due to invalid sim id, apn creation stops early
 */
HWTEST_F(ApnManagerTest, CreateAllApnItemByDatabase_002, Function | MediumTest | Level1)
{
    auto mockSim = std::make_shared<MockSimManager>();
    EXPECT_CALL(*mockSim, GetSimOperatorNumeric(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(u"46000"), Return(0)));
    EXPECT_CALL(*mockSim, IsCTSimCard(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(0)));
    EXPECT_CALL(*mockSim, GetSimId(_)).WillOnce(Return(0));
    // 同 _001：临时屏蔽 vendor ext 钩子，用例结束经守卫还原
    ScopedSetter<TelephonyExtWrapper::CREATE_ALL_APN_ITEM_EXT> createAllApnGuard(TELEPHONY_EXT_WRAPPER.createAllApnItemExt_, nullptr);
    ScopedSetter<TelephonyExtWrapper::CREATE_DC_APN_ITEM_EXT> createDcApnGuard(TELEPHONY_EXT_WRAPPER.createDcApnItemExt_, nullptr);
    ScopedSetter<std::shared_ptr<ISimManager>> simGuard(CoreManagerInner::GetInstance().simManager_, mockSim);
    std::string errMsg;
    EXPECT_EQ(apnManager->CreateAllApnItemByDatabase(0, errMsg), 0);
    EXPECT_EQ(errMsg, "GetPreferId create failed due to invalid sim id");
}

/*
 * ==== P2 batch-2 续：apn_manager 纯逻辑函数直驱（b2full 函数级缺口）====
 * 目标：GetCTOperator 44、FetchDunApns 40、GetOverallApnState 28、GetOverallDefaultApnState 18。
 * CoreManagerInner 依赖一律走 RAII 安装（构造替换、析构还原原 mock，避免跨用例污染）。
 */
namespace {
class ScopedCoreMock {
public:
    ScopedCoreMock()
    {
        auto &core = CoreManagerInner::GetInstance();
        savedSim_ = core.simManager_;
        savedNss_ = core.networkSearchManager_;
        simMock_ = std::make_shared<MockSimManager>();
        nssMock_ = std::make_shared<MockNetworkSearchManager>();
        core.simManager_ = simMock_;
        core.networkSearchManager_ = nssMock_;
    }
    ~ScopedCoreMock()
    {
        auto &core = CoreManagerInner::GetInstance();
        core.simManager_ = savedSim_;
        core.networkSearchManager_ = savedNss_;
    }
    std::shared_ptr<MockSimManager> simMock_;
    std::shared_ptr<MockNetworkSearchManager> nssMock_;

private:
    std::shared_ptr<ISimManager> savedSim_;
    std::shared_ptr<INetworkSearch> savedNss_;
};
} // namespace

/**
 * @tc.number   ApnManager_GetCTOperator_001
 * @tc.name     test get ct operator with isCT false / gc iccid / 46011 unchanged
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_GetCTOperator_001, TestSize.Level0)
{
    ScopedCoreMock core;
    // 1) isCT=false：numeric 不变（第一/第二分支均不进）
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = false; return 0; });
    std::string numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46000");
    // 2) isCT=true 且 numeric != 46011 → 改 46003；天翼通 GC ICCID 前缀命中 → 45431
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"8985231999"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"China Telecom"; return 0; });
    numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "45431");
    // 3) isCT=true 且 numeric == 46011 → 第一个分支不进；非 46003 → 深层分支不进
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    numeric = "46011";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46011");
}

/**
 * @tc.number   ApnManager_GetCTOperator_002
 * @tc.name     test get ct operator with gc spn / mo iccid plmn variants / no match
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_GetCTOperator_002, TestSize.Level0)
{
    ScopedCoreMock core;
    // 1) ICCID 不命中但 SPN=CTExcel → GC 分支
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"1234567890"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"CTExcel"; return 0; });
    std::string numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "45431");
    // 2) MO ICCID_1 命中 + plmn=46001 → 转 46001
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"8985302123"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"China Telecom"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimOperatorNumeric(0, _))
        .WillOnce([](int32_t, std::u16string &plmn) { plmn = u"46001"; return 0; });
    numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46001");
    // 3) MO ICCID_2 命中 + plmn 空 → 保持 46003
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"8985307123"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"China Telecom"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimOperatorNumeric(0, _))
        .WillOnce([](int32_t, std::u16string &plmn) { plmn = u""; return 0; });
    numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46003");
    // 4) MO ICCID 命中 + plmn 非 46001 → 保持 46003
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"8985302123"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"China Telecom"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimOperatorNumeric(0, _))
        .WillOnce([](int32_t, std::u16string &plmn) { plmn = u"46002"; return 0; });
    numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46003");
    // 5) ICCID/SPN 均不命中 → 保持 46003
    EXPECT_CALL(*core.simMock_, IsCTSimCard(0, _))
        .WillOnce([](int32_t, bool &isCT) { isCT = true; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimIccId(0, _))
        .WillOnce([](int32_t, std::u16string &iccId) { iccId = u"9999999999"; return 0; });
    EXPECT_CALL(*core.simMock_, GetSimSpn(0, _))
        .WillOnce([](int32_t, std::u16string &spn) { spn = u"China Mobile"; return 0; });
    numeric = "46000";
    apnManager->GetCTOperator(0, numeric);
    EXPECT_EQ(numeric, "46003");
}

/**
 * @tc.number   ApnManager_FetchDunApns_001
 * @tc.name     test fetch dun apns in roaming with user edited prefer
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_FetchDunApns_001, TestSize.Level0)
{
    ScopedCoreMock core;
    apnManager->preferId_ = 5;
    sptr<ApnItem> dunApn = ApnItem::MakeDefaultApn("default,dun");
    dunApn->attr_.profileId_ = 5;
    dunApn->attr_.isEdited_ = true;
    // 1) 漫游 + 无偏好（未编辑）→ 早退，列表不变
    EXPECT_CALL(*core.nssMock_, GetPsRoamingState(0)).WillOnce(Return(1));
    apnManager->allApnItem_.clear();
    std::vector<sptr<ApnItem>> matchList;
    apnManager->FetchDunApns(matchList, 0);
    EXPECT_TRUE(matchList.empty());
    // 2) 漫游 + 偏好已编辑且可处理 DUN → 偏好插入队首
    apnManager->allApnItem_.push_back(dunApn);
    EXPECT_CALL(*core.nssMock_, GetPsRoamingState(0)).WillOnce(Return(1));
    apnManager->FetchDunApns(matchList, 0);
    ASSERT_EQ(matchList.size(), 1);
    EXPECT_EQ(matchList[0], dunApn);
}

/**
 * @tc.number   ApnManager_FetchDunApns_002
 * @tc.name     test fetch dun apns not in roaming with scan-all fallback
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_FetchDunApns_002, TestSize.Level0)
{
    ScopedCoreMock core;
    // 1) 非漫游 + 偏好存在但不可处理 DUN → 不插入 → 全表扫描兜底收集 DUN 项
    apnManager->preferId_ = 5;
    sptr<ApnItem> preferApn = ApnItem::MakeDefaultApn("default");
    preferApn->attr_.profileId_ = 5;
    sptr<ApnItem> dunApn = ApnItem::MakeDefaultApn("default,dun");
    apnManager->allApnItem_.clear();
    apnManager->allApnItem_.push_back(preferApn);
    apnManager->allApnItem_.push_back(dunApn);
    EXPECT_CALL(*core.nssMock_, GetPsRoamingState(0)).WillOnce(Return(0));
    std::vector<sptr<ApnItem>> matchList;
    apnManager->FetchDunApns(matchList, 0);
    ASSERT_EQ(matchList.size(), 1);
    EXPECT_EQ(matchList[0], dunApn);
    // 2) 非漫游 + 无偏好命中 + 空表 → 列表保持空
    apnManager->preferId_ = 99;
    apnManager->allApnItem_.clear();
    EXPECT_CALL(*core.nssMock_, GetPsRoamingState(0)).WillOnce(Return(0));
    matchList.clear();
    apnManager->FetchDunApns(matchList, 0);
    EXPECT_TRUE(matchList.empty());
}

/**
 * @tc.number   ApnManager_GetOverallApnState_002
 * @tc.name     test overall apn state with connected/connecting/retrying/idle holders
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_GetOverallApnState_002, TestSize.Level0)
{
    auto makeHolder = [](ApnProfileState state) {
        sptr<ApnHolder> holder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
            static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
        holder->apnState_ = state;
        return holder;
    };
    // 1) 空 holders → IDLE
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_IDLE);
    // 2) CONNECTED → HasAnyConnectedState 命中 → CONNECTED
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_CONNECTED));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    apnManager->apnHolders_.clear();
    // 3) DISCONNECTING → HasAnyConnectedState 命中 → CONNECTED
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_DISCONNECTING));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    apnManager->apnHolders_.clear();
    // 4) CONNECTING → CONNECTING
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_CONNECTING));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTING);
    apnManager->apnHolders_.clear();
    // 5) RETRYING → CONNECTING
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_RETRYING));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTING);
    apnManager->apnHolders_.clear();
    // 6) IDLE → failState → IDLE（含 nullptr holder 跳过）
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_IDLE));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_IDLE);
}

/**
 * @tc.number   ApnManager_GetOverallDefaultApnState_002
 * @tc.name     test overall default apn state with default/internal holders
 * @tc.desc     Function test
 */
HWTEST_F(ApnManagerTest, ApnManager_GetOverallDefaultApnState_002, TestSize.Level0)
{
    auto makeHolder = [](const std::string &type, ApnProfileState state) {
        sptr<ApnHolder> holder = std::make_unique<ApnHolder>(type,
            static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
        holder->apnState_ = state;
        return holder;
    };
    // 1) 空 holders → IDLE
    EXPECT_EQ(apnManager->GetOverallDefaultApnState(), ApnProfileState::PROFILE_STATE_IDLE);
    // 2) default=CONNECTED → 按优先级数组命中 CONNECTED
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_DEFAULT,
        ApnProfileState::PROFILE_STATE_CONNECTED));
    EXPECT_EQ(apnManager->GetOverallDefaultApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    apnManager->apnHolders_.clear();
    // 3) default=FAILED + internal=RETRYING → IDLE 检查双双不中 → RETRYING
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_DEFAULT,
        ApnProfileState::PROFILE_STATE_FAILED));
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT,
        ApnProfileState::PROFILE_STATE_RETRYING));
    EXPECT_EQ(apnManager->GetOverallDefaultApnState(), ApnProfileState::PROFILE_STATE_RETRYING);
}
namespace {
/*
 * b8 批基建：CellularDataRdbHelper 单例注入缝隙。
 * 经 #define private public 直接改写 DelayedSingleton<CellularDataRdbHelper>::instance_
 * （ScopedSetter RAII 守卫还原），配合 CreateDataAbilityHelper 虚函数缝隙
 * （对齐 cellular_data_rdb_helper_test 的 TestRdbHelper 模式），
 * 使 ApnManager::GetPreferId / CreateMvnoApnItems 走到受控的 DataShare 分支，
 * 不触达真机 APN 库。
 */
class ApnMgrRdbStub : public CellularDataRdbHelper {
public:
    ApnMgrRdbStub() = default;
    ~ApnMgrRdbStub() = default;
    std::shared_ptr<DataShare::DataShareHelper> helperToReturn_ = nullptr;

    std::shared_ptr<DataShare::DataShareHelper> CreateDataAbilityHelper(const int waitTime) override
    {
        return helperToReturn_;
    }
};

// 简化结果集 wiring（列名→值映射，同 rdb_helper_test 模式；仅本批用例使用，进入用例时显式清零）
int32_t g_apnRowCnt = 0;
std::map<std::string, std::string> g_apnColStr;
std::map<std::string, int32_t> g_apnColInt;
std::map<std::string, int32_t> g_apnColIdx;
std::map<int32_t, std::string> g_apnIdxName;

void ResetApnResultSet()
{
    g_apnRowCnt = 0;
    g_apnColStr.clear();
    g_apnColInt.clear();
    g_apnColIdx.clear();
    g_apnIdxName.clear();
}

void SetupApnResultSet(const std::shared_ptr<DataShareResultSetMock> &rs)
{
    ON_CALL(*rs, GetRowCount(_))
        .WillByDefault(Invoke([](int &count) {
            count = g_apnRowCnt;
            return 0;
        }));
    ON_CALL(*rs, GoToRow(_)).WillByDefault(Return(0));
    ON_CALL(*rs, Close()).WillByDefault(Return(0));
    ON_CALL(*rs, GetColumnIndex(_, _))
        .WillByDefault(Invoke([](const std::string &columnName, int &columnIndex) {
            auto it = g_apnColIdx.find(columnName);
            if (it == g_apnColIdx.end()) {
                int id = static_cast<int>(g_apnColIdx.size());
                g_apnColIdx[columnName] = id;
                g_apnIdxName[id] = columnName;
                it = g_apnColIdx.find(columnName);
            }
            columnIndex = it->second;
            return 0;
        }));
    ON_CALL(*rs, GetString(_, _))
        .WillByDefault(Invoke([](int columnIndex, std::string &value) {
            auto nameIt = g_apnIdxName.find(columnIndex);
            std::string name = (nameIt != g_apnIdxName.end()) ? nameIt->second : "";
            auto valIt = g_apnColStr.find(name);
            value = (valIt != g_apnColStr.end()) ? valIt->second : "";
            return 0;
        }));
    ON_CALL(*rs, GetInt(_, _))
        .WillByDefault(Invoke([](int columnIndex, int &value) {
            auto nameIt = g_apnIdxName.find(columnIndex);
            std::string name = (nameIt != g_apnIdxName.end()) ? nameIt->second : "";
            auto valIt = g_apnColInt.find(name);
            value = (valIt != g_apnColInt.end()) ? valIt->second : 0;
            return 0;
        }));
}
} // namespace

/**
 * @tc.number   ApnManager_GetPreferId_002
 * @tc.name     test GetPreferId with valid sim id but rdb helper creation fail
 * @tc.desc     simId 有效越过早退；DataShare helper 为 null → QueryPreferApn 失败 → 返回 false
 */
HWTEST_F(ApnManagerTest, ApnManager_GetPreferId_002, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimId(_)).WillRepeatedly(Return(1));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    std::string errMsg = "";
    EXPECT_FALSE(apnManager->GetPreferId(0, errMsg));
}

/**
 * @tc.number   ApnManager_GetPreferId_003
 * @tc.name     test GetPreferId with query success but empty prefer apn
 * @tc.desc     QueryPreferApn 成功且结果为空 → 返回 true，preferId_ 不变
 */
HWTEST_F(ApnManagerTest, ApnManager_GetPreferId_003, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimId(_)).WillRepeatedly(Return(1));
    ResetApnResultSet();
    g_apnRowCnt = 0;
    auto rs = std::make_shared<DataShareResultSetMock>();
    SetupApnResultSet(rs);
    auto helper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*helper, Release()).WillByDefault(Return(true));
    ON_CALL(*helper, Query(_, _, _, _)).WillByDefault(Return(rs));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    stub->helperToReturn_ = helper;
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    std::string errMsg = "";
    EXPECT_TRUE(apnManager->GetPreferId(0, errMsg));
    EXPECT_EQ(apnManager->preferId_, static_cast<int32_t>(INVALID_PROFILE_ID));
}

/**
 * @tc.number   ApnManager_GetPreferId_004
 * @tc.name     test GetPreferId with one prefer apn row
 * @tc.desc     QueryPreferApn 成功且返回 1 行 → preferId_ 取该行 PROFILE_ID
 */
HWTEST_F(ApnManagerTest, ApnManager_GetPreferId_004, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimId(_)).WillRepeatedly(Return(1));
    ResetApnResultSet();
    g_apnRowCnt = 1;
    g_apnColInt[PdpProfileData::PROFILE_ID] = 7;
    auto rs = std::make_shared<DataShareResultSetMock>();
    SetupApnResultSet(rs);
    auto helper = std::make_shared<MockDataShareHelper>();
    ON_CALL(*helper, Release()).WillByDefault(Return(true));
    ON_CALL(*helper, Query(_, _, _, _)).WillByDefault(Return(rs));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    stub->helperToReturn_ = helper;
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    std::string errMsg = "";
    EXPECT_TRUE(apnManager->GetPreferId(0, errMsg));
    EXPECT_EQ(apnManager->preferId_, 7);
}

/**
 * @tc.number   CreateMvnoApnItems_002
 * @tc.name     test CreateMvnoApnItems with non-empty spn and null rdb helper
 * @tc.desc     SPN 非空 → SPN 查询走 helper → helper 为 null → 查询失败早退
 */
HWTEST_F(ApnManagerTest, CreateMvnoApnItems_002, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimSpn(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::u16string(u"CMCC")), Return(0)));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    EXPECT_EQ(apnManager->CreateMvnoApnItems(0, "460", "00"), 0);
}

/**
 * @tc.number   CreateMvnoApnItems_003
 * @tc.name     test CreateMvnoApnItems with non-empty imsi
 * @tc.desc     SPN 为空（查询早退成功）→ IMSI 非空 → IMSI 查询失败早退
 */
HWTEST_F(ApnManagerTest, CreateMvnoApnItems_003, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetIMSI(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::u16string(u"460001234567890")), Return(0)));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    EXPECT_EQ(apnManager->CreateMvnoApnItems(0, "460", "00"), 0);
}

/**
 * @tc.number   CreateMvnoApnItems_004
 * @tc.name     test CreateMvnoApnItems with non-empty gid1
 * @tc.desc     SPN/IMSI 为空（查询早退成功）→ GID1 非空 → GID1 查询失败早退
 */
HWTEST_F(ApnManagerTest, CreateMvnoApnItems_004, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimGid1(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::u16string(u"GID1DATA")), Return(0)));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    EXPECT_EQ(apnManager->CreateMvnoApnItems(0, "460", "00"), 0);
}

/**
 * @tc.number   CreateMvnoApnItems_005
 * @tc.name     test CreateMvnoApnItems with non-empty iccId
 * @tc.desc     SPN/IMSI/GID1 为空（查询早退成功）→ ICCID 非空 → ICCID 查询失败早退
 */
HWTEST_F(ApnManagerTest, CreateMvnoApnItems_005, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimIccId(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(std::u16string(u"89860012345678901234")), Return(0)));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    EXPECT_EQ(apnManager->CreateMvnoApnItems(0, "460", "00"), 0);
}

/**
 * @tc.number   GetRilAttachApn_005
 * @tc.name     test GetRilAttachApn with default first then ia
 * @tc.desc     先记录 DEFAULT 再被 IA 覆盖 break → 返回 IA 项
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_005, TestSize.Level0)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> defaultApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    sptr<ApnItem> iaApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_IA);
    allApnItem.push_back(defaultApn);
    allApnItem.push_back(iaApn);
    apnManager->allApnItem_ = allApnItem;
    EXPECT_EQ(apnManager->GetRilAttachApn(), iaApn);
}

/**
 * @tc.number   GetRilAttachApn_006
 * @tc.name     test GetRilAttachApn with two default items
 * @tc.desc     第二项 DEFAULT 查询因 attachApn 非空短路 → 返回首个 DEFAULT 项
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_006, TestSize.Level0)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> first = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    sptr<ApnItem> second = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    allApnItem.push_back(first);
    allApnItem.push_back(second);
    apnManager->allApnItem_ = allApnItem;
    EXPECT_EQ(apnManager->GetRilAttachApn(), first);
}

/**
 * @tc.number   GetRilAttachApn_007
 * @tc.name     test GetRilAttachApn with wildcard apn
 * @tc.desc     "*" 通配项：IA 查询不匹配（IA 排除通配）→ DEFAULT 查询经通配弧匹配
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_007, TestSize.Level0)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> wildcard = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_ALL);
    sptr<ApnItem> mmsApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS);
    allApnItem.push_back(wildcard);
    allApnItem.push_back(mmsApn);
    apnManager->allApnItem_ = allApnItem;
    EXPECT_EQ(apnManager->GetRilAttachApn(), wildcard);
}

/**
 * @tc.number   GetRilAttachApn_008
 * @tc.name     test GetRilAttachApn with no ia/default-capable item
 * @tc.desc     全表无匹配 → 兜底返回 allApnItem_[0]
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_008, TestSize.Level0)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> mmsApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_MMS);
    sptr<ApnItem> dunApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DUN);
    allApnItem.push_back(mmsApn);
    allApnItem.push_back(dunApn);
    apnManager->allApnItem_ = allApnItem;
    EXPECT_EQ(apnManager->GetRilAttachApn(), mmsApn);
}

/**
 * @tc.number   GetRilAttachApn_009
 * @tc.name     test GetRilAttachApn with multi-type single apn
 * @tc.desc     "ia,default" 多类型单项 → 首次迭代 IA 命中即 break
 */
HWTEST_F(ApnManagerTest, GetRilAttachApn_009, TestSize.Level0)
{
    std::vector<sptr<ApnItem>> allApnItem;
    sptr<ApnItem> multiType = ApnItem::MakeDefaultApn("ia,default");
    allApnItem.push_back(multiType);
    apnManager->allApnItem_ = allApnItem;
    EXPECT_EQ(apnManager->GetRilAttachApn(), multiType);
}

/**
 * @tc.number   ApnManager_GetOverallApnState_003
 * @tc.name     test overall apn state with null/connecting/idle holder matrix
 * @tc.desc     null 项 continue；CONNECTING → 聚合 CONNECTING；仅 IDLE → 聚合 IDLE
 */
HWTEST_F(ApnManagerTest, ApnManager_GetOverallApnState_003, TestSize.Level0)
{
    auto makeHolder = [](ApnProfileState state) {
        sptr<ApnHolder> holder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
            static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
        holder->apnState_ = state;
        return holder;
    };
    // 1) null + CONNECTED → HasAnyConnectedState 命中（null continue 弧）
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_CONNECTED));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    apnManager->apnHolders_.clear();
    // 2) CONNECTING → 聚合 CONNECTING
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_CONNECTING));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_CONNECTING);
    apnManager->apnHolders_.clear();
    // 3) 仅 IDLE → failState → 聚合 IDLE
    apnManager->apnHolders_.push_back(makeHolder(ApnProfileState::PROFILE_STATE_IDLE));
    EXPECT_EQ(apnManager->GetOverallApnState(), ApnProfileState::PROFILE_STATE_IDLE);
}

/**
 * @tc.number   ApnManager_GetOverallDefaultApnState_003
 * @tc.name     test overall default apn state with null item and both role holders
 * @tc.desc     null 项 continue；DEFAULT/INTERNAL_DEFAULT 双 holder 状态聚合；双 IDLE → 最终 IDLE
 */
HWTEST_F(ApnManagerTest, ApnManager_GetOverallDefaultApnState_003, TestSize.Level0)
{
    auto makeHolder = [](const std::string &type, ApnProfileState state) {
        sptr<ApnHolder> holder = std::make_unique<ApnHolder>(type,
            static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
        holder->apnState_ = state;
        return holder;
    };
    // 1) null + 双角色 CONNECTED/CONNECTING → 按优先级数组命中 CONNECTED
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_DEFAULT,
        ApnProfileState::PROFILE_STATE_CONNECTED));
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT,
        ApnProfileState::PROFILE_STATE_CONNECTING));
    EXPECT_EQ(apnManager->GetOverallDefaultApnState(), ApnProfileState::PROFILE_STATE_CONNECTED);
    apnManager->apnHolders_.clear();
    // 2) 双角色均 IDLE → 优先级数组全不中 → 最终 IDLE
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_DEFAULT,
        ApnProfileState::PROFILE_STATE_IDLE));
    apnManager->apnHolders_.push_back(makeHolder(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT,
        ApnProfileState::PROFILE_STATE_IDLE));
    EXPECT_EQ(apnManager->GetOverallDefaultApnState(), ApnProfileState::PROFILE_STATE_IDLE);
}

/**
 * @tc.number   HasAnyConnectedState_005
 * @tc.name     test HasAnyConnectedState with null then connected holder
 * @tc.desc     null 项 continue 后命中 CONNECTED → true
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_005, TestSize.Level0)
{
    sptr<ApnHolder> holder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    holder->apnState_ = ApnProfileState::PROFILE_STATE_CONNECTED;
    apnManager->apnHolders_.push_back(nullptr);
    apnManager->apnHolders_.push_back(holder);
    EXPECT_TRUE(apnManager->HasAnyConnectedState());
}

/**
 * @tc.number   HasAnyConnectedState_006
 * @tc.name     test HasAnyConnectedState with connecting holder only
 * @tc.desc     CONNECTING 不属于连接态 → false
 */
HWTEST_F(ApnManagerTest, HasAnyConnectedState_006, TestSize.Level0)
{
    sptr<ApnHolder> holder = std::make_unique<ApnHolder>(DATA_CONTEXT_ROLE_DEFAULT,
        static_cast<int32_t>(DataContextPriority::PRIORITY_LOW)).release();
    holder->apnState_ = ApnProfileState::PROFILE_STATE_CONNECTING;
    apnManager->apnHolders_.push_back(holder);
    EXPECT_FALSE(apnManager->HasAnyConnectedState());
}

/**
 * @tc.number   ApnManager_FilterMatchedApns_001
 * @tc.name     test FilterMatchedApns type matching matrix
 * @tc.desc     INTERNAL_DEFAULT→default 隐式匹配弧、default→"*" 通配弧、ia→"*" 不匹配弧、多类型迭代弧
 */
HWTEST_F(ApnManagerTest, ApnManager_FilterMatchedApns_001, TestSize.Level0)
{
    apnManager->allApnItem_.clear();
    sptr<ApnItem> defaultApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    apnManager->allApnItem_.push_back(defaultApn);
    // 1) INTERNAL_DEFAULT 请求 → default 项经隐式弧匹配
    EXPECT_EQ(apnManager->FilterMatchedApns(DATA_CONTEXT_ROLE_INTERNAL_DEFAULT, 0).size(), 1);
    apnManager->allApnItem_.clear();
    // 2) default 请求 → "*" 通配项匹配
    sptr<ApnItem> wildcard = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_ALL);
    apnManager->allApnItem_.push_back(wildcard);
    EXPECT_EQ(apnManager->FilterMatchedApns(DATA_CONTEXT_ROLE_DEFAULT, 0).size(), 1);
    // 3) ia 请求 → "*" 通配项不匹配（IA 排除通配）
    EXPECT_EQ(apnManager->FilterMatchedApns(DATA_CONTEXT_ROLE_IA, 0).size(), 0);
    apnManager->allApnItem_.clear();
    // 4) mms 请求 → 多类型项第二段命中
    sptr<ApnItem> multiType = ApnItem::MakeDefaultApn("default,mms");
    apnManager->allApnItem_.push_back(multiType);
    EXPECT_EQ(apnManager->FilterMatchedApns(DATA_CONTEXT_ROLE_MMS, 0).size(), 1);
}

/**
 * @tc.number   FetchBipApns_002
 * @tc.name     test FetchBipApns with wildcard apn
 * @tc.desc     "*" 通配项经 BIP 查询通配弧命中收集；无 BIP 能力项不收集
 */
HWTEST_F(ApnManagerTest, FetchBipApns_002, TestSize.Level0)
{
    apnManager->allApnItem_.clear();
    sptr<ApnItem> wildcard = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_ALL);
    apnManager->allApnItem_.push_back(wildcard);
    std::vector<sptr<ApnItem>> bipList;
    apnManager->FetchBipApns(bipList);
    ASSERT_EQ(bipList.size(), 1);
    EXPECT_EQ(bipList[0], wildcard);
    // ia 项（通配排除 IA）不命中
    apnManager->allApnItem_.clear();
    apnManager->allApnItem_.push_back(ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_IA));
    bipList.clear();
    apnManager->FetchBipApns(bipList);
    EXPECT_TRUE(bipList.empty());
}

/**
 * @tc.number   ApnManager_FetchDunApns_003
 * @tc.name     test FetchDunApns with wildcard prefer and wildcard scan item
 * @tc.desc     非漫游 + "*" 通配偏好（DUN 通配弧命中）插入队首；无偏好时全表 "*" 项经通配弧收集
 */
HWTEST_F(ApnManagerTest, ApnManager_FetchDunApns_003, TestSize.Level0)
{
    ScopedCoreMock core;
    EXPECT_CALL(*core.simMock_, GetSimId(_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*core.nssMock_, GetPsRoamingState(_)).WillRepeatedly(Return(0));
    auto stub = std::make_shared<ApnMgrRdbStub>();
    ScopedSetter<std::shared_ptr<CellularDataRdbHelper>> rdbGuard(
        DelayedSingleton<CellularDataRdbHelper>::instance_, stub);
    // 1) 偏好 "*" 通配项 → CanDealWithType(DUN) 通配弧命中 → 插入队首
    apnManager->preferId_ = 5;
    sptr<ApnItem> wildcardPrefer = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_ALL);
    wildcardPrefer->attr_.profileId_ = 5;
    apnManager->allApnItem_.clear();
    apnManager->allApnItem_.push_back(wildcardPrefer);
    std::vector<sptr<ApnItem>> matchList;
    apnManager->FetchDunApns(matchList, 0);
    ASSERT_EQ(matchList.size(), 1);
    EXPECT_EQ(matchList[0], wildcardPrefer);
    // 2) 偏好未命中 + 空列表 → 全表 "*" 项经通配弧收集
    apnManager->preferId_ = 99;
    apnManager->allApnItem_.clear();
    sptr<ApnItem> wildcardItem = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_ALL);
    apnManager->allApnItem_.push_back(wildcardItem);
    matchList.clear();
    apnManager->FetchDunApns(matchList, 0);
    ASSERT_EQ(matchList.size(), 1);
    EXPECT_EQ(matchList[0], wildcardItem);
}

/**
 * @tc.number   GetNextRetryApnItem_008
 * @tc.name     test GetNextRetryApnItem skipping bad apn
 * @tc.desc     首项 bad → 复位计数并前进索引 → 递归返回次项 good
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_008, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> badApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    badApn->MarkBadApn(true);
    sptr<ApnItem> goodApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    policy->matchedApns_.push_back(badApn);
    policy->matchedApns_.push_back(goodApn);
    policy->currentApnIndex_ = 0;
    EXPECT_EQ(policy->GetNextRetryApnItem(), goodApn);
}

/**
 * @tc.number   GetNextRetryApnItem_009
 * @tc.name     test GetNextRetryApnItem with try count exceeding max
 * @tc.desc     tryCount 超过 maxCount → 复位并前进索引 → 越界复位后返回同项
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_009, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> goodApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    policy->matchedApns_.push_back(goodApn);
    policy->tryCount_ = 5;
    policy->maxCount_ = 5;
    policy->currentApnIndex_ = 0;
    EXPECT_EQ(policy->GetNextRetryApnItem(), goodApn);
    EXPECT_EQ(policy->currentApnIndex_, 0);
    EXPECT_EQ(policy->tryCount_, 1);
}

/**
 * @tc.number   GetNextRetryApnItem_010
 * @tc.name     test GetNextRetryApnItem with null apn item
 * @tc.desc     索引指向 null 项 → 首条件短路（IsAllBadApn 无判空，故 null 项不能是首元素）→ 返回 null
 */
HWTEST_F(ApnManagerTest, GetNextRetryApnItem_010, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> goodApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    policy->matchedApns_.push_back(goodApn);
    policy->matchedApns_.push_back(nullptr);
    policy->currentApnIndex_ = 1;
    EXPECT_EQ(policy->GetNextRetryApnItem(), nullptr);
}

/**
 * @tc.number   SetMatchedApns_002
 * @tc.name     test SetMatchedApns with same size but mismatched profile id
 * @tc.desc     同长度且 profileId 不一致 → 复位 currentApnIndex_
 */
HWTEST_F(ApnManagerTest, SetMatchedApns_002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> oldApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    oldApn->attr_.profileId_ = 1;
    policy->matchedApns_.push_back(oldApn);
    policy->currentApnIndex_ = 3;
    std::vector<sptr<ApnItem>> newApns;
    sptr<ApnItem> newApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    newApn->attr_.profileId_ = 2;
    newApns.push_back(newApn);
    policy->SetMatchedApns(newApns);
    EXPECT_EQ(policy->currentApnIndex_, 0);
    EXPECT_EQ(policy->GetMatchedApns().size(), 1);
}

/**
 * @tc.number   SetMatchedApns_003
 * @tc.name     test SetMatchedApns with null elements in same-size lists
 * @tc.desc     同长度但存在 null 元素 → 跳过比较直接替换
 */
HWTEST_F(ApnManagerTest, SetMatchedApns_003, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    policy->matchedApns_.push_back(nullptr);
    policy->currentApnIndex_ = 3;
    std::vector<sptr<ApnItem>> newApns;
    sptr<ApnItem> newApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    newApns.push_back(newApn);
    policy->SetMatchedApns(newApns);
    EXPECT_EQ(policy->currentApnIndex_, 3);
    ASSERT_EQ(policy->GetMatchedApns().size(), 1);
    EXPECT_EQ(policy->GetMatchedApns()[0], newApn);
}

/**
 * @tc.number   OnPropChanged_002
 * @tc.name     test OnPropChanged with partially parsed numeric value
 * @tc.desc     "30x" 半解析（ec 成功但未到串尾）→ ConvertStrToInt 失败 → 延迟值不变
 */
HWTEST_F(ApnManagerTest, OnPropChanged_002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    ConnectionRetryPolicy::defaultSetupFailDelay_ = 2222;
    policy->OnPropChanged("persist.telephony.setupfail.delay", "30x", nullptr);
    // from_chars 半解析（ec 成功但未到串尾）→ 返回 false 走日志分支，但已把 30 写入出参（现行为）
    EXPECT_EQ(ConnectionRetryPolicy::defaultSetupFailDelay_, 30);
    policy->OnPropChanged("persist.telephony.modemdend.delay", "40y", nullptr);
    // 结束前构造新策略，经真实系统属性还原类级静态值，不向后续用例泄漏
    std::make_shared<ConnectionRetryPolicy>();
}

/**
 * @tc.number   GetNextRetryDelay_003
 * @tc.name     test GetNextRetryDelay in modem deactivate scene
 * @tc.desc     DEFAULT 类型 + RETRY_SCENE_MODEM_DEACTIVATE → 走 modemDend 延迟分支
 */
HWTEST_F(ApnManagerTest, GetNextRetryDelay_003, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    auto delay = policy->GetNextRetryDelay(DATA_CONTEXT_ROLE_DEFAULT, 0, 0,
        RetryScene::RETRY_SCENE_MODEM_DEACTIVATE, false);
    EXPECT_GE(delay, 0);
}

/**
 * @tc.number   IsAllBadApn_002
 * @tc.name     test IsAllBadApn with bad first then good
 * @tc.desc     首项 bad 走完循环继续迭代 → 次项 good 判 false 返回
 */
HWTEST_F(ApnManagerTest, IsAllBadApn_002, TestSize.Level0)
{
    std::shared_ptr<ConnectionRetryPolicy> policy = std::make_shared<ConnectionRetryPolicy>();
    sptr<ApnItem> badApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    badApn->MarkBadApn(true);
    sptr<ApnItem> goodApn = ApnItem::MakeDefaultApn(DATA_CONTEXT_ROLE_DEFAULT);
    goodApn->MarkBadApn(false);
    policy->matchedApns_.push_back(badApn);
    policy->matchedApns_.push_back(goodApn);
    EXPECT_FALSE(policy->IsAllBadApn());
}

/**
 * @tc.number   ApnItem_MakeApn_OverlongFields_001
 * @tc.name     test MakeApn with each overlong field
 * @tc.desc     逐一将单个字段置为超长串（> ALL_APN_ITEM_CHAR_LENGTH=256）→
 *              对应 strcpy_s 失败早退返回 nullptr，覆盖 12 处拷贝失败弧
 */
HWTEST_F(ApnManagerTest, ApnItem_MakeApn_OverlongFields_001, TestSize.Level0)
{
    const std::string huge(300, 'x');
    const std::string normal = "normal";
    for (int field = 0; field < 12; ++field) {
        PdpProfile profile;
        profile.profileId = 1;
        profile.apnTypes = normal;
        profile.mcc = "460";
        profile.mnc = "00";
        profile.pdpProtocol = normal;
        profile.roamPdpProtocol = normal;
        profile.apn = normal;
        profile.profileName = normal;
        profile.authUser = normal;
        profile.authPwd = normal;
        profile.homeUrl = normal;
        profile.proxyIpAddress = normal;
        profile.mmsIpAddress = normal;
        switch (field) {
            case 0: profile.apnTypes = huge; break;          // L95 types_
            case 1: profile.mcc = huge; break;               // L100 numeric_(mcc+mnc)
            case 2: profile.pdpProtocol = huge; break;       // L104 protocol_
            case 3: profile.roamPdpProtocol = huge; break;   // L108 roamingProtocol_
            case 4: profile.apn = huge; break;               // L113 apn_
            case 5: profile.profileName = huge; break;       // L117 apnName_
            case 6: profile.authUser = huge; break;          // L121 user_
            case 7: profile.authPwd = huge; break;           // L125 password_
            case 8: profile.homeUrl = huge; break;           // L134 homeUrl_（BuildOther）
            case 9: profile.proxyIpAddress = huge; break;    // L138 proxyIpAddress_（BuildOther）
            case 10: profile.mmsIpAddress = huge; break;     // L142 mmsIpAddress_（BuildOther）
            case 11: profile.mnc = huge; break;              // L100 numeric_ 另一来源
            default: break;
        }
        EXPECT_EQ(ApnItem::MakeApn(profile), nullptr);
    }
}

/**
 * @tc.number   ApnItem_MakeDefaultApn_Overlong_001
 * @tc.name     test MakeDefaultApn with overlong apn type string
 * @tc.desc     超长类型串 → attr_.types_ 拷贝失败 → 返回 nullptr
 */
HWTEST_F(ApnManagerTest, ApnItem_MakeDefaultApn_Overlong_001, TestSize.Level0)
{
    EXPECT_EQ(ApnItem::MakeDefaultApn(std::string(300, 'x')), nullptr);
}

/**
 * @tc.number   ApnItem_IsSimilarPdpProfile_003
 * @tc.name     test IsSimilarPdpProfile with default-type containing apn types
 * @tc.desc     首条件（双方 apnTypes 含 default 即判不相似）的各方向：
 *              双含 → false；单侧含 → 进入字段比较
 */
HWTEST_F(ApnManagerTest, ApnItem_IsSimilarPdpProfile_003, TestSize.Level0)
{
    PdpProfile p1;
    PdpProfile p2;
    p1.apnTypes = "default,mms";
    p2.apnTypes = "default";
    // 双方均含 default → 直接判不相似
    EXPECT_FALSE(ApnItem::IsSimilarPdpProfile(p1, p2));
    // 仅 new 含 default → 越过首条件进入字段比较（字段一致 → 相似）
    p2.apnTypes = "mms";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
    // 仅 old 含 default → 同样进入字段比较
    p1.apnTypes = "mms";
    p2.apnTypes = "default";
    EXPECT_TRUE(ApnItem::IsSimilarPdpProfile(p1, p2));
}
} // namespace Telephony
} // namespace OHOS
