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

#include "cellular_data_state_machine.h"

#include "telephony_log_wrapper.h"

#include "activating.h"
#include "active.h"
#include "apn_manager.h"
#include "cellular_data_utils.h"
#include "default.h"
#include "disconnecting.h"
#include "inactive.h"
#include "network_search_utils.h"
#include "ril_adapter_utils.h"

namespace OHOS {
using namespace NetManagerStandard;
namespace Telephony {
bool CellularDataStateMachine::IsInactiveState() const
{
    return currentState_ == inActiveState_;
}

void CellularDataStateMachine::SetCapability(uint64_t capability)
{
    capability_ = capability;
}

uint64_t CellularDataStateMachine::GetCapability() const
{
    return capability_;
}

int32_t CellularDataStateMachine::GetCid() const
{
    return cid_;
}

void CellularDataStateMachine::SetCid(const int32_t cid)
{
    cid_ = cid;
}

int32_t CellularDataStateMachine::GetSlotId() const
{
    if (cdConnectionManager_ == nullptr) {
        TELEPHONY_LOGE("cdConnectionManager_ is null");
        return CoreManager::DEFAULT_SLOT_ID;
    }
    return cdConnectionManager_->GetSlotId();
}

sptr<ApnItem> CellularDataStateMachine::GetApnItem() const
{
    return apnItem_;
}

void CellularDataStateMachine::DoConnect(const DataConnectionParams &connectionParams)
{
    if (connectionParams.GetApnHolder() == nullptr) {
        TELEPHONY_LOGE("apnHolder is null");
        return;
    }
    apnId_ = ApnManager::FindApnIdByApnName(connectionParams.GetApnHolder()->GetApnType());
    auto apn = connectionParams.GetApnHolder()->GetCurrentApn();
    apnItem_ = apn;
    if (apnItem_ == nullptr) {
        TELEPHONY_LOGE("apnItem is null");
        return;
    }
    ActivateInfo activeInfo;
    const int32_t slotId = GetSlotId();
    activeInfo.slotId = slotId;
    activeInfo.allowRoaming = true;
    activeInfo.isRoaming = false;
    ITelRilManager::CellularDataProfile dataProfile(apn->attr_.profileId_, std::string(apn->attr_.apn_),
        std::string(apn->attr_.protocol_), apn->attr_.authType_, std::string(apn->attr_.user_),
        std::string(apn->attr_.password_), std::string(apn->attr_.roamingProtocol_));
    int32_t radioTech = NetworkSearchUtils::GetPsRadioTech(slotId);
    auto event = AppExecFwk::InnerEvent::Get(ObserverHandler::RADIO_RIL_SETUP_DATA_CALL,
        ApnManager::FindApnIdByApnName(connectionParams.GetApnHolder()->GetApnType()));
    if (event == nullptr) {
        TELEPHONY_LOGE("event is null");
        return;
    }
    event->SetOwner(stateMachineEventHandler_);
    TELEPHONY_LOGI("Activate PDP context (%{public}d, %{public}s, %{public}s)", apn->attr_.profileId_,
        apn->attr_.apn_, apn->attr_.protocol_);
    RilAdapterUtils::ActivatePdpContext(activeInfo, radioTech, dataProfile, event);
}

void CellularDataStateMachine::FreeConnection(const DataDisconnectParams &params)
{
    const int32_t slotId = GetSlotId();
    int32_t apnId = ApnManager::FindApnIdByApnName(params.GetApnType());
    TELEPHONY_LOGI("Deactivate PDP context cid:%{public}d type:%{public}s id:%{public}d slot:%{public}d",
        cid_, params.GetApnType().c_str(), apnId, slotId);
    auto event = AppExecFwk::InnerEvent::Get(ObserverHandler::RADIO_RIL_DEACTIVATE_DATA_CALL, apnId);
    if (event == nullptr) {
        TELEPHONY_LOGE("get event is null");
        return;
    }
    event->SetOwner(stateMachineEventHandler_);
    RilAdapterUtils::DeactivatePdpContext(slotId, cid_, DEACTIVATE_REASON_NONE, event);
}

bool CellularDataStateMachine::operator==(const CellularDataStateMachine &stateMachine) const
{
    return this->GetCid() == stateMachine.GetCid();
}

void CellularDataStateMachine::Init()
{
    activeState_ = std::make_unique<Active>(
        std::weak_ptr<CellularDataStateMachine>(shared_from_this()), "Active").release();
    inActiveState_ = std::make_unique<Inactive>(
        std::weak_ptr<CellularDataStateMachine>(shared_from_this()), "Inactive").release();
    activatingState_ = std::make_unique<Activating>(
        std::weak_ptr<CellularDataStateMachine>(shared_from_this()), "Activating").release();
    disconnectingState_ = std::make_unique<Disconnecting>(
        std::weak_ptr<CellularDataStateMachine>(shared_from_this()), "Disconnecting").release();
    defaultState_ = std::make_unique<Default>(
        std::weak_ptr<CellularDataStateMachine>(shared_from_this()), "Default").release();
    netSupplierInfo_ = std::make_unique<NetSupplierInfo>().release();
    netLinkInfo_ = std::make_unique<NetLinkInfo>().release();
    if (activeState_ == nullptr || inActiveState_ == nullptr || activatingState_ == nullptr ||
        disconnectingState_ == nullptr || defaultState_ == nullptr || netSupplierInfo_ == nullptr ||
        netLinkInfo_ == nullptr) {
        TELEPHONY_LOGE("memory allocation failed");
        return;
    }
    activeState_->SetParentState(defaultState_);
    inActiveState_->SetParentState(defaultState_);
    activatingState_->SetParentState(defaultState_);
    disconnectingState_->SetParentState(defaultState_);
    StateMachine::SetOriginalState(inActiveState_);
    StateMachine::Start();
}

void CellularDataStateMachine::SetCurrentState(const sptr<State> &&state)
{
    currentState_ = std::move(state);
}

sptr<State> CellularDataStateMachine::GetCurrentState() const
{
    return currentState_;
}

void CellularDataStateMachine::UpdateNetworkInfo(const SetupDataCallResultInfo &dataCallInfo)
{
    TELEPHONY_LOGI("dataCall capability: %{public}" PRIu64", state: %{public}d, addr: %{public}s,"
        "dns: %{public}s, gw: %{public}s", capability_, dataCallInfo.reason,
        dataCallInfo.address.c_str(), dataCallInfo.dns.c_str(), dataCallInfo.gateway.c_str());
    int slotId = GetSlotId();
    std::vector<AddressInfo> ipInfoArray = CellularDataUtils::ParseIpAddr(dataCallInfo.address);
    std::vector<AddressInfo> dnsInfoArray = CellularDataUtils::ParseNormalIpAddr(dataCallInfo.dns);
    std::vector<AddressInfo> routeInfoArray = CellularDataUtils::ParseNormalIpAddr(dataCallInfo.gateway);
    if (ipInfoArray.empty() || dnsInfoArray.empty() || routeInfoArray.empty()) {
        TELEPHONY_LOGE("Verifying network Information(ipInfoArray or dnsInfoArray or routeInfoArray empty)");
        return;
    }
    std::lock_guard<std::mutex> guard(mtx_);
    if (netLinkInfo_ == nullptr || netSupplierInfo_ == nullptr) {
        TELEPHONY_LOGE("start update net info,but netLinkInfo or netSupplierInfo is null!");
        return;
    }
    netLinkInfo_->ifaceName_ = dataCallInfo.netPortName;
    netLinkInfo_->mtu_ = (dataCallInfo.maxTransferUnit == 0) ? DEFAULT_MTU : dataCallInfo.maxTransferUnit;
    ResolveIp(ipInfoArray);
    ResolveDns(dnsInfoArray);
    ResolveRoute(routeInfoArray, dataCallInfo.netPortName);
    netSupplierInfo_->isAvailable_ = (dataCallInfo.active > 0);
    netSupplierInfo_->isRoaming_ = NetworkSearchUtils::GetRoamingState(slotId);
    CellularDataNetAgent &netAgent = CellularDataNetAgent::GetInstance();
    int32_t supplierId = netAgent.GetSupplierId(slotId, capability_);
    netAgent.UpdateNetSupplierInfo(supplierId, netSupplierInfo_);
    netAgent.UpdateNetLinkInfo(supplierId, netLinkInfo_);
}

void CellularDataStateMachine::ResolveIp(std::vector<AddressInfo> &ipInfoArray)
{
    TELEPHONY_LOGI("Resolve Ip ifaceName_: %{public}s, domain_: %{public}s, mtu_: %{public}d, isAvailable_: %{public}d,"
        "isRoaming_:%{public}d", netLinkInfo_->ifaceName_.c_str(), netLinkInfo_->domain_.c_str(), netLinkInfo_->mtu_,
        netSupplierInfo_->isAvailable_, netSupplierInfo_->isRoaming_);
    netLinkInfo_->netAddrList_.clear();
    for (auto ipInfo : ipInfoArray) {
        INetAddr netAddr;
        netAddr.address_ = ipInfo.ip;
        netAddr.family_ = ipInfo.type;
        netAddr.type_ = ipInfo.type;
        netAddr.hostName_ = DEFAULT_HOSTNAME;
        netAddr.netMask_ = ipInfo.netMask.length() > 0 ? ipInfo.netMask : DEFAULT_MASK;
        netAddr.prefixlen_ = ipInfo.prefixLen;
        netLinkInfo_->netAddrList_.push_back(netAddr);
    }
}

void CellularDataStateMachine::ResolveDns(std::vector<AddressInfo> &dnsInfoArray)
{
    netLinkInfo_->dnsList_.clear();
    for (auto dnsInfo : dnsInfoArray) {
        INetAddr dnsAddr;
        dnsAddr.address_ = dnsInfo.ip;
        dnsAddr.family_ = dnsInfo.type;
        dnsAddr.type_ = dnsInfo.type;
        dnsAddr.hostName_ = DEFAULT_HOSTNAME;
        dnsAddr.netMask_ = DEFAULT_MASK;
        dnsAddr.prefixlen_ = dnsInfo.prefixLen;
        netLinkInfo_->dnsList_.push_back(dnsAddr);
    }
}

void CellularDataStateMachine::ResolveRoute(std::vector<AddressInfo> &routeInfoArray, const std::string &name)
{
    netLinkInfo_->routeList_.clear();
    for (auto routeInfo : routeInfoArray) {
        NetManagerStandard::Route route;
        route.iface_ = name;
        route.gateway_.address_ = routeInfo.ip;
        route.gateway_.family_ = routeInfo.type;
        route.gateway_.type_ = routeInfo.type;
        route.gateway_.hostName_ = DEFAULT_HOSTNAME;
        route.gateway_.netMask_ = DEFAULT_MASK;
        route.gateway_.prefixlen_ = routeInfo.prefixLen;
        route.destination_.address_ = "0.0.0.0";
        route.destination_.family_ = routeInfo.type;
        route.destination_.type_ = routeInfo.type;
        route.destination_.hostName_ = DEFAULT_HOSTNAME;
        route.destination_.netMask_ = DEFAULT_MASK;
        route.destination_.prefixlen_ = routeInfo.prefixLen;
        netLinkInfo_->routeList_.push_back(route);
    }
}
} // namespace Telephony
} // namespace OHOS