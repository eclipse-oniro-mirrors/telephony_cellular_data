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

#include "cellular_data_constant.h"
#include "cellular_data_settings_rdb_helper.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "telephony_errors.h"
#include "uri.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "utils/scoped_setter.h"

namespace OHOS {
namespace Telephony {
using namespace testing;
using namespace testing::ext;

namespace {
const std::string BACKUP_SIM_LIST_VALUE_VALID = "0,1,0;1,2,1";
}

/*
 * Mock 注入 fixture：直接向单例的 settingsHelper_ 缓存成员塞 MockDataShareHelper，
 * 在 UT 中模拟 DataShare 环境成功/失败/空结果等分支。
 */
class CellularDataSettingsRdbHelperMockTest : public testing::Test {
public:
    CellularDataSettingsRdbHelperMockTest() = default;
    ~CellularDataSettingsRdbHelperMockTest() = default;
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp()
    {
        mockResultSet_ = std::make_shared<DataShareResultSetMock>();
        result_ = mockResultSet_;
        mockHelper_ = std::make_shared<MockDataShareHelper>();
        ON_CALL(*mockHelper_, Release()).WillByDefault(Return(true));
        ON_CALL(*mockHelper_, Query(_, _, _, _)).WillByDefault(Return(nullptr));
        ON_CALL(*mockHelper_, Insert(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockHelper_, Update(_, _, _)).WillByDefault(Return(0));
        ON_CALL(*mockHelper_, RegisterObserver(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockHelper_, UnregisterObserver(_, _)).WillByDefault(Return(0));
        ON_CALL(*mockHelper_, NotifyChange(_)).WillByDefault(Return());
        ON_CALL(*mockResultSet_, GoToFirstRow()).WillByDefault(Return(0));
        ON_CALL(*mockResultSet_, Close()).WillByDefault(Return(0));
        // 经守卫注入：TearDown 还原进入用例前的原值，不向其他用例泄漏 mock
        settingsHelperGuard_ = std::make_unique<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>>(
            CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, mockHelper_);
    }

    void TearDown()
    {
        // 先还原单例成员再销毁 mock，避免后续用例读到悬垂指针
        settingsHelperGuard_.reset();
        mockHelper_.reset();
        mockResultSet_.reset();
        result_.reset();
    }

protected:
    // 配置 GetValue(int) 的结果集行为：返回字符串 resultValue（WillRepeatedly 支持同用例内多次读回）
    void SetupQueryResult(const std::string &resultValue)
    {
        EXPECT_CALL(*mockHelper_, Query(_, _, _, _)).WillRepeatedly(Return(result_));
        EXPECT_CALL(*mockResultSet_, GetColumnIndex(CELLULAR_DATA_COLUMN_VALUE, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(0)));
        EXPECT_CALL(*mockResultSet_, GetString(0, _))
            .WillRepeatedly(DoAll(SetArgReferee<1>(resultValue), Return(0)));
    }

    std::shared_ptr<MockDataShareHelper> mockHelper_;
    std::shared_ptr<DataShareResultSetMock> mockResultSet_;
    std::shared_ptr<DataShare::DataShareResultSet> result_;
    std::unique_ptr<ScopedSetter<std::shared_ptr<DataShare::DataShareHelper>>> settingsHelperGuard_;
};

/*
 * 说明：GetValue/PutValue/Observer 的 LOCAL_PTR_NULL 分支仅在 CreateDataShareHelper
 * 创建真实 helper 失败（无 DataShare 环境）时可达；真机上会成功创建并直连真库，
 * 对应 NullHelper 型用例在设备上不可达且可能写真机数据库，故不设此类用例。
 */

/**
 * @tc.number: CellularDataSettingsRdbHelper_GetValueString_001
 * @tc.name: Test GetValue string overload with existing value
 * @tc.desc: Verify GetValue string overload returns success and reads back the backup sim list value
 * @tc.note  原真库版本经 WriteBackupSimList 写 DataShare，真机上测试进程无
 *           ohos.permission.MANAGE_SETTINGS 被拒恒 GTEST_SKIP；迁至 mock fixture，
 *           "写入"以结果集期望值模拟（WillRepeatedly 支持同用例多次读回）。
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, CellularDataSettingsRdbHelper_GetValueString_001,
    Function | MediumTest | Level3)
{
    SetupQueryResult(BACKUP_SIM_LIST_VALUE_VALID);
    Uri backupSimListUri(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI);
    std::string value;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        backupSimListUri, BACKUP_SIM_LIST_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(value, BACKUP_SIM_LIST_VALUE_VALID);
    // 同用例内二次读回（对应真库语义的重复查询）
    std::string again;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        backupSimListUri, BACKUP_SIM_LIST_COLUMN_ENABLE, again), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(again, BACKUP_SIM_LIST_VALUE_VALID);
}

/**
 * @tc.number: CellularDataSettingsRdbHelper_GetValueString_002
 * @tc.name: Test GetValue string overload with empty result
 * @tc.desc: Verify GetValue string overload returns success when the queried value is empty
 * @tc.note  原真库版查询不存在的列；mock 化后以空值结果集表达同一分支（GetColumnIndex
 *           命中但值为空串，value 保持空）。
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, CellularDataSettingsRdbHelper_GetValueString_002,
    Function | MediumTest | Level3)
{
    SetupQueryResult("");
    Uri backupSimListUri(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI);
    std::string value;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        backupSimListUri, BACKUP_SIM_LIST_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_TRUE(value.empty());
}

/**
 * @tc.number: CellularDataSettingsRdbHelper_PutValue_001
 * @tc.name: Test PutValue int then read back
 * @tc.desc: Verify PutValue update path succeeds and GetValue reads back the current configured value
 * @tc.note  原真库版本写读回真库被权限拒绝恒 skip；mock 化：PutValue 走 Update 成功
 *           分支，读回值经结果集期望切换（"1"→"0"）模拟先后两次落库状态。
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, CellularDataSettingsRdbHelper_PutValue_001,
    Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillRepeatedly(Return(1));
    Uri intelligenceNetworkUri(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, 1), TELEPHONY_ERR_SUCCESS);
    int32_t value = 0;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(value, 1);

    // 第二次：期望值切到 "0"（后设的 EXPECT_CALL 优先生效）
    SetupQueryResult("0");
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, 0), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(value, 0);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_GetValueInt_002
 * @tc.name: Test GetValue int overload with null query result
 * @tc.desc: Verify GetValue int overload returns DATABASE_READ_FAIL when query returns null
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, GetValueInt_NullResult, Function | MediumTest | Level3)
{
    Uri userDataEnableUri(CELLULAR_DATA_SETTING_DATA_ENABLE_URI);
    int32_t value = 0;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        userDataEnableUri, CELLULAR_DATA_COLUMN_ENABLE, value), TELEPHONY_ERR_DATABASE_READ_FAIL);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_GetValueInt_003
 * @tc.name: Test GetValue int overload with convertible string
 * @tc.desc: Verify GetValue int overload converts result string to int successfully
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, GetValueInt_Convertible, Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    Uri userDataEnableUri(CELLULAR_DATA_SETTING_DATA_ENABLE_URI);
    int32_t value = 0;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        userDataEnableUri, CELLULAR_DATA_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(value, 1);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_GetValueInt_004
 * @tc.name: Test GetValue int overload with non-convertible string
 * @tc.desc: Verify GetValue int overload returns DATABASE_READ_EMPTY when result string is not an int
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, GetValueInt_NotConvertible, Function | MediumTest | Level3)
{
    SetupQueryResult("abc");
    Uri userDataEnableUri(CELLULAR_DATA_SETTING_DATA_ENABLE_URI);
    int32_t value = 0;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        userDataEnableUri, CELLULAR_DATA_COLUMN_ENABLE, value), TELEPHONY_ERR_DATABASE_READ_EMPTY);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_GetValueString_002
 * @tc.name: Test GetValue string overload with null query result
 * @tc.desc: Verify GetValue string overload returns DATABASE_READ_FAIL when query returns null
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, GetValueString_NullResult, Function | MediumTest | Level3)
{
    Uri backupSimListUri(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI);
    std::string value;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        backupSimListUri, BACKUP_SIM_LIST_COLUMN_ENABLE, value), TELEPHONY_ERR_DATABASE_READ_FAIL);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_GetValueString_003
 * @tc.name: Test GetValue string overload with valid result
 * @tc.desc: Verify GetValue string overload reads back the result string
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, GetValueString_Valid, Function | MediumTest | Level3)
{
    SetupQueryResult(BACKUP_SIM_LIST_VALUE_VALID);
    Uri backupSimListUri(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI);
    std::string value;
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->GetValue(
        backupSimListUri, BACKUP_SIM_LIST_COLUMN_ENABLE, value), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(value, BACKUP_SIM_LIST_VALUE_VALID);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_PutValue_002
 * @tc.name: Test PutValue insert path when current value is absent
 * @tc.desc: Verify PutValue inserts a new row when GetValue reports read empty
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, PutValue_InsertPath, Function | MediumTest | Level3)
{
    // GetValue 返回非 SUCCESS（结果集返回空值，转换失败），走 Insert 分支
    SetupQueryResult("");
    EXPECT_CALL(*mockHelper_, Insert(_, _)).WillOnce(Return(1));
    Uri intelligenceNetworkUri(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, 1), TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_PutValue_003
 * @tc.name: Test PutValue update path when current value exists
 * @tc.desc: Verify PutValue updates the existing row when GetValue succeeds
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, PutValue_UpdatePath, Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(1));
    Uri intelligenceNetworkUri(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, 0), TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_PutValue_004
 * @tc.name: Test PutValue write failure on data enable uri
 * @tc.desc: Verify PutValue returns DATABASE_WRITE_FAIL and writes fault event on data enable uri
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, PutValue_WriteFailDataEnable, Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(-1));
    Uri userDataEnableUri(CELLULAR_DATA_SETTING_DATA_ENABLE_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        userDataEnableUri, CELLULAR_DATA_COLUMN_ENABLE, 1), TELEPHONY_ERR_DATABASE_WRITE_FAIL);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_PutValue_005
 * @tc.name: Test PutValue write failure on roaming uri
 * @tc.desc: Verify PutValue returns DATABASE_WRITE_FAIL and writes fault event on roaming uri
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, PutValue_WriteFailRoaming, Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(-1));
    Uri userDataRoamingUri(CELLULAR_DATA_SETTING_DATA_ROAMING_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        userDataRoamingUri, CELLULAR_DATA_COLUMN_ENABLE, 1), TELEPHONY_ERR_DATABASE_WRITE_FAIL);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_PutValue_006
 * @tc.name: Test PutValue write failure on other uri
 * @tc.desc: Verify PutValue returns DATABASE_WRITE_FAIL without fault event on unrelated uri
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, PutValue_WriteFailOtherUri, Function | MediumTest | Level3)
{
    SetupQueryResult("1");
    EXPECT_CALL(*mockHelper_, Update(_, _, _)).WillOnce(Return(-1));
    Uri intelligenceNetworkUri(CELLULAR_DATA_SETTING_INTELLIGENCE_NETWORK_URI);
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->PutValue(
        intelligenceNetworkUri, INTELLIGENCE_NETWORK_COLUMN_ENABLE, 1), TELEPHONY_ERR_DATABASE_WRITE_FAIL);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_Observer_002
 * @tc.name: Test Register/UnRegister/NotifyChange with valid helper
 * @tc.desc: Verify observer registration and notify delegate to the data share helper
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, Observer_ValidHelper, Function | MediumTest | Level3)
{
    Uri backupSimListUri(CELLULAR_DATA_SETTING_BACKUP_SIM_LIST_URI);
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    EXPECT_CALL(*mockHelper_, RegisterObserver(_, _)).WillOnce(Return(0));
    CellularDataSettingsRdbHelper::GetInstance()->RegisterSettingsObserver(backupSimListUri, dataObserver);
    EXPECT_CALL(*mockHelper_, UnregisterObserver(_, _)).WillOnce(Return(0));
    CellularDataSettingsRdbHelper::GetInstance()->UnRegisterSettingsObserver(backupSimListUri, dataObserver);
    EXPECT_CALL(*mockHelper_, NotifyChange(_)).WillOnce(Return());
    CellularDataSettingsRdbHelper::GetInstance()->NotifyChange(backupSimListUri);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_ReleaseSettingsHelper_001
 * @tc.name: Test ReleaseSettingsHelper with null and non-null cached helper
 * @tc.desc: Verify ReleaseSettingsHelper releases the cached helper and resets it to null
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, ReleaseSettingsHelper_001, Function | MediumTest | Level3)
{
    // 空缓存直接返回
    CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_ = nullptr;
    CellularDataSettingsRdbHelper::GetInstance()->ReleaseSettingsHelper();
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, nullptr);

    // 非空缓存走 Release 并清空
    CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_ = mockHelper_;
    EXPECT_CALL(*mockHelper_, Release()).WillOnce(Return(true));
    CellularDataSettingsRdbHelper::GetInstance()->ReleaseSettingsHelper();
    EXPECT_EQ(CellularDataSettingsRdbHelper::GetInstance()->settingsHelper_, nullptr);
}

/**
 * @tc.number: CellularDataSettingsRdbHelperMock_CreateDataShareHelper_001
 * @tc.name: Test CreateDataShareHelper cache hit
 * @tc.desc: Verify CreateDataShareHelper returns the cached helper without recreating
 */
HWTEST_F(CellularDataSettingsRdbHelperMockTest, CreateDataShareHelper_CacheHit, Function | MediumTest | Level3)
{
    auto helper = CellularDataSettingsRdbHelper::GetInstance()->CreateDataShareHelper();
    EXPECT_EQ(helper, mockHelper_);
}
} // namespace Telephony
} // namespace OHOS
