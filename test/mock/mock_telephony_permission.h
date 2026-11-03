/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MOCK_TELEPHONY_PERMISSION_H
#define MOCK_TELEPHONY_PERMISSION_H

#include <gmock/gmock.h>
#include <memory>
#include <mutex>
#include <string>

namespace OHOS {
namespace Telephony {
/*
 * 链接期打桩（对齐 state_registry/test/mock 先例）：测试 target 本地定义
 * TelephonyPermission::CheckPermission / CheckCallerIsSystemApp，抢占
 * libtel_common.so 中的同名符号（可执行文件符号优先），彻底排除
 * AccessTokenKit → atm 服务的真实 IPC 与设备权限差异对测试的影响。
 * 默认（未配置 EXPECT_CALL）返回 false（等价权限拒绝），需要放行的用例
 * 显式 EXPECT_CALL(...).WillRepeatedly(Return(true))，并在用例结束后由
 * fixture TearDown 释放单例，避免期望跨用例残留。
 */
class MockTelephonyPermission {
public:
    MockTelephonyPermission() = default;
    virtual ~MockTelephonyPermission() = default;

    MOCK_METHOD(bool, CheckPermission, (const std::string &permissionName));
    MOCK_METHOD(bool, CheckCallerIsSystemApp, ());

    static std::shared_ptr<MockTelephonyPermission> GetOrCreateMockTelephonyPermission();
    static void ReleaseMockTelephonyPermission();
private:
    static std::mutex mutex_;
    static std::shared_ptr<MockTelephonyPermission> permission_;
};
} // namespace Telephony
} // namespace OHOS
#endif // MOCK_TELEPHONY_PERMISSION_H
