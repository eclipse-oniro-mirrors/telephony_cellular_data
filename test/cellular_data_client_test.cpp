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

#include "cellular_data_client.h"
#include "cellular_data_constant.h"
#include "data_access_token.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mock/mock_cellular_data_manager.h"
#include "telephony_errors.h"
#include "telephony_types.h"

namespace OHOS {
namespace Telephony {
using namespace testing;
using namespace testing::ext;

/*
 * 客户端冒烟套件（mock 版）：经 #define private public 向 CellularDataClient::proxy_ 注入
 * ICellularDataManager 的 gmock 桩（MockCellularDataManager），只验证客户端封装
 * （参数传递/返回值透传/默认卡缓存），不做真实 IPC（客户端测客户端）。
 * 断言由原"环境自适应双态"恢复为精确断言：mock 返回值完全受控，与运行环境无关。
 */
class CellularDataClientTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

protected:
    sptr<MockCellularDataManager> mockCellularData_ = nullptr;
};
void CellularDataClientTest::SetUpTestCase() {}

void CellularDataClientTest::TearDownTestCase() {}

void CellularDataClientTest::SetUp()
{
    // 每个用例注入独立 mock 代理，并复位客户端默认卡缓存，避免用例间相互影响
    mockCellularData_ = sptr<MockCellularDataManager>::MakeSptr();
    ASSERT_TRUE(mockCellularData_ != nullptr);
    CellularDataClient::GetInstance().proxy_ = mockCellularData_;
    CellularDataClient::GetInstance().defaultCellularDataSlotId_ = INVALID_MAIN_CARD_SLOTID;
    CellularDataClient::GetInstance().defaultCellularDataSimId_ = 0;
}

void CellularDataClientTest::TearDown()
{
    CellularDataClient::GetInstance().registerStatus_ = false;
    CellularDataClient::GetInstance().proxy_ = nullptr;
    mockCellularData_ = nullptr;
}

/**
 * @tc.number   CellularDataServiceValidTest_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, CellularDataServiceValidTest_001, TestSize.Level0)
{
    auto proxy = CellularDataClient::GetInstance().GetProxy();
    EXPECT_TRUE(proxy != nullptr);
}

/**
 * @tc.number   EnableIntelligenceSwitch_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, EnableIntelligenceSwitch_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, EnableIntelligenceSwitch(false)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().EnableIntelligenceSwitch(false);
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetApnState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetApnState_001, TestSize.Level0)
{
    std::string str;
    EXPECT_CALL(*mockCellularData_, GetApnState(0, str, _))
        .WillOnce(DoAll(SetArgReferee<2>(5), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetApnState(0, str);
    EXPECT_EQ(result, 5);
}

/**
 * @tc.number   GetDataRecoveryState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetDataRecoveryState_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, GetDataRecoveryState(_))
        .WillOnce(DoAll(SetArgReferee<0>(2), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetDataRecoveryState();
    EXPECT_EQ(result, 2);
}

/**
 * @tc.number   IsCellularDataRoamingEnabled_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, IsCellularDataRoamingEnabled_001, TestSize.Level0)
{
    bool dataRoamingEnabled = false;
    EXPECT_CALL(*mockCellularData_, IsCellularDataRoamingEnabled(0, _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(TELEPHONY_ERR_SUCCESS)));
    CellularDataClient::GetInstance().IsCellularDataRoamingEnabled(0, dataRoamingEnabled);
    EXPECT_EQ(dataRoamingEnabled, true);
}

/**
 * @tc.number   ClearCellularDataConnections_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, ClearCellularDataConnections_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, ClearAllConnections(0, _)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().ClearCellularDataConnections(0);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetDataConnApnAttr_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetDataConnApnAttr_001, TestSize.Level0)
{
    ApnItem::Attribute apnAttr;
    EXPECT_CALL(*mockCellularData_, GetDataConnApnAttr(0, _)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().GetDataConnApnAttr(0, apnAttr);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.number   GetDataConnIpType_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetDataConnIpType_001, TestSize.Level0)
{
    std::string ipType;
    EXPECT_CALL(*mockCellularData_, GetDataConnIpType(0, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("IPV4V6")), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetDataConnIpType(0, ipType);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(ipType, "IPV4V6");
}

/**
 * @tc.number   ClearAllConnections_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, ClearAllConnections_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, ClearAllConnections(0, _)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().ClearAllConnections(0, DisConnectionReason::REASON_NORMAL);
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   IsNeedDoRecovery_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, IsNeedDoRecovery_001, TestSize.Level0)
{
    bool needDoRecovery = false;
    EXPECT_CALL(*mockCellularData_, IsNeedDoRecovery(0, false)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    CellularDataClient::GetInstance().IsNeedDoRecovery(0, needDoRecovery);
    EXPECT_EQ(needDoRecovery, false);
}

/**
 * @tc.number   SetDefaultCellularDataSlotId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, SetDefaultCellularDataSlotId_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, RegisterSimAccountCallback(_)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    EXPECT_CALL(*mockCellularData_, SetDefaultCellularDataSlotId(0)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    EXPECT_CALL(*mockCellularData_, GetDefaultCellularDataSimId(_))
        .WillOnce(DoAll(SetArgReferee<0>(100), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().SetDefaultCellularDataSlotId(0);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(CellularDataClient::GetInstance().defaultCellularDataSlotId_, 0);
    EXPECT_EQ(CellularDataClient::GetInstance().defaultCellularDataSimId_, 100);
}

/**
 * @tc.number   EstablishAllApnsIfConnectable_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, EstablishAllApnsIfConnectable_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, EstablishAllApnsIfConnectable(0)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().EstablishAllApnsIfConnectable(0);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetCellularDataSupplierId_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetCellularDataSupplierId_001, TestSize.Level0)
{
    uint32_t supplierId = 0;
    EXPECT_CALL(*mockCellularData_, GetCellularDataSupplierId(0, 0, _))
        .WillOnce(DoAll(SetArgReferee<2>(555), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetCellularDataSupplierId(0, 0, supplierId);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(supplierId, 555);
}

/**
 * @tc.number   CorrectNetSupplierNoAvailable_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, CorrectNetSupplierNoAvailable_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, CorrectNetSupplierNoAvailable(0)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().CorrectNetSupplierNoAvailable(0);
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetSupplierRegisterState_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetSupplierRegisterState_001, TestSize.Level0)
{
    int32_t regState = 0;
    EXPECT_CALL(*mockCellularData_, GetSupplierRegisterState(0, _))
        .WillOnce(DoAll(SetArgReferee<1>(3), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetSupplierRegisterState(0, regState);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(regState, 3);
}

/**
 * @tc.number   OnInitCellularDataController_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, OnInitCellularDataController_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, InitCellularDataController(0)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().InitCellularDataController(0);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetIfSupportDunApn_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetIfSupportDunApn_001, TestSize.Level0)
{
    bool isSupportDun = false;
    EXPECT_CALL(*mockCellularData_, GetIfSupportDunApn(_))
        .WillOnce(DoAll(SetArgReferee<0>(true), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetIfSupportDunApn(isSupportDun);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(isSupportDun, true);
}

/**
 * @tc.number   GetDefaultActReportInfo_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetDefaultActReportInfo_001, TestSize.Level0)
{
    ApnActivateReportInfo info;
    EXPECT_CALL(*mockCellularData_, GetDefaultActReportInfo(0, _)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().GetDefaultActReportInfo(0, info);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetInternalActReportInfo_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetInternalActReportInfo_001, TestSize.Level0)
{
    ApnActivateReportInfo info;
    EXPECT_CALL(*mockCellularData_, GetInternalActReportInfo(0, _)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().GetInternalActReportInfo(0, info);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   SendUrspDecodeResult_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, SendUrspDecodeResult_001, TestSize.Level1)
{
    std::vector<uint8_t> buffer = {};
    EXPECT_CALL(*mockCellularData_, SendUrspDecodeResult(0, _)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().SendUrspDecodeResult(0, buffer);
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   SendUePolicySectionIdentifier_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, SendUePolicySectionIdentifier_001, TestSize.Level1)
{
    std::vector<uint8_t> buffer = {};
    EXPECT_CALL(*mockCellularData_, SendUePolicySectionIdentifier(0, _)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().SendUePolicySectionIdentifier(0, buffer);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   SendImsRsdList_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, SendImsRsdList_001, TestSize.Level1)
{
    std::vector<uint8_t> buffer = {};
    EXPECT_CALL(*mockCellularData_, SendImsRsdList(0, _)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().SendImsRsdList(0, buffer);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetNetworkSliceAllowedNssai_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetNetworkSliceAllowedNssai_001, TestSize.Level1)
{
    std::vector<uint8_t> buffer = {};
    EXPECT_CALL(*mockCellularData_, GetNetworkSliceAllowedNssai(0, _)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().GetNetworkSliceAllowedNssai(0, buffer);
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

/**
 * @tc.number   GetNetworkSliceEhplmn_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetNetworkSliceEhplmn_001, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, GetNetworkSliceEhplmn(0)).WillOnce(Return(TELEPHONY_ERR_SUCCESS));
    int32_t result = CellularDataClient::GetInstance().GetNetworkSliceEhplmn(0);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   GetActiveApnName_001
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetActiveApnName_001, TestSize.Level0)
{
    DataAccessToken token;
    std::string apnName;
    EXPECT_CALL(*mockCellularData_, GetActiveApnName(_))
        .WillOnce(DoAll(SetArgReferee<0>(std::string("cmnet")), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t result = CellularDataClient::GetInstance().GetActiveApnName(apnName);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(apnName, "cmnet");
}

/**
 * @tc.number   GetActiveApnName_002
 * @tc.name     test function branch
 * @tc.desc     Function test
 */
HWTEST_F(CellularDataClientTest, GetActiveApnName_002, TestSize.Level0)
{
    std::string apnName;
    EXPECT_CALL(*mockCellularData_, GetActiveApnName(_)).WillOnce(Return(TELEPHONY_ERR_PERMISSION_ERR));
    int32_t result = CellularDataClient::GetInstance().GetActiveApnName(apnName);
    EXPECT_CALL(*mockCellularData_, AsObject()).WillOnce(Return(sptr<IRemoteObject>(nullptr)));
    CellularDataClient::GetInstance().RemoveDeathRecipient();
    EXPECT_EQ(result, TELEPHONY_ERR_PERMISSION_ERR);
}

HWTEST_F(CellularDataClientTest, GetDefaultCellularDataSlotIdFromProxy, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, GetDefaultCellularDataSlotId(_))
        .WillOnce(DoAll(SetArgReferee<0>(1), Return(TELEPHONY_ERR_SUCCESS)));
    int32_t slotId = CellularDataClient::GetInstance().GetDefaultCellularDataSlotIdFromProxy();
    EXPECT_EQ(slotId, 1);
}

/**
 * @tc.number   CellularDataServiceValidTest_001
 * @tc.name     test get proxy valid
 * @tc.desc     Function test
 * @tc.note     移植自 DT 补丁分支；原版为真实 IPC 断言（依赖设备上 SA 4007 在线）。
 *              按"client 测 client（无真实 IPC）"原则改造：SetUp 已将 mockCellularData_
 *              注入 CellularDataClient::proxy_，GetProxy() 应短路返回注入代理，不触达 samgr。
 */
HWTEST_F(CellularDataClientTest, CellularDataServiceValidTest_001, TestSize.Level3)
{
    EXPECT_TRUE(CellularDataClient::GetInstance().GetProxy() != nullptr);
}
} // namespace Telephony
} // namespace OHOS