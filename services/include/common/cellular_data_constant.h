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

#ifndef CELLULAR_DATA_CONSTANT_H
#define CELLULAR_DATA_CONSTANT_H

#include <string>

#include "cellular_data_types.h"

namespace OHOS {
namespace Telephony {
enum ApnProfileState {
    PROFILE_STATE_IDLE,
    PROFILE_STATE_CONNECTING,
    PROFILE_STATE_CONNECTED,
    PROFILE_STATE_DISCONNECTING,
    PROFILE_STATE_FAILED,
    PROFILE_STATE_RETRYING
};

enum RecoveryState {
    STATE_REQUEST_CONTEXT_LIST,
    STATE_CLEANUP_CONNECTIONS,
    STATE_REREGISTER_NETWORK,
    STATE_RADIO_STATUS_RESTART
};

struct AddressInfo {
    std::string ip;
    std::string netMask;
    uint8_t type;
    uint8_t prefixLen;
};

struct RouteInfo {
    std::string ip;
    uint8_t type;
    std::string destination;
};

struct NetSupplier {
    int32_t supplierId;
    uint32_t netType;
    uint64_t capabilities;
    int32_t slotId;
};

struct NetRequest {
    uint64_t capability;
    std::string ident;
};

struct ActivateInfo {
    int32_t slotId;
    bool isRoaming;
    bool allowRoaming;
};

constexpr int32_t CellularDataStateAdapter(ApnProfileState state)
{
    switch (state) {
        case PROFILE_STATE_CONNECTING:
            return static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTING);
        case PROFILE_STATE_CONNECTED:
            [[fallthrough]]; // fall_through
        case PROFILE_STATE_DISCONNECTING:
            return static_cast<int32_t>(DataConnectionStatus::DATA_STATE_CONNECTED);
        case PROFILE_STATE_FAILED:
            [[fallthrough]]; // fall_through
        case PROFILE_STATE_RETRYING:
            [[fallthrough]]; // fall_through
        case PROFILE_STATE_IDLE:
            return static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
        default:
            return static_cast<int32_t>(DataConnectionStatus::DATA_STATE_DISCONNECTED);
    }
}

enum DataContextRolesId {
    DATA_CONTEXT_ROLE_INVALID_ID = -1,
    DATA_CONTEXT_ROLE_ALL_ID = 0,
    DATA_CONTEXT_ROLE_DEFAULT_ID = 1,
    DATA_CONTEXT_ROLE_MMS_ID = 2,
    DATA_CONTEXT_ROLE_SUPL_ID = 3,
    DATA_CONTEXT_ROLE_DUN_ID = 4,
    DATA_CONTEXT_ROLE_IMS_ID = 5,
    DATA_CONTEXT_ROLE_IA_ID = 6,
    DATA_CONTEXT_ROLE_EMERGENCY_ID = 7
};

enum TelCallStatus {
    CALL_STATUS_UNKNOWN = -1,
    CALL_STATE_ORIGINATE = 0,
    CALL_STATE_PROCEEDING = 1,
    CALL_STATE_ALERTING = 2,
    CALL_STATE_CONNECTED = 3,
    CALL_STATE_RELEASED = 4,
    CALL_STATE_INCOMING = 5,
    CALL_STATE_WAITING = 6,
    CALL_STATE_HOLD = 7,
    CALL_STATE_RETRIEVE = 8,
};

enum DisConnectionReason {
    REASON_NORMAL,
    REASON_GSM_AND_CALLING_ONLY,
    REASON_RETRY_CONNECTION,
    REASON_CLEAR_CONNECTION
};

static const std::string DATA_CONTEXT_ROLE_ALL = "*";
static const std::string DATA_CONTEXT_ROLE_DEFAULT = "default";
static const std::string DATA_CONTEXT_ROLE_MMS = "mms";
static const std::string DATA_CONTEXT_ROLE_SUPL = "supl";
static const std::string DATA_CONTEXT_ROLE_DUN = "dun";
static const std::string DATA_CONTEXT_ROLE_IMS = "ims";
static const std::string DATA_CONTEXT_ROLE_IA = "ia";
static const std::string DATA_CONTEXT_ROLE_EMERGENCY = "emergency";
static const int32_t INVALID_PROFILE_ID = -1;
static const int32_t DATA_PROFILE_DEFAULT = 0;
static const int32_t DATA_PROFILE_MMS = 1;
static const int32_t DEFAULT_SIM_NUM = 1;
static const int32_t CMCC_MCC_MNC = 46002;
static const int32_t DEFAULT_RAT = 2;
static const int32_t DEFAULT_AUTH_TYPE = -1;
static const int32_t DEFAULT_MTU = 1500;
static const uint8_t DEFAULT_STRENGTH = 20;
static const uint32_t DEFAULT_FREQUENCY = 50;
static const int64_t REG_NET_MANAGER_DELAY_TIME = 2000;
static const int64_t CORE_INIT_DELAY_TIME = 1000;
static const int32_t IPV4_BIT = 32;
static const int32_t IPV6_BIT = 128;
static const int32_t SIM_STATE_UNKNOWN = 0;
static const int32_t MIN_SIM = 1;
static const std::string DEFAULT_OPERATOR_NUMERIC = "46001";
static const std::string DATA_METERED_CONTEXT_ROLES = "default";
static const std::string IS_CELLULAR_DATA_ENABLE = "isCellularDataEnable";
static const std::string IS_ROAMING = "isRoaming";
static const std::string SETTING_SWITCH = "settingSwitch";
static const std::string IDENT_PREFIX = "slotId";
static const std::string DEFAULT_HOSTNAME = "os_hostname";
static const std::string DEFAULT_MASK = "";
static const std::string CELLULAR_DATA_RDB_URI = "dataability:///com.ohos.pdpprofileability/net/pdp_profile";
static const int32_t DEFAULT_NET_STATISTICS_PERIOD = 3 * 1000;
static const int32_t DEFAULT_STALL_DETECTION_PERIOD = 10 * 1000;
static const int32_t ESTABLISH_DATA_CONNECTION_DELAY = 1 * 1000;
static const int32_t RECOVERY_TRIGGER_PACKET = 10;
static const int32_t ERROR_APN_ID = -1;
static const int32_t VALID_IP_SIZE = 2;
static const int32_t TYPE_REQUEST_NET = 1;
static const int32_t TYPE_RELEASE_NET = 0;
static const int32_t DEFAULT_READ_APN_TIME = 2;
static const int32_t DEFAULT_MCC_SIZE = 3;
static const int32_t NULL_POINTER_EXCEPTION = -1;
static const std::string ROUTED_IPV4 = "0.0.0.0";
static const std::string ROUTED_IPV6 = "::";
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_DATA_CONSTANT_H
