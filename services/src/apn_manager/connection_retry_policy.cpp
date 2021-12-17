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

#include "connection_retry_policy.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
ConnectionRetryPolicy::ConnectionRetryPolicy() = default;

ConnectionRetryPolicy::~ConnectionRetryPolicy() = default;

sptr<ApnItem> ConnectionRetryPolicy::GetNextRetryApnItem() const
{
    if (matchedApns_.empty()) {
        TELEPHONY_LOGE("matchedApns is null");
        return nullptr;
    }
    if (currentApnIndex_ >= static_cast<int>(matchedApns_.size()) || currentApnIndex_ < 0) {
        currentApnIndex_ = 0;
        return matchedApns_[currentApnIndex_];
    }
    auto apnItem = matchedApns_[currentApnIndex_];
    tryCount_++;
    if ((apnItem != nullptr && apnItem->IsBadApn()) || tryCount_ > maxCount_) {
        tryCount_ = 0;
        currentApnIndex_++;
        return GetNextRetryApnItem();
    }
    return apnItem;
}

std::vector<sptr<ApnItem>> ConnectionRetryPolicy::GetRetryApns() const
{
    return matchedApns_;
}

void ConnectionRetryPolicy::SetMatchedApns(std::vector<sptr<ApnItem>> &apns)
{
    matchedApns_ = apns;
}

void ConnectionRetryPolicy::ClearRetryApns()
{
    matchedApns_.clear();
}

void ConnectionRetryPolicy::MarkBadApn(ApnItem &apn)
{
    // The APN that fails after multiple retries is true
    apn.MarkBadApn(true);
}

int64_t ConnectionRetryPolicy::GetNextRetryDelay() const
{
    return DEFAULT_DELAY_FOR_NEXT_APN;
}
} // namespace Telephony
} // namespace OHOS