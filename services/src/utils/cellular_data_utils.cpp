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

#include "cellular_data_utils.h"

#include "inet_addr.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace NetManagerStandard;
std::vector<AddressInfo> CellularDataUtils::ParseIpAddr(const std::string &address)
{
    std::vector<AddressInfo> ipInfoArray;
    std::vector<std::string> ipArray = Split(address, " ");
    for (std::string &ipItem: ipArray) {
        AddressInfo ipInfo;
        if (ipItem.find(':') == std::string::npos) {
            ipInfo.type = INetAddr::IpType::IPV4;
        } else {
            ipInfo.type = INetAddr::IpType::IPV6;
        }
        std::vector<std::string> ipData = Split(ipItem, "/");
        ipInfo.ip = ipData[0];
        int prefixLen = 0;
        if (ipData.size() == VALID_IP_SIZE) {
            ipInfo.ip = ipData[0];
            prefixLen = atoi(ipData[1].c_str());
        } else if (ipInfo.type == INetAddr::IpType::IPV4) {
            size_t dotIndex = 0;
            const int ipv4DotLimit = 4;
            for (int i = 0; i < ipv4DotLimit; i++) {
                dotIndex = ipInfo.ip.find('.', dotIndex+1);
                if (dotIndex == std::string::npos) {
                    break;
                }
            }
            if (dotIndex != std::string::npos) {
                ipInfo.netMask = ipInfo.ip.substr(dotIndex + 1);
                ipInfo.ip = ipInfo.ip.substr(0, dotIndex);
            }
        }
        if (prefixLen == 0) {
            if (ipInfo.type == INetAddr::IpType::IPV4) {
                prefixLen = IPV4_BIT;
            } else {
                prefixLen = IPV6_BIT;
            }
        }
        ipInfo.prefixLen = prefixLen;
        ipInfoArray.push_back(ipInfo);
    }
    return ipInfoArray;
}

std::vector<AddressInfo> CellularDataUtils::ParseNormalIpAddr(const std::string &address)
{
    std::vector<AddressInfo> ipInfoArray;
    std::vector<std::string> ipArray = Split(address, " ");
    for (size_t i = 0; i < ipArray.size(); ++i) {
        AddressInfo ipInfo;
        if (ipArray[i].find(':') == std::string::npos) {
            ipInfo.prefixLen = IPV4_BIT;
            ipInfo.type = INetAddr::IpType::IPV4;
        } else {
            ipInfo.prefixLen = IPV6_BIT;
            ipInfo.type = INetAddr::IpType::IPV6;
        }
        ipInfo.ip = ipArray[i];
        ipInfoArray.push_back(ipInfo);
    }
    return ipInfoArray;
}

std::vector<RouteInfo> CellularDataUtils::ParseRoute(const std::string &address)
{
    std::vector<RouteInfo> routeInfoArray;
    std::vector<std::string> routeArray = Split(address, " ");
    for (size_t i = 0; i < routeArray.size(); ++i) {
        RouteInfo route;
        if (routeArray[i].find(':') == std::string::npos) {
            route.type = INetAddr::IpType::IPV4;
            route.destination = ROUTED_IPV4;
        } else {
            route.type = INetAddr::IpType::IPV6;
            route.destination = ROUTED_IPV6;
        }
        route.ip = routeArray[i];
        routeInfoArray.push_back(route);
    }
    return routeInfoArray;
}

std::vector<std::string> CellularDataUtils::Split(const std::string &input, const std::string &flag)
{
    std::vector<std::string> vec;
    if (input.empty()) {
        TELEPHONY_LOGE("input is null");
        return vec;
    }
    std::string::size_type start = 0;
    std::string::size_type pos = 0;
    while ((pos = input.find(flag, start)) != std::string::npos) {
        vec.push_back(input.substr(start, pos - start));
        start = pos + flag.size();
    }
    if (start != input.size()) {
        vec.push_back(input.substr(start, input.size() - start));
    }
    return vec;
}

bool CellularDataUtils::IsDigit(const std::string &data)
{
    if (data.empty()) {
        TELEPHONY_LOGE("data is null");
        return false;
    }
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[0] == '-') {
            continue;
        }
        if (!isdigit(data[i])) {
            TELEPHONY_LOGE("data %{public}s is not digit!", data.c_str());
            return false;
        }
    }
    return true;
}
} // namespace Telephony
} // namespace OHOS