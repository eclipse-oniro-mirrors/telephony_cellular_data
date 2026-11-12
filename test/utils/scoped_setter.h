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
#ifndef SCOPED_SETTER_H
#define SCOPED_SETTER_H

#include <string>
#include <utility>
#include "parameters.h"

namespace OHOS {
namespace Telephony {
/*
 * RAII 单例/全局成员注入守卫：构造时保存旧值并注入新值，析构时还原旧值。
 * 用例内所有对全局/单例成员的改写（CoreManagerInner::simManager_、
 * CellularDataSettingsRdbHelper::settingsHelper_、TELEPHONY_EXT_WRAPPER 钩子等）
 * 必须经由本守卫，保证用例间互不影响：
 * - 断言失败提前返回也能还原，不向后续用例泄漏 mock 或硬编码空值；
 * - 还原的是进入用例前的原值而非 nullptr，不破坏其他（真机）用例建立的缓存。
 */
template <typename T>
class ScopedSetter {
public:
    ScopedSetter(T &target, T value) : target_(target), saved_(target)
    {
        target_ = std::move(value);
    }
    ~ScopedSetter()
    {
        target_ = std::move(saved_);
    }
    ScopedSetter(const ScopedSetter &) = delete;
    ScopedSetter &operator=(const ScopedSetter &) = delete;

private:
    T &target_;
    T saved_;
};

/*
 * RAII 系统参数守卫：构造时记录原值并写入新值，析构时还原原值。
 * 用例中所有 system::SetParameter 改写系统参数（PERSIST_TSTS_MODE、
 * persist.edm.mobile_data_policy 等）必须经本守卫：设备参数是进程外全局状态，
 * 断言失败提前返回也能还原，不向后续用例/后续轮次泄漏参数状态。
 */
class ScopedSystemParam {
public:
    ScopedSystemParam(const std::string &key, const std::string &value) : key_(key)
    {
        saved_ = system::GetParameter(key, "0");
        system::SetParameter(key, value);
    }
    ~ScopedSystemParam()
    {
        system::SetParameter(key_, saved_);
    }
    ScopedSystemParam(const ScopedSystemParam &) = delete;
    ScopedSystemParam &operator=(const ScopedSystemParam &) = delete;

private:
    std::string key_;
    std::string saved_;
};
} // namespace Telephony
} // namespace OHOS
#endif // SCOPED_SETTER_H
