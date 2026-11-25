/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include <string>
#include "cellular_data_constant.h"
#include "cellular_data_rdb_helper.h"
#include "core_manager_inner.h"
#include "pdp_profile_data.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "utils/scoped_setter.h"
#include "mock/mock_sim_manager.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Telephony;

namespace OHOS {
namespace Telephony {

static PdpProfile g_testPdpProfile;
static int g_currentRow = 0;

/*
 * 列级结果集驱动：GetColumnIndex 按列名分配稳定索引，
 * GetString/GetInt 按列名从下方两个映射取值，实现对
 * MakePdpProfile/GetApnInfo/ReadApnResult 等解析函数的逐列分支控制。
 */
static int g_rowCnt = 0;
static std::map<std::string, int> g_colIdxMap;
static std::map<int, std::string> g_idxNameMap;
static std::map<std::string, std::string> g_colStrVal;
static std::map<std::string, int> g_colIntVal;

/*
 * 测试注入缝隙：重写 CreateDataAbilityHelper 返回可配置的 MockDataShareHelper
 * （或 nullptr），在 UT 中模拟 DataShare 环境成功/失败/空结果等分支。
 */
class TestRdbHelper : public CellularDataRdbHelper {
public:
    TestRdbHelper() = default;
    ~TestRdbHelper() = default;
    std::shared_ptr<DataShare::DataShareHelper> helperToReturn_ = nullptr;

    std::shared_ptr<DataShare::DataShareHelper> CreateDataAbilityHelper(const int waitTime) override
    {
        return helperToReturn_;
    }
};

class CellularDataRdbHelperTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

protected:
    void UseSimMock(int32_t defaultSlotId, int32_t simId);
    std::shared_ptr<DataShareResultSetMock> mockResultSet_;
    std::shared_ptr<DataShare::DataShareResultSet> result_;
    std::shared_ptr<MockDataShareHelper> mockHelper_;
    std::shared_ptr<MockSimManager> mockSim_;
    std::unique_ptr<ScopedSetter<std::shared_ptr<ISimManager>>> simManagerGuard_;
    std::vector<PdpProfile> apnVec_;
};

void CellularDataRdbHelperTest::SetUpTestCase()
{
}

void CellularDataRdbHelperTest::TearDownTestCase()
{
}

void CellularDataRdbHelperTest::SetUp()
{
    mockResultSet_ = std::make_shared<DataShareResultSetMock>();
    result_ = mockResultSet_;
    apnVec_.clear();
    g_testPdpProfile = PdpProfile();
    g_currentRow = 0;
    g_rowCnt = 0;
    g_colIdxMap.clear();
    g_idxNameMap.clear();
    g_colStrVal.clear();
    g_colIntVal.clear();

    mockHelper_ = std::make_shared<MockDataShareHelper>();
    ON_CALL(*mockHelper_, Release()).WillByDefault(Return(true));
    ON_CALL(*mockHelper_, Query(_, _, _, _)).WillByDefault(Return(nullptr));
    ON_CALL(*mockHelper_, Insert(_, _)).WillByDefault(Return(0));
    ON_CALL(*mockHelper_, Update(_, _, _)).WillByDefault(Return(0));
    ON_CALL(*mockHelper_, RegisterObserver(_, _)).WillByDefault(Return(0));
    ON_CALL(*mockHelper_, UnregisterObserver(_, _)).WillByDefault(Return(0));

    ON_CALL(*mockResultSet_, GetRowCount(_))
        .WillByDefault(Invoke([](int &count) {
            count = g_rowCnt;
            return 0;
        }));
    ON_CALL(*mockResultSet_, GoToRow(_)).WillByDefault(Return(0));
    ON_CALL(*mockResultSet_, Close()).WillByDefault(Return(0));
    ON_CALL(*mockResultSet_, GetColumnIndex(_, _))
        .WillByDefault(Invoke([](const std::string &columnName, int &columnIndex) {
            auto it = g_colIdxMap.find(columnName);
            if (it == g_colIdxMap.end()) {
                int id = static_cast<int>(g_colIdxMap.size());
                g_colIdxMap[columnName] = id;
                g_idxNameMap[id] = columnName;
                it = g_colIdxMap.find(columnName);
            }
            columnIndex = it->second;
            return 0;
        }));
    ON_CALL(*mockResultSet_, GetString(_, _))
        .WillByDefault(Invoke([](int columnIndex, std::string &value) {
            auto nameIt = g_idxNameMap.find(columnIndex);
            std::string name = (nameIt != g_idxNameMap.end()) ? nameIt->second : "";
            auto valIt = g_colStrVal.find(name);
            value = (valIt != g_colStrVal.end()) ? valIt->second : "";
            return 0;
        }));
    ON_CALL(*mockResultSet_, GetInt(_, _))
        .WillByDefault(Invoke([](int columnIndex, int &value) {
            auto nameIt = g_idxNameMap.find(columnIndex);
            std::string name = (nameIt != g_idxNameMap.end()) ? nameIt->second : "";
            auto valIt = g_colIntVal.find(name);
            value = (valIt != g_colIntVal.end()) ? valIt->second : 0;
            return 0;
        }));
}

void CellularDataRdbHelperTest::TearDown()
{
    // 先还原单例成员再销毁 mock，避免后续用例读到悬垂指针
    simManagerGuard_.reset();
    mockSim_.reset();
    mockHelper_.reset();
    mockResultSet_.reset();
    result_.reset();
    apnVec_.clear();
    g_rowCnt = 0;
    g_colIdxMap.clear();
    g_idxNameMap.clear();
    g_colStrVal.clear();
    g_colIntVal.clear();
}

void CellularDataRdbHelperTest::UseSimMock(int32_t defaultSlotId, int32_t simId)
{
    mockSim_ = std::make_shared<MockSimManager>();
    EXPECT_CALL(*mockSim_, GetDefaultCellularDataSlotId()).WillRepeatedly(Return(defaultSlotId));
    EXPECT_CALL(*mockSim_, GetSimId(_)).WillRepeatedly(Return(simId));
    // 经守卫注入：TearDown 还原进入用例前的原值，不向其他用例泄漏 mock
    simManagerGuard_ = std::make_unique<ScopedSetter<std::shared_ptr<ISimManager>>>(
        CoreManagerInner::GetInstance().simManager_, mockSim_);
}

/**
 * @tc.number: CellularDataRdbHelper_ReadApnResult_NullResult
 * @tc.name: Test handling of null result set
 * @tc.desc: Verify that ReadApnResult handles null result set correctly by returning without processing
 */
HWTEST_F(CellularDataRdbHelperTest, ReadApnResult_NullResult, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    std::shared_ptr<DataShare::DataShareResultSet> nullResult = nullptr;

    helper.ReadApnResult(nullResult, apnVec_);

    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_ReadApnResult_EmptyResult
 * @tc.name: Test handling of empty result set
 * @tc.desc: Verify that ReadApnResult handles empty result set (rowCnt = 0) correctly
 */
HWTEST_F(CellularDataRdbHelperTest, ReadApnResult_EmptyResult, TestSize.Level1)
{
    int rowCount = 0;
    EXPECT_CALL(*mockResultSet_, GetRowCount(_))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>(rowCount), Return(0)));

    CellularDataRdbHelper helper;
    helper.ReadApnResult(result_, apnVec_);

    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_ReadApnResult_NonMvnoApn
 * @tc.name: Test adding non-MVNO type APN
 * @tc.desc: Verify that non-MVNO type APN (mvnoType is empty) is correctly added to the list
 */
HWTEST_F(CellularDataRdbHelperTest, ReadApnResult_NonMvnoApn, TestSize.Level1)
{
    int rowCount = 1;
    g_testPdpProfile.profileId = 1;
    g_testPdpProfile.mvnoType = "";
    g_testPdpProfile.edited = 0;

    EXPECT_CALL(*mockResultSet_, GetRowCount(_))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>(rowCount), Return(0)));

    for (int i = 0; i < rowCount; ++i) {
        EXPECT_CALL(*mockResultSet_, GoToRow(i))
            .Times(1)
            .WillOnce(Return(0));
        
        // Mock GetColumnIndex and GetInt to return the expected values
        EXPECT_CALL(*mockResultSet_, GetColumnIndex(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetInt(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.edited), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetString(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.mvnoType), Return(0)));
    }

    CellularDataRdbHelper helper;
    helper.ReadApnResult(result_, apnVec_);

    EXPECT_EQ(apnVec_.size(), 1);
    EXPECT_TRUE(apnVec_[0].mvnoType.empty());
}

/**
 * @tc.number: CellularDataRdbHelper_ReadApnResult_EditedMvnoApn
 * @tc.name: Test adding edited MVNO type APN
 * @tc.desc: Verify that user-edited MVNO type APN is correctly added to the list
 */
HWTEST_F(CellularDataRdbHelperTest, ReadApnResult_EditedMvnoApn, TestSize.Level1)
{
    int rowCount = 1;
    g_testPdpProfile.profileId = 2;
    g_testPdpProfile.mvnoType = "imsi";
    g_testPdpProfile.edited = 1;

    EXPECT_CALL(*mockResultSet_, GetRowCount(_))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>(rowCount), Return(0)));

    for (int i = 0; i < rowCount; ++i) {
        EXPECT_CALL(*mockResultSet_, GoToRow(i))
            .Times(1)
            .WillOnce(Return(0));
        
        // Mock GetColumnIndex and GetInt to return the expected values
        EXPECT_CALL(*mockResultSet_, GetColumnIndex(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetInt(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.edited), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetString(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.mvnoType), Return(0)));
    }

    CellularDataRdbHelper helper;
    helper.ReadApnResult(result_, apnVec_);

    EXPECT_EQ(apnVec_.size(), 1);
    EXPECT_EQ(apnVec_[0].mvnoType, "imsi");
    EXPECT_EQ(apnVec_[0].edited, 1);
}

/**
 * @tc.number: CellularDataRdbHelper_ReadApnResult_UneditedMvnoApn
 * @tc.name: Test filtering unedited MVNO type APN
 * @tc.desc: Verify that unedited MVNO type APN is correctly filtered out and not added to the list
 */
HWTEST_F(CellularDataRdbHelperTest, ReadApnResult_UneditedMvnoApn, TestSize.Level1)
{
    int rowCount = 1;
    g_testPdpProfile.profileId = 3;
    g_testPdpProfile.mvnoType = "imsi";
    g_testPdpProfile.edited = 0;

    EXPECT_CALL(*mockResultSet_, GetRowCount(_))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>(rowCount), Return(0)));

    for (int i = 0; i < rowCount; ++i) {
        EXPECT_CALL(*mockResultSet_, GoToRow(i))
            .Times(1)
            .WillOnce(Return(0));
        
        // Mock GetColumnIndex and GetInt to return the expected values
        EXPECT_CALL(*mockResultSet_, GetColumnIndex(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetInt(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.edited), Return(0)));
        
        EXPECT_CALL(*mockResultSet_, GetString(_, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(g_testPdpProfile.mvnoType), Return(0)));
    }

    CellularDataRdbHelper helper;
    helper.ReadApnResult(result_, apnVec_);

    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_GetSimId_001
 * @tc.name: Test GetSimId with invalid default slot id
 * @tc.desc: Verify GetSimId returns -1 when default cellular data slot id is invalid
 */
HWTEST_F(CellularDataRdbHelperTest, GetSimId_001, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    UseSimMock(-1, 1);
    EXPECT_EQ(helper.GetSimId(), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_GetSimId_002
 * @tc.name: Test GetSimId with slot id out of range
 * @tc.desc: Verify GetSimId returns -1 when default slot id exceeds max slot count
 */
HWTEST_F(CellularDataRdbHelperTest, GetSimId_002, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    UseSimMock(99, 1);
    EXPECT_EQ(helper.GetSimId(), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_GetSimId_003
 * @tc.name: Test GetSimId with invalid sim id
 * @tc.desc: Verify GetSimId returns -1 when sim id is not positive
 */
HWTEST_F(CellularDataRdbHelperTest, GetSimId_003, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    UseSimMock(0, 0);
    EXPECT_EQ(helper.GetSimId(), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_GetSimId_004
 * @tc.name: Test GetSimId with valid values
 * @tc.desc: Verify GetSimId returns sim id when both slot id and sim id are valid
 */
HWTEST_F(CellularDataRdbHelperTest, GetSimId_004, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    UseSimMock(0, 1);
    EXPECT_EQ(helper.GetSimId(), 1);
}

/**
 * @tc.number: CellularDataRdbHelper_GetOpKey_001
 * @tc.name: Test GetOpKey without core service
 * @tc.desc: Verify GetOpKey returns empty string when core service is unavailable in UT environment
 */
HWTEST_F(CellularDataRdbHelperTest, GetOpKey_001, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    std::string opkey = helper.GetOpKey(0);
    EXPECT_TRUE(opkey.empty());
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApns_001
 * @tc.name: Test QueryApns with null data share helper
 * @tc.desc: Verify QueryApns returns false and sets error message when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApns_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    std::string errMsg;
    EXPECT_FALSE(helper.QueryApns("460", "00", apnVec_, 0, errMsg));
    EXPECT_EQ(errMsg, "dataShareHelper is null");
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApns_002
 * @tc.name: Test QueryApns with null query result
 * @tc.desc: Verify QueryApns returns false when query returns null result set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApns_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    std::string errMsg;
    EXPECT_FALSE(helper.QueryApns("460", "00", apnVec_, 0, errMsg));
    EXPECT_EQ(errMsg, "query apns error");
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApns_003
 * @tc.name: Test QueryApns with empty result
 * @tc.desc: Verify QueryApns returns true with error message when result set has no rows
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApns_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 0;
    std::string errMsg;
    EXPECT_TRUE(helper.QueryApns("460", "00", apnVec_, 0, errMsg));
    EXPECT_EQ(errMsg, "read apn result empty");
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApns_004
 * @tc.name: Test QueryApns with valid row
 * @tc.desc: Verify QueryApns returns true and parses a valid non-MVNO row into apnVec
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApns_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colIntVal[PdpProfileData::PROFILE_ID] = 1;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "";
    std::string errMsg;
    EXPECT_TRUE(helper.QueryApns("460", "00", apnVec_, 0, errMsg));
    EXPECT_TRUE(errMsg.empty());
    EXPECT_EQ(apnVec_.size(), 1);
    EXPECT_EQ(apnVec_[0].profileId, 1);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_001
 * @tc.name: Test QueryMvnoApnsByType with empty sim data
 * @tc.desc: Verify QueryMvnoApnsByType returns true early when mvnoDataFromSim is empty
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    EXPECT_TRUE(helper.QueryMvnoApnsByType("460", "00", "imsi", "", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_002
 * @tc.name: Test QueryMvnoApnsByType with null helper
 * @tc.desc: Verify QueryMvnoApnsByType returns false when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    EXPECT_FALSE(helper.QueryMvnoApnsByType("460", "00", "imsi", "460001", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_003
 * @tc.name: Test QueryMvnoApnsByType with null query result
 * @tc.desc: Verify QueryMvnoApnsByType returns false when query returns null result set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_FALSE(helper.QueryMvnoApnsByType("460", "00", "imsi", "460001", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_004
 * @tc.name: Test QueryMvnoApnsByType with matched IMSI row
 * @tc.desc: Verify QueryMvnoApnsByType parses a regex-matched IMSI MVNO row into mvnoApnVec
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colIntVal[PdpProfileData::PROFILE_ID] = 7;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "imsi";
    g_colStrVal[PdpProfileData::MVNO_MATCH_DATA] = "4600\\d*";
    EXPECT_TRUE(helper.QueryMvnoApnsByType("460", "00", "imsi", "460001", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 1);
    EXPECT_EQ(apnVec_[0].profileId, 7);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_005
 * @tc.name: Test QueryMvnoApnsByType with unmatched IMSI row
 * @tc.desc: Verify QueryMvnoApnsByType filters out regex-unmatched MVNO rows
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_005, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "imsi";
    g_colStrVal[PdpProfileData::MVNO_MATCH_DATA] = "46001";
    EXPECT_TRUE(helper.QueryMvnoApnsByType("460", "00", "imsi", "46000", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_006
 * @tc.name: Test QueryMvnoApnsByType with matched GID1 row
 * @tc.desc: Verify QueryMvnoApnsByType keeps a GID1 prefix-matched MVNO row
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_006, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "gid1";
    g_colStrVal[PdpProfileData::MVNO_MATCH_DATA] = "AABB";
    EXPECT_TRUE(helper.QueryMvnoApnsByType("460", "00", "gid1", "AABBCC", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 1);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryMvnoApnsByType_007
 * @tc.name: Test QueryMvnoApnsByType with invalid regex match data
 * @tc.desc: Verify QueryMvnoApnsByType filters out rows when regex match data is invalid
 */
HWTEST_F(CellularDataRdbHelperTest, QueryMvnoApnsByType_007, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "spn";
    g_colStrVal[PdpProfileData::MVNO_MATCH_DATA] = "[";
    EXPECT_TRUE(helper.QueryMvnoApnsByType("460", "00", "spn", "CMCC", apnVec_, 0));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryPreferApn_001
 * @tc.name: Test QueryPreferApn with null helper
 * @tc.desc: Verify QueryPreferApn returns false when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, QueryPreferApn_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    EXPECT_FALSE(helper.QueryPreferApn(0, apnVec_));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryPreferApn_002
 * @tc.name: Test QueryPreferApn with null query result
 * @tc.desc: Verify QueryPreferApn returns false when query returns null result set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryPreferApn_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_FALSE(helper.QueryPreferApn(0, apnVec_));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryPreferApn_003
 * @tc.name: Test QueryPreferApn with empty result
 * @tc.desc: Verify QueryPreferApn returns true when no prefer apn is set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryPreferApn_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 0;
    EXPECT_TRUE(helper.QueryPreferApn(0, apnVec_));
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryPreferApn_004
 * @tc.name: Test QueryPreferApn with valid row
 * @tc.desc: Verify QueryPreferApn parses a valid prefer apn row into apnVec
 */
HWTEST_F(CellularDataRdbHelperTest, QueryPreferApn_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 1;
    g_colIntVal[PdpProfileData::PROFILE_ID] = 3;
    g_colStrVal[PdpProfileData::MVNO_TYPE] = "";
    EXPECT_TRUE(helper.QueryPreferApn(0, apnVec_));
    EXPECT_EQ(apnVec_.size(), 1);
    EXPECT_EQ(apnVec_[0].profileId, 3);
}

/**
 * @tc.number: CellularDataRdbHelper_ResetApns_001
 * @tc.name: Test ResetApns with null helper
 * @tc.desc: Verify ResetApns returns false when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, ResetApns_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    EXPECT_FALSE(helper.ResetApns(0));
}

/**
 * @tc.number: CellularDataRdbHelper_ResetApns_002
 * @tc.name: Test ResetApns with update failure
 * @tc.desc: Verify ResetApns returns false when update returns negative result
 */
HWTEST_F(CellularDataRdbHelperTest, ResetApns_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(-1));
    EXPECT_FALSE(helper.ResetApns(0));
}

/**
 * @tc.number: CellularDataRdbHelper_ResetApns_003
 * @tc.name: Test ResetApns with update success
 * @tc.desc: Verify ResetApns returns true when update succeeds
 */
HWTEST_F(CellularDataRdbHelperTest, ResetApns_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(0));
    EXPECT_TRUE(helper.ResetApns(0));
}

/**
 * @tc.number: CellularDataRdbHelper_Insert_001
 * @tc.name: Test Insert with null helper
 * @tc.desc: Verify Insert returns NULL_POINTER_EXCEPTION when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, Insert_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    DataShare::DataShareValuesBucket values;
    EXPECT_EQ(helper.Insert(values), NULL_POINTER_EXCEPTION);
}

/**
 * @tc.number: CellularDataRdbHelper_Insert_002
 * @tc.name: Test Insert with success
 * @tc.desc: Verify Insert returns row id when helper insert succeeds
 */
HWTEST_F(CellularDataRdbHelperTest, Insert_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    DataShare::DataShareValuesBucket values;
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(10));
    EXPECT_EQ(helper.Insert(values), 10);
}

/**
 * @tc.number: CellularDataRdbHelper_Update_001
 * @tc.name: Test Update with null helper
 * @tc.desc: Verify Update returns NULL_POINTER_EXCEPTION when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, Update_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    DataShare::DataShareValuesBucket value;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(helper.Update(value, predicates), NULL_POINTER_EXCEPTION);
}

/**
 * @tc.number: CellularDataRdbHelper_Update_002
 * @tc.name: Test Update with success
 * @tc.desc: Verify Update returns result code when helper update succeeds
 */
HWTEST_F(CellularDataRdbHelperTest, Update_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    DataShare::DataShareValuesBucket value;
    DataShare::DataSharePredicates predicates;
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(0));
    EXPECT_EQ(helper.Update(value, predicates), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_SetPreferApn_001
 * @tc.name: Test SetPreferApn with invalid sim id
 * @tc.desc: Verify SetPreferApn returns -1 when sim id is invalid
 */
HWTEST_F(CellularDataRdbHelperTest, SetPreferApn_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 0);
    EXPECT_EQ(helper.SetPreferApn(1), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_SetPreferApn_002
 * @tc.name: Test SetPreferApn with null helper
 * @tc.desc: Verify SetPreferApn returns -1 when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, SetPreferApn_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    EXPECT_EQ(helper.SetPreferApn(1), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_SetPreferApn_003
 * @tc.name: Test SetPreferApn with update failure
 * @tc.desc: Verify SetPreferApn returns -1 when update returns negative result
 */
HWTEST_F(CellularDataRdbHelperTest, SetPreferApn_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(helper.SetPreferApn(1), -1);
}

/**
 * @tc.number: CellularDataRdbHelper_SetPreferApn_004
 * @tc.name: Test SetPreferApn with update success
 * @tc.desc: Verify SetPreferApn returns 0 when update succeeds
 */
HWTEST_F(CellularDataRdbHelperTest, SetPreferApn_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(0));
    EXPECT_EQ(helper.SetPreferApn(1), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_RegisterObserver_001
 * @tc.name: Test RegisterObserver with null helper
 * @tc.desc: Verify RegisterObserver returns early without crash when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, RegisterObserver_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    helper.RegisterObserver(dataObserver);
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_RegisterObserver_002
 * @tc.name: Test RegisterObserver with valid helper
 * @tc.desc: Verify RegisterObserver registers all four uris on the data share helper
 */
HWTEST_F(CellularDataRdbHelperTest, RegisterObserver_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    EXPECT_CALL(*mockHelper_, RegisterObserver(_, _)).Times(4).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockHelper_, Release()).WillOnce(Return(true));
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    helper.RegisterObserver(dataObserver);
}

/**
 * @tc.number: CellularDataRdbHelper_UnRegisterObserver_001
 * @tc.name: Test UnRegisterObserver with null helper
 * @tc.desc: Verify UnRegisterObserver returns early without crash when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, UnRegisterObserver_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    helper.UnRegisterObserver(dataObserver);
    EXPECT_EQ(apnVec_.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_UnRegisterObserver_002
 * @tc.name: Test UnRegisterObserver with valid helper
 * @tc.desc: Verify UnRegisterObserver unregisters all four uris on the data share helper
 */
HWTEST_F(CellularDataRdbHelperTest, UnRegisterObserver_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    EXPECT_CALL(*mockHelper_, UnregisterObserver(_, _)).Times(4).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockHelper_, Release()).WillOnce(Return(true));
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    helper.UnRegisterObserver(dataObserver);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApnIds_001
 * @tc.name: Test QueryApnIds with invalid sim id
 * @tc.desc: Verify QueryApnIds returns early when GetSimId is invalid
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApnIds_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(-1, 1);
    ApnInfo apnInfo;
    std::vector<uint32_t> apnIdList;
    helper.QueryApnIds(apnInfo, apnIdList);
    EXPECT_EQ(apnIdList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApnIds_002
 * @tc.name: Test QueryApnIds with null helper
 * @tc.desc: Verify QueryApnIds returns early when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApnIds_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    ApnInfo apnInfo;
    std::vector<uint32_t> apnIdList;
    helper.QueryApnIds(apnInfo, apnIdList);
    EXPECT_EQ(apnIdList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApnIds_003
 * @tc.name: Test QueryApnIds with null query result
 * @tc.desc: Verify QueryApnIds returns early when query returns null result set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApnIds_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    ApnInfo apnInfo;
    std::vector<uint32_t> apnIdList;
    helper.QueryApnIds(apnInfo, apnIdList);
    EXPECT_EQ(apnIdList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApnIds_004
 * @tc.name: Test QueryApnIds with unfilled optional fields
 * @tc.desc: Verify QueryApnIds skips optional predicates when apn info fields are not filled in
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApnIds_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    ApnInfo apnInfo;
    apnInfo.apnName = u"ChinaNet";
    apnInfo.apn = u"cmnet";
    apnInfo.mcc = u"460";
    apnInfo.mnc = u"00";
    apnInfo.user = Str8ToStr16(NOT_FILLED_IN);
    apnInfo.type = Str8ToStr16(NOT_FILLED_IN);
    apnInfo.proxy = Str8ToStr16(NOT_FILLED_IN);
    apnInfo.mmsproxy = Str8ToStr16(NOT_FILLED_IN);
    g_rowCnt = 2;
    g_colIntVal[PdpProfileData::PROFILE_ID] = 5;
    std::vector<uint32_t> apnIdList;
    helper.QueryApnIds(apnInfo, apnIdList);
    EXPECT_EQ(apnIdList.size(), 2);
    EXPECT_EQ(apnIdList[0], 5);
    EXPECT_EQ(apnIdList[1], 5);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryApnIds_005
 * @tc.name: Test QueryApnIds with filled optional fields
 * @tc.desc: Verify QueryApnIds adds optional predicates when apn info fields are filled
 */
HWTEST_F(CellularDataRdbHelperTest, QueryApnIds_005, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    ApnInfo apnInfo;
    apnInfo.apnName = u"ChinaNet";
    apnInfo.apn = u"cmnet";
    apnInfo.mcc = u"460";
    apnInfo.mnc = u"00";
    apnInfo.user = u"user";
    apnInfo.type = u"default";
    apnInfo.proxy = u"10.0.0.1";
    apnInfo.mmsproxy = u"10.0.0.2";
    g_rowCnt = 1;
    g_colIntVal[PdpProfileData::PROFILE_ID] = 6;
    std::vector<uint32_t> apnIdList;
    helper.QueryApnIds(apnInfo, apnIdList);
    EXPECT_EQ(apnIdList.size(), 1);
    EXPECT_EQ(apnIdList[0], 6);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryAllApnInfo_001
 * @tc.name: Test QueryAllApnInfo with invalid sim id
 * @tc.desc: Verify QueryAllApnInfo returns early when GetSimId is invalid
 */
HWTEST_F(CellularDataRdbHelperTest, QueryAllApnInfo_001, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 0);
    std::vector<ApnInfo> apnInfoList;
    helper.QueryAllApnInfo(apnInfoList);
    EXPECT_EQ(apnInfoList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryAllApnInfo_002
 * @tc.name: Test QueryAllApnInfo with null helper
 * @tc.desc: Verify QueryAllApnInfo returns early when helper creation fails
 */
HWTEST_F(CellularDataRdbHelperTest, QueryAllApnInfo_002, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = nullptr;
    UseSimMock(0, 1);
    std::vector<ApnInfo> apnInfoList;
    helper.QueryAllApnInfo(apnInfoList);
    EXPECT_EQ(apnInfoList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryAllApnInfo_003
 * @tc.name: Test QueryAllApnInfo with null query result
 * @tc.desc: Verify QueryAllApnInfo returns early when query returns null result set
 */
HWTEST_F(CellularDataRdbHelperTest, QueryAllApnInfo_003, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    std::vector<ApnInfo> apnInfoList;
    helper.QueryAllApnInfo(apnInfoList);
    EXPECT_EQ(apnInfoList.size(), 0);
}

/**
 * @tc.number: CellularDataRdbHelper_QueryAllApnInfo_004
 * @tc.name: Test QueryAllApnInfo with two valid rows
 * @tc.desc: Verify QueryAllApnInfo parses each row into ApnInfo entries
 */
HWTEST_F(CellularDataRdbHelperTest, QueryAllApnInfo_004, TestSize.Level1)
{
    TestRdbHelper helper;
    helper.helperToReturn_ = mockHelper_;
    UseSimMock(0, 1);
    EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillOnce(Return(result_));
    g_rowCnt = 2;
    g_colStrVal[PdpProfileData::PROFILE_NAME] = "ChinaNet";
    g_colStrVal[PdpProfileData::APN] = "cmnet";
    g_colStrVal[PdpProfileData::MCC] = "460";
    g_colStrVal[PdpProfileData::MNC] = "00";
    g_colStrVal[PdpProfileData::AUTH_USER] = "user";
    g_colStrVal[PdpProfileData::APN_TYPES] = "default";
    g_colStrVal[PdpProfileData::PROXY_IP_ADDRESS] = "10.0.0.1";
    g_colStrVal[PdpProfileData::MMS_IP_ADDRESS] = "10.0.0.2";
    std::vector<ApnInfo> apnInfoList;
    helper.QueryAllApnInfo(apnInfoList);
    EXPECT_EQ(apnInfoList.size(), 2);
    EXPECT_EQ(Str16ToStr8(apnInfoList[0].apnName), "ChinaNet");
    EXPECT_EQ(Str16ToStr8(apnInfoList[0].apn), "cmnet");
    EXPECT_EQ(Str16ToStr8(apnInfoList[0].mcc), "460");
    EXPECT_EQ(Str16ToStr8(apnInfoList[0].mnc), "00");
    EXPECT_EQ(Str16ToStr8(apnInfoList[1].proxy), "10.0.0.1");
    EXPECT_EQ(Str16ToStr8(apnInfoList[1].mmsproxy), "10.0.0.2");
}

/**
 * @tc.number: CellularDataRdbHelper_GetApnInfo_001
 * @tc.name: Test GetApnInfo direct parsing
 * @tc.desc: Verify GetApnInfo reads all columns of the given row into ApnInfo
 */
HWTEST_F(CellularDataRdbHelperTest, GetApnInfo_001, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    g_colStrVal[PdpProfileData::PROFILE_NAME] = "ChinaWap";
    g_colStrVal[PdpProfileData::APN] = "cmwap";
    g_colStrVal[PdpProfileData::MCC] = "460";
    g_colStrVal[PdpProfileData::MNC] = "02";
    g_colStrVal[PdpProfileData::AUTH_USER] = "";
    g_colStrVal[PdpProfileData::APN_TYPES] = "mms";
    g_colStrVal[PdpProfileData::PROXY_IP_ADDRESS] = "";
    g_colStrVal[PdpProfileData::MMS_IP_ADDRESS] = "10.0.0.3";
    ApnInfo apnInfo;
    helper.GetApnInfo(apnInfo, 0, result_);
    EXPECT_EQ(Str16ToStr8(apnInfo.apnName), "ChinaWap");
    EXPECT_EQ(Str16ToStr8(apnInfo.apn), "cmwap");
    EXPECT_EQ(Str16ToStr8(apnInfo.mcc), "460");
    EXPECT_EQ(Str16ToStr8(apnInfo.mnc), "02");
    EXPECT_TRUE(apnInfo.user.empty());
    EXPECT_EQ(Str16ToStr8(apnInfo.type), "mms");
    EXPECT_TRUE(apnInfo.proxy.empty());
    EXPECT_EQ(Str16ToStr8(apnInfo.mmsproxy), "10.0.0.3");
}

/**
 * @tc.number: CellularDataRdbHelper_MakePdpProfile_001
 * @tc.name: Test MakePdpProfile with authType -1 and empty user
 * @tc.desc: Verify MakePdpProfile sets authType to NONE when authType is -1 and authUser is empty
 */
HWTEST_F(CellularDataRdbHelperTest, MakePdpProfile_001, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    g_colIntVal[PdpProfileData::AUTH_TYPE] = -1;
    g_colStrVal[PdpProfileData::AUTH_USER] = "";
    PdpProfile apnBean;
    helper.MakePdpProfile(result_, 0, apnBean);
    EXPECT_EQ(apnBean.authType, SETUP_DATA_AUTH_NONE);
    EXPECT_EQ(apnBean.pdpProtocol, "IP");
    EXPECT_EQ(apnBean.roamPdpProtocol, "IP");
}

/**
 * @tc.number: CellularDataRdbHelper_MakePdpProfile_002
 * @tc.name: Test MakePdpProfile with authType -1 and non-empty user
 * @tc.desc: Verify MakePdpProfile sets authType to PAP_CHAP when authType is -1 and authUser is set
 */
HWTEST_F(CellularDataRdbHelperTest, MakePdpProfile_002, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    g_colIntVal[PdpProfileData::AUTH_TYPE] = -1;
    g_colStrVal[PdpProfileData::AUTH_USER] = "user";
    PdpProfile apnBean;
    helper.MakePdpProfile(result_, 0, apnBean);
    EXPECT_EQ(apnBean.authType, SETUP_DATA_AUTH_PAP_CHAP);
}

/**
 * @tc.number: CellularDataRdbHelper_MakePdpProfile_003
 * @tc.name: Test MakePdpProfile with explicit authType and protocols
 * @tc.desc: Verify MakePdpProfile keeps explicit authType and non-empty protocol values
 */
HWTEST_F(CellularDataRdbHelperTest, MakePdpProfile_003, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    g_colIntVal[PdpProfileData::AUTH_TYPE] = 2;
    g_colStrVal[PdpProfileData::APN_PROTOCOL] = "IPV6";
    g_colStrVal[PdpProfileData::APN_ROAM_PROTOCOL] = "IPV4V6";
    PdpProfile apnBean;
    helper.MakePdpProfile(result_, 0, apnBean);
    EXPECT_EQ(apnBean.authType, 2);
    EXPECT_EQ(apnBean.pdpProtocol, "IPV6");
    EXPECT_EQ(apnBean.roamPdpProtocol, "IPV4V6");
}

/**
 * @tc.number: CellularDataRdbHelper_IsMvnoDataMatched_001
 * @tc.name: Test IsMvnoDataMatched with empty sim data
 * @tc.desc: Verify IsMvnoDataMatched returns false when mvnoDataFromSim is empty
 */
HWTEST_F(CellularDataRdbHelperTest, IsMvnoDataMatched_001, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    PdpProfile apnBean;
    apnBean.mvnoType = "imsi";
    apnBean.mvnoMatchData = "4600\\d*";
    EXPECT_FALSE(helper.IsMvnoDataMatched("", apnBean));
}

/**
 * @tc.number: CellularDataRdbHelper_IsMvnoDataMatched_002
 * @tc.name: Test IsMvnoDataMatched with ICCID match
 * @tc.desc: Verify IsMvnoDataMatched returns true for a regex-matched ICCID type
 */
HWTEST_F(CellularDataRdbHelperTest, IsMvnoDataMatched_002, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    PdpProfile apnBean;
    apnBean.mvnoType = "iccid";
    apnBean.mvnoMatchData = "8986\\d*";
    EXPECT_TRUE(helper.IsMvnoDataMatched("8986001", apnBean));
}

/**
 * @tc.number: CellularDataRdbHelper_IsMvnoDataMatched_003
 * @tc.name: Test IsMvnoDataMatched with SPN mismatch
 * @tc.desc: Verify IsMvnoDataMatched returns false for a regex-unmatched SPN type
 */
HWTEST_F(CellularDataRdbHelperTest, IsMvnoDataMatched_003, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    PdpProfile apnBean;
    apnBean.mvnoType = "spn";
    apnBean.mvnoMatchData = "CMCC";
    EXPECT_FALSE(helper.IsMvnoDataMatched("UNICOM", apnBean));
}

/**
 * @tc.number: CellularDataRdbHelper_IsMvnoDataMatched_004
 * @tc.name: Test IsMvnoDataMatched with GID1 match and mismatch
 * @tc.desc: Verify IsMvnoDataMatched uses prefix comparison for GID1 type
 */
HWTEST_F(CellularDataRdbHelperTest, IsMvnoDataMatched_004, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    PdpProfile apnBean;
    apnBean.mvnoType = "gid1";
    apnBean.mvnoMatchData = "AABB";
    EXPECT_TRUE(helper.IsMvnoDataMatched("AABBCC", apnBean));
    EXPECT_FALSE(helper.IsMvnoDataMatched("ABBBCC", apnBean));
}

/**
 * @tc.number: CellularDataRdbHelper_IsMvnoDataMatched_005
 * @tc.name: Test IsMvnoDataMatched with unknown mvno type
 * @tc.desc: Verify IsMvnoDataMatched returns false for an unsupported mvno type
 */
HWTEST_F(CellularDataRdbHelperTest, IsMvnoDataMatched_005, TestSize.Level1)
{
    CellularDataRdbHelper helper;
    PdpProfile apnBean;
    apnBean.mvnoType = "unknown";
    apnBean.mvnoMatchData = ".*";
    EXPECT_FALSE(helper.IsMvnoDataMatched("460001", apnBean));
}

} // namespace Telephony
} // namespace OHOS
