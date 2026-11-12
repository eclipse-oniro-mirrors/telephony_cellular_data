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

#include <cstdio>
#include <cstring>

#include "apn_activate_report_info.h"
#include "apn_attribute.h"
#include "cellular_data_client.h"
#include "cellular_data_constant.h"
#include "data_access_token.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ipc_object_stub.h"
#include "mock/mock_cellular_data_manager.h"
#include "parcel.h"
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

/*
 * ==== P2 batch-2：frameworks/native 零覆盖文件（apn_attribute.cpp 140 / apn_activate_report_info.cpp 42 分支）====
 * 两者均为纯 IPC 序列化/字段转移辅助（libtel_cellular_data_api so 侧），真 Parcel 进程内可回环，无 mock。
 * 覆盖策略：
 *   1) 全字段回环 = 全部写成功 + 读成功分支（RewindRead(0) 回读）；
 *   2) 前缀截断（只写前 k 个字段）= 第 k+1 个读取早退分支（k 遍历全部字段序）；
 *   3) 超长字符串（> Parcel 默认容量 200K）= 对应字符串字段的写失败分支
 *      （WriteInt32/WriteBool 无法精确触发容量耗尽，对应 4 个写失败分支保持未覆盖，可接受）。
 */
namespace {
// 与 ApnAttribute::Marshalling 完全一致的 16 字段写入顺序，写前 count 个字段
void WriteApnAttributePrefix(Parcel &parcel, const ApnAttribute &src, size_t count)
{
    if (count > 0) { parcel.WriteString(src.types_); }
    if (count > 1) { parcel.WriteString(src.numeric_); }
    if (count > 2) { parcel.WriteInt32(src.profileId_); }
    if (count > 3) { parcel.WriteString(src.protocol_); }
    if (count > 4) { parcel.WriteString(src.roamingProtocol_); }
    if (count > 5) { parcel.WriteInt32(src.authType_); }
    if (count > 6) { parcel.WriteString(src.apn_); }
    if (count > 7) { parcel.WriteString(src.apnName_); }
    if (count > 8) { parcel.WriteString(src.user_); }
    if (count > 9) { parcel.WriteString(src.password_); }
    if (count > 10) { parcel.WriteBool(src.isRoamingApn_); }
    if (count > 11) { parcel.WriteString(src.homeUrl_); }
    if (count > 12) { parcel.WriteString(src.proxyIpAddress_); }
    if (count > 13) { parcel.WriteString(src.mmsIpAddress_); }
    if (count > 14) { parcel.WriteBool(src.isEdited_); }
}

ApnAttribute MakeFullApnAttribute()
{
    ApnAttribute src;
    src.types_ = "default,mms";
    src.numeric_ = "46001";
    src.profileId_ = 1;
    src.protocol_ = "IPV4V6";
    src.roamingProtocol_ = "IPV6";
    src.authType_ = 3;
    src.apn_ = "cmnet";
    src.apnName_ = "cmnet_name";
    src.user_ = "user01";
    src.password_ = "pwd01";
    src.isRoamingApn_ = true;
    src.homeUrl_ = "http://home";
    src.proxyIpAddress_ = "10.10.10.1";
    src.mmsIpAddress_ = "10.10.10.2";
    src.isEdited_ = true;
    return src;
}
} // namespace

/**
 * @tc.number   ApnAttribute_Parcel_RoundTrip_001
 * @tc.name     test apn attribute parcel round trip
 * @tc.desc     Function test
 * @tc.note     全字段 Marshalling/RewindRead(0)/Unmarshalling 回环，覆盖全部写成功+读成功分支。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Parcel_RoundTrip_001, TestSize.Level3)
{
    ApnAttribute src = MakeFullApnAttribute();
    Parcel parcel;
    EXPECT_TRUE(src.Marshalling(parcel));
    EXPECT_TRUE(parcel.RewindRead(0));
    ApnAttribute *dst = ApnAttribute::Unmarshalling(parcel);
    ASSERT_NE(dst, nullptr);
    EXPECT_EQ(dst->types_, src.types_);
    EXPECT_EQ(dst->numeric_, src.numeric_);
    EXPECT_EQ(dst->profileId_, src.profileId_);
    EXPECT_EQ(dst->protocol_, src.protocol_);
    EXPECT_EQ(dst->roamingProtocol_, src.roamingProtocol_);
    EXPECT_EQ(dst->authType_, src.authType_);
    EXPECT_EQ(dst->apn_, src.apn_);
    EXPECT_EQ(dst->apnName_, src.apnName_);
    EXPECT_EQ(dst->user_, src.user_);
    EXPECT_EQ(dst->password_, src.password_);
    EXPECT_EQ(dst->isRoamingApn_, src.isRoamingApn_);
    EXPECT_EQ(dst->homeUrl_, src.homeUrl_);
    EXPECT_EQ(dst->proxyIpAddress_, src.proxyIpAddress_);
    EXPECT_EQ(dst->mmsIpAddress_, src.mmsIpAddress_);
    EXPECT_EQ(dst->isEdited_, src.isEdited_);
    delete dst;
}

/**
 * @tc.number   ApnAttribute_Parcel_TruncatedRead_001
 * @tc.name     test apn attribute unmarshalling with truncated parcel
 * @tc.desc     Function test
 * @tc.note     只写前 k 个字段（k=0..14），Unmarshalling 在第 k+1 个读取处早退 → 覆盖全部读取失败分支。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Parcel_TruncatedRead_001, TestSize.Level3)
{
    ApnAttribute src = MakeFullApnAttribute();
    for (size_t k = 0; k < 15; ++k) {
        Parcel parcel;
        WriteApnAttributePrefix(parcel, src, k);
        EXPECT_EQ(ApnAttribute::Unmarshalling(parcel), nullptr);
    }
}

/**
 * @tc.number   ApnAttribute_Parcel_WriteFail_001
 * @tc.name     test apn attribute marshalling write failure
 * @tc.desc     Function test
 * @tc.note     将第 idx 个字符串字段置为 300K 超长串（> Parcel 默认容量 200K），
 *              Marshalling 在该字段写入处失败 → 覆盖各字符串字段写失败分支。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Parcel_WriteFail_001, TestSize.Level3)
{
    const std::string huge(300000, 'x'); // > Parcel 默认容量 204800
    const int stringFieldIdx[] = { 0, 1, 3, 4, 6, 7, 8, 9, 11, 12, 13 };
    for (int idx : stringFieldIdx) {
        ApnAttribute src = MakeFullApnAttribute();
        switch (idx) {
            case 0: src.types_ = huge; break;
            case 1: src.numeric_ = huge; break;
            case 3: src.protocol_ = huge; break;
            case 4: src.roamingProtocol_ = huge; break;
            case 6: src.apn_ = huge; break;
            case 7: src.apnName_ = huge; break;
            case 8: src.user_ = huge; break;
            case 9: src.password_ = huge; break;
            case 11: src.homeUrl_ = huge; break;
            case 12: src.proxyIpAddress_ = huge; break;
            case 13: src.mmsIpAddress_ = huge; break;
            default: break;
        }
        Parcel parcel;
        EXPECT_FALSE(src.Marshalling(parcel));
    }
}

/**
 * @tc.number   ApnAttribute_Transfer_001
 * @tc.name     test apn attribute transfer before/after ipc
 * @tc.desc     Function test
 * @tc.note     BeforeIpc：char 数组 → std::string 全字段转移；AfterIpc：反向转移含
 *              255 长度截断（std::string::copy 到 ALL_APN_ITEM_CHAR_LENGTH-1）。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Transfer_001, TestSize.Level3)
{
    ApnItem::Attribute attr = {};
    snprintf(attr.types_, sizeof(attr.types_), "default,mms");
    snprintf(attr.numeric_, sizeof(attr.numeric_), "46001");
    attr.profileId_ = 1;
    snprintf(attr.protocol_, sizeof(attr.protocol_), "IPV4V6");
    snprintf(attr.roamingProtocol_, sizeof(attr.roamingProtocol_), "IPV6");
    attr.authType_ = 3;
    snprintf(attr.apn_, sizeof(attr.apn_), "cmnet");
    snprintf(attr.apnName_, sizeof(attr.apnName_), "cmnet_name");
    snprintf(attr.user_, sizeof(attr.user_), "user01");
    snprintf(attr.password_, sizeof(attr.password_), "pwd01");
    attr.isRoamingApn_ = true;
    snprintf(attr.homeUrl_, sizeof(attr.homeUrl_), "http://home");
    snprintf(attr.proxyIpAddress_, sizeof(attr.proxyIpAddress_), "10.10.10.1");
    snprintf(attr.mmsIpAddress_, sizeof(attr.mmsIpAddress_), "10.10.10.2");
    attr.isEdited_ = true;

    ApnAttribute trans;
    ApnAttribute::TransferApnAttributeBeforeIpc(attr, trans);
    EXPECT_EQ(trans.types_, std::string("default,mms"));
    EXPECT_EQ(trans.numeric_, std::string("46001"));
    EXPECT_EQ(trans.profileId_, 1);
    EXPECT_EQ(trans.protocol_, std::string("IPV4V6"));
    EXPECT_EQ(trans.roamingProtocol_, std::string("IPV6"));
    EXPECT_EQ(trans.authType_, 3);
    EXPECT_EQ(trans.apn_, std::string("cmnet"));
    EXPECT_EQ(trans.user_, std::string("user01"));
    EXPECT_TRUE(trans.isRoamingApn_);
    EXPECT_TRUE(trans.isEdited_);

    // AfterIpc 反向转移 + 超长截断：源串 300 字符 → char[256] 数组截断为 255
    ApnItem::Attribute back = {};
    trans.apn_ = std::string(300, 'y');
    ApnAttribute::TransferApnAttributeAfterIpc(back, trans);
    EXPECT_EQ(strlen(back.apn_), ApnItem::ALL_APN_ITEM_CHAR_LENGTH - 1);
    EXPECT_EQ(back.profileId_, trans.profileId_);
    EXPECT_EQ(back.authType_, trans.authType_);
    EXPECT_TRUE(back.isRoamingApn_);
    EXPECT_TRUE(back.isEdited_);
}

/**
 * @tc.number   ApnActivateReportInfo_Parcel_001
 * @tc.name     test apn activate report info parcel and transfer
 * @tc.desc     Function test
 * @tc.note     4 字段回环（全写/读成功分支）+ 前缀截断 k=0..3（读失败分支）+ transferToReportInfo。
 */
HWTEST_F(CellularDataClientTest, ApnActivateReportInfo_Parcel_001, TestSize.Level3)
{
    ApnActivateReportInfoIpc src;
    src.actTimes = 7;
    src.averDuration = 1200;
    src.topReason = 12;
    src.actSuccTimes = 5;

    Parcel parcel;
    EXPECT_TRUE(src.Marshalling(parcel));
    EXPECT_TRUE(parcel.RewindRead(0));
    ApnActivateReportInfoIpc *dst = ApnActivateReportInfoIpc::Unmarshalling(parcel);
    ASSERT_NE(dst, nullptr);
    EXPECT_EQ(dst->actTimes, 7U);
    EXPECT_EQ(dst->averDuration, 1200U);
    EXPECT_EQ(dst->topReason, 12U);
    EXPECT_EQ(dst->actSuccTimes, 5U);
    delete dst;

    // 前缀截断：只写前 k 个字段（k=0..3），Unmarshalling 早退 → 覆盖全部读取失败分支
    for (uint32_t k = 0; k < 4; ++k) {
        Parcel truncated;
        if (k > 0) { truncated.WriteUint32(src.actTimes); }
        if (k > 1) { truncated.WriteUint32(src.averDuration); }
        if (k > 2) { truncated.WriteUint32(src.topReason); }
        EXPECT_EQ(ApnActivateReportInfoIpc::Unmarshalling(truncated), nullptr);
    }

    ApnActivateReportInfo info = {};
    ApnActivateReportInfoIpc::transferToReportInfo(src, info);
    EXPECT_EQ(info.actTimes, 7U);
    EXPECT_EQ(info.averDuration, 1200U);
    EXPECT_EQ(info.topReason, 12U);
    EXPECT_EQ(info.actSuccTimes, 5U);
}

/**
 * @tc.number   OnRemoteDied_001
 * @tc.name     test OnRemoteDied with non-null remote but null proxy
 * @tc.desc     remote 非空越过首判；proxy_ 为空 → 直接返回不触碰成员
 */
HWTEST_F(CellularDataClientTest, OnRemoteDied_001, TestSize.Level0)
{
    CellularDataClient::GetInstance().proxy_ = nullptr;
    sptr<IPCObjectStub> stub = new IPCObjectStub();
    wptr<IRemoteObject> remote(stub);
    CellularDataClient::GetInstance().OnRemoteDied(remote);
    EXPECT_EQ(CellularDataClient::GetInstance().proxy_, nullptr);
}

/**
 * @tc.number   OnRemoteDied_002
 * @tc.name     test OnRemoteDied with proxy whose AsObject returns null
 * @tc.desc     proxy_ 非空但 AsObject 为 null → serviceRemote 空短路，不清理代理
 */
HWTEST_F(CellularDataClientTest, OnRemoteDied_002, TestSize.Level0)
{
    EXPECT_CALL(*mockCellularData_, AsObject()).WillOnce(Return(sptr<IRemoteObject>(nullptr)));
    sptr<IPCObjectStub> stub = new IPCObjectStub();
    wptr<IRemoteObject> remote(stub);
    CellularDataClient::GetInstance().OnRemoteDied(remote);
    EXPECT_NE(CellularDataClient::GetInstance().proxy_, nullptr);
}

/**
 * @tc.number   OnRemoteDied_003
 * @tc.name     test OnRemoteDied with remote different from proxy's service object
 * @tc.desc     serviceRemote 与 remote 不为同一对象 → 不清理代理
 */
HWTEST_F(CellularDataClientTest, OnRemoteDied_003, TestSize.Level0)
{
    sptr<IPCObjectStub> stubA = new IPCObjectStub();
    sptr<IPCObjectStub> stubB = new IPCObjectStub();
    EXPECT_CALL(*mockCellularData_, AsObject()).WillRepeatedly(Return(sptr<IRemoteObject>(stubA)));
    wptr<IRemoteObject> remote(stubB);
    CellularDataClient::GetInstance().OnRemoteDied(remote);
    EXPECT_NE(CellularDataClient::GetInstance().proxy_, nullptr);
}

/**
 * @tc.number   OnRemoteDied_004
 * @tc.name     test OnRemoteDied with remote matching proxy's service object
 * @tc.desc     serviceRemote 与 remote 同一对象 → 清理代理并复位默认卡缓存
 */
HWTEST_F(CellularDataClientTest, OnRemoteDied_004, TestSize.Level0)
{
    sptr<IPCObjectStub> stub = new IPCObjectStub();
    EXPECT_CALL(*mockCellularData_, AsObject()).WillRepeatedly(Return(sptr<IRemoteObject>(stub)));
    CellularDataClient::GetInstance().defaultCellularDataSlotId_ = 0;
    CellularDataClient::GetInstance().registerStatus_ = true;
    wptr<IRemoteObject> remote(stub);
    CellularDataClient::GetInstance().OnRemoteDied(remote);
    EXPECT_EQ(CellularDataClient::GetInstance().proxy_, nullptr);
    EXPECT_EQ(CellularDataClient::GetInstance().defaultCellularDataSlotId_,
        static_cast<int32_t>(INVALID_MAIN_CARD_SLOTID));
    EXPECT_FALSE(CellularDataClient::GetInstance().registerStatus_);
}

/**
 * @tc.number   ApnAttribute_Parcel_LongStringRoundTrip_001
 * @tc.name     test apn attribute parcel round trip with long strings
 * @tc.desc     全字段使用 100 字符长串（超过 libc++ SSO 22 字符阈值，走堆分配路径）
 *              完整 Marshalling/RewindRead/Unmarshalling 回环，覆盖序列化内联
 *              std::string 弧的堆路径（既有用例均为短串 SSO 路径）。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Parcel_LongStringRoundTrip_001, TestSize.Level3)
{
    ApnAttribute src;
    src.types_ = std::string(100, 'a');
    src.numeric_ = std::string(100, 'b');
    src.profileId_ = 1;
    src.protocol_ = std::string(100, 'c');
    src.roamingProtocol_ = std::string(100, 'd');
    src.authType_ = 3;
    src.apn_ = std::string(100, 'e');
    src.apnName_ = std::string(100, 'f');
    src.user_ = std::string(100, 'g');
    src.password_ = std::string(100, 'h');
    src.isRoamingApn_ = true;
    src.homeUrl_ = std::string(100, 'i');
    src.proxyIpAddress_ = std::string(100, 'j');
    src.mmsIpAddress_ = std::string(100, 'k');
    src.isEdited_ = true;
    Parcel parcel;
    EXPECT_TRUE(src.Marshalling(parcel));
    EXPECT_TRUE(parcel.RewindRead(0));
    ApnAttribute *dst = ApnAttribute::Unmarshalling(parcel);
    ASSERT_NE(dst, nullptr);
    EXPECT_EQ(dst->types_, src.types_);
    EXPECT_EQ(dst->numeric_, src.numeric_);
    EXPECT_EQ(dst->protocol_, src.protocol_);
    EXPECT_EQ(dst->roamingProtocol_, src.roamingProtocol_);
    EXPECT_EQ(dst->apn_, src.apn_);
    EXPECT_EQ(dst->apnName_, src.apnName_);
    EXPECT_EQ(dst->user_, src.user_);
    EXPECT_EQ(dst->password_, src.password_);
    EXPECT_EQ(dst->isRoamingApn_, src.isRoamingApn_);
    EXPECT_EQ(dst->homeUrl_, src.homeUrl_);
    EXPECT_EQ(dst->proxyIpAddress_, src.proxyIpAddress_);
    EXPECT_EQ(dst->mmsIpAddress_, src.mmsIpAddress_);
    EXPECT_EQ(dst->isEdited_, src.isEdited_);
    delete dst;
}

/**
 * @tc.number   ApnAttribute_Parcel_LongStringTruncatedRead_001
 * @tc.name     test apn attribute unmarshalling with truncated parcel and long strings
 * @tc.desc     只写前 k 个字段（k=0..14，字段取 100 字符长串走堆路径），
 *              Unmarshalling 在第 k+1 个读取处早退 → 覆盖读取失败分支的堆路径内联弧。
 */
HWTEST_F(CellularDataClientTest, ApnAttribute_Parcel_LongStringTruncatedRead_001, TestSize.Level3)
{
    ApnAttribute src;
    src.types_ = std::string(100, 'a');
    src.numeric_ = std::string(100, 'b');
    src.profileId_ = 1;
    src.protocol_ = std::string(100, 'c');
    src.roamingProtocol_ = std::string(100, 'd');
    src.authType_ = 3;
    src.apn_ = std::string(100, 'e');
    src.apnName_ = std::string(100, 'f');
    src.user_ = std::string(100, 'g');
    src.password_ = std::string(100, 'h');
    src.isRoamingApn_ = true;
    src.homeUrl_ = std::string(100, 'i');
    src.proxyIpAddress_ = std::string(100, 'j');
    src.mmsIpAddress_ = std::string(100, 'k');
    src.isEdited_ = true;
    for (size_t k = 0; k < 15; ++k) {
        Parcel parcel;
        WriteApnAttributePrefix(parcel, src, k);
        EXPECT_EQ(ApnAttribute::Unmarshalling(parcel), nullptr);
    }
}

/**
 * @tc.number   ApnActivateReportInfo_Parcel_WriteFail_001
 * @tc.name     test apn activate report info marshalling write failure
 * @tc.desc     SetMaxCapacity 不约束 WriteUint32，改用"大串填占容量"：先二分找到
 *              Parcel 可写的最大串长，再细扫定位"填充后恰好还剩 4k 字节可写"的长度
 *              （k=0..3）→ Marshalling 第 k+1 个 Uint32 写失败早退，覆盖 4 处写失败
 *              分支（读失败已由 _001 前缀截断覆盖）。
 */
HWTEST_F(CellularDataClientTest, ApnActivateReportInfo_Parcel_WriteFail_001, TestSize.Level3)
{
    ApnActivateReportInfoIpc src;
    src.actTimes = 7;
    src.averDuration = 1200;
    src.topReason = 12;
    src.actSuccTimes = 5;
    // 二分找 Parcel 默认容量下可写入的最大串长
    size_t lo = 0;
    size_t hi = 1 << 20;
    while (lo < hi) {
        size_t mid = (lo + hi + 1) / 2;
        Parcel probe;
        if (probe.WriteString(std::string(mid, 'x'))) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    const size_t maxFill = lo;
    ASSERT_GE(maxFill, 64u);
    // 细扫：填充 maxFill-d 字节后，探测还能连续写几个 Uint32
    size_t fillFor[4] = { 0, 0, 0, 0 };
    for (size_t d = 0; d <= 48; ++d) {
        Parcel parcel;
        if (!parcel.WriteString(std::string(maxFill - d, 'x'))) {
            continue;
        }
        size_t okCnt = 0;
        for (size_t i = 0; i < 4; ++i) {
            if (!parcel.WriteUint32(1)) {
                break;
            }
            ++okCnt;
        }
        if (okCnt < 4 && fillFor[okCnt] == 0) {
            fillFor[okCnt] = maxFill - d;
        }
    }
    for (size_t k = 0; k < 4; ++k) {
        ASSERT_NE(fillFor[k], 0u);
        Parcel parcel;
        ASSERT_TRUE(parcel.WriteString(std::string(fillFor[k], 'x')));
        EXPECT_FALSE(src.Marshalling(parcel));
    }
}
} // namespace Telephony
} // namespace OHOS
