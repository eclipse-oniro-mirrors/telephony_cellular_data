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

#include <iostream>
#include <unistd.h>
#include <map>
#include <string>

#include "core_manager.h"

#include "cellular_data_constant.h"
#include "cellular_data_client.h"

namespace OHOS {
namespace Telephony {
class CellularDataCodeTest {
    using Fun = void (*)();
public:
    CellularDataCodeTest() = default;

    ~CellularDataCodeTest() = default;

    static void IsCellularDataEnabledTest()
    {
        int32_t result = CellularDataClient::GetInstance().IsCellularDataEnabled();
        std::cout << "TelephonyTestService Remote IsCellularDataEnabled result [" << result << "]" << std::endl;
    }

    static void EnableCellularDataTest()
    {
        const int32_t maxTestCount = 61;
        const uint32_t spaceTime = 1000 * 100 * 2.6;
        const int32_t useMaxType = 1;
        const int32_t evenNumber = 2;
        int32_t type = 0;
        std::cout << "please input whether enable: enable/1,disable/0 " << std::endl;
        std::cin >> type;
        int32_t result = 0;
        if (type > useMaxType) {
            for (int32_t i = 1; i < maxTestCount; i++) {
                bool dataEnable = i % evenNumber;
                if (dataEnable) {
                    std::cout << "enable====: " << dataEnable << std::endl;
                    CellularDataClient::GetInstance().EnableCellularData(dataEnable);
                    usleep(spaceTime);
                    result = CellularDataClient::GetInstance().GetCellularDataState();
                    std::cout << "Remote GetCellularDataState result [" << result << "]" << std::endl;
                } else {
                    std::cout << "enable false====: " << dataEnable << std::endl;
                    CellularDataClient::GetInstance().EnableCellularData(dataEnable);
                    usleep(spaceTime);
                    result = CellularDataClient::GetInstance().GetCellularDataState();
                    std::cout << "Remote GetCellularDataState result [" << result << "]" << std::endl;
                }
                std::cout << "i := " << i << std::endl;
            }
            return;
        }
        bool enable = (type > 0);
        result = CellularDataClient::GetInstance().EnableCellularData(enable);
        std::cout << "Remote EnableCellularData " << enable <<" result [" << result << "]" << std::endl;
    }

    static void GetCellularDataStateTest()
    {
        int32_t result = CellularDataClient::GetInstance().GetCellularDataState();
        std::cout << "Remote GetCellularDataState result [" << result << "]" << std::endl;
    }

    static void IsCellularDataRoamingEnabledTest()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        std::cout << "please input parameter int slotId" << std::endl;
        std::cin >> slotId;
        int32_t result = CellularDataClient::GetInstance().IsCellularDataRoamingEnabled(slotId);
        std::cout << "Remote IsCellularDataRoamingEnabled result [" << result << "]" << std::endl;
    }

    static void EnableCellularDataRoamingTest()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        int32_t type = 0;
        std::cout << "please input parameter int slotId " << std::endl;
        std::cin >> slotId;
        std::cout << "please input parameter roaming enable: enable/1,disable/0" << std::endl;
        std::cin >> type;
        bool enable = (type > 0);
        int32_t result = CellularDataClient::GetInstance().EnableCellularDataRoaming(slotId, enable);
        std::cout << "Remote GetOperatorName " << enable << " result [" << result << "]" << std::endl;
    }

    static void RequestNetworkTest()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        uint64_t capability = 0;
        std::cout << "please input parameter int slotId" << std::endl;
        std::cin >> slotId;
        std::cout << "please input parameter capability" << std::endl;
        std::cin >> capability;
        int32_t result =
            CellularDataClient::GetInstance().RequestNet(IDENT_PREFIX + std::to_string(slotId), capability);
        std::cout << "TelephonyTestService Remote result [" << result << "]" << std::endl;
    }

    static void ReleaseNetworkTest()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        uint64_t capability = 0;
        std::cout << "please input parameter int slotId" << std::endl;
        std::cin >> slotId;
        std::cout << "please input parameter capability" << std::endl;
        std::cin >> capability;
        int32_t result =
            CellularDataClient::GetInstance().ReleaseNet(IDENT_PREFIX + std::to_string(slotId), capability);
        std::cout << "TelephonyTestService Remote result [" << result << "]" << std::endl;
    }

    static void HandleApnChangedTest()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        std::string apns;
        std::cout << "please input parameter apn" << std::endl;
        std::cin >> apns;
        auto proxy = CellularDataClient::GetInstance().GetProxy();
        if (proxy != nullptr) {
            int32_t result = proxy->HandleApnChanged(slotId, apns);
            std::cout << "TelephonyTestService Remote result [" << result << "]" << std::endl;
        } else {
            std::cout << "TelephonyTestService Remote is null" << std::endl;
        }
    }

    static void GetDefaultSlotId()
    {
        auto proxy = CellularDataClient::GetInstance().GetProxy();
        if (proxy != nullptr) {
            int32_t result = proxy->GetDefaultCellularDataSlotId();
            std::cout << "get default slot id is := " << result << std::endl;
        } else {
            std::cout << "get default slot id fail" << std::endl;
        }
    }

    static void SetDefaultSlotId()
    {
        int32_t slotId = CoreManager::DEFAULT_SLOT_ID;
        std::cout << "please input parameter int slot" << std::endl;
        std::cin >> slotId;
        auto proxy = CellularDataClient::GetInstance().GetProxy();
        if (proxy != nullptr) {
            int32_t  result = proxy->SetDefaultCellularDataSlotId(slotId);
            std::cout << "TelephonyTestService SetDefaultSlotId is " << result << std::endl;
        } else {
            std::cout << "TelephonyTestService SetDefaultSlotId is null" << std::endl;
        }
    }

    static void GetDataFlowType()
    {
        auto proxy = CellularDataClient::GetInstance().GetProxy();
        if (proxy != nullptr) {
            int32_t result = proxy->GetCellularDataFlowType();
            std::cout << "get CellDataFlowType is := " << result << std::endl;
        } else {
            std::cout << "get CellDataFlowType is fail" << std::endl;
        }
    }

    static void TestDataPackageUp()
    {
        system("rm /data/cell_tx");
        system("touch /data/cell_rx");
        if (access("/data/cell_tx", F_OK) == 0) {
            std::cout << "Please manually delete cell_tx file " << std::endl;
            return;
        }
        if (access("/data/cell_tx", F_OK) != 0) {
            std::cout << "Please manually touch cell_tx file" << std::endl;
        }
    }

    static void TestDataPackageDown()
    {
        system("rm /data/cell_rx");
        system("touch /data/cell_tx");
        if (access("/data/cell_rx", F_OK) == 0) {
            std::cout << "Please manually delete cell_rx file " << std::endl;
            return;
        }
        if (access("/data/cell_tx", F_OK) != 0) {
            std::cout << "Please manually touch cell_tx file" << std::endl;
        }
    }

    static void TestDataPackageUpDown()
    {
        system("rm /data/cell_tx");
        system("rm /data/cell_rx");
        if (access("/data/cell_tx", F_OK) == 0 || access("/data/cell_rx", F_OK) == 0) {
            std::cout << "Please manually delete cell_rx and cell_tx file " << std::endl;
            return;
        }
    }

    static bool DataPackageTest(const int32_t testId)
    {
        std::map<int32_t, Fun> testFunMap {
            {TEST_DATA_PACKAGE_UP, &CellularDataCodeTest::TestDataPackageUp},
            {TEST_DATA_PACKAGE_DOWN, &CellularDataCodeTest::TestDataPackageDown},
            {TEST_DATA_PACKAGE_UP_DOWN, &CellularDataCodeTest::TestDataPackageUpDown},

        };
        auto it = testFunMap.find(testId);
        if (it != testFunMap.end()) {
            (*(it->second))();
        } else if (TEST_DATA_PACKAGE_EXIT == testId) {
            std::cout << "exit..." << std::endl;
            return false;
        } else {
            std::cout << "please input correct number..." << std::endl;
        }
        return true;
    }

    static void TouchOrRmDataPackage()
    {
        int32_t  input = 0;
        while (true) {
            std::cout << "\n-----------start--------------\nusage:please input a cmd num:\n"
                         "0:testDataPackageUp\n"
                         "1:testDataPackageDown\n"
                         "2:testDataPackageUpDown\n"
                         "100:exit\n"
                      << std::endl;
            std::cin >> input;
            std::cout << "inputCMD is [" << input << "]" << std::endl;
            if (!DataPackageTest(input)) {
                break;
            }
        }
    }

    bool UnitTest(const int32_t testId) const
    {
        std::map<int32_t, Fun> testFunMap {
            {IS_CELLULAR_DATA_ENABLED_TEST, &IsCellularDataEnabledTest},
            {ENABLE_CELLULAR_DATA_TEST, &EnableCellularDataTest},
            {GET_CELLULAR_DATA_STATE_TEST, &GetCellularDataStateTest},
            {IS_DATA_ROAMING_ENABLED_TEST, &IsCellularDataRoamingEnabledTest},
            {ENABLE_DATA_ROAMING_TEST, &EnableCellularDataRoamingTest},
            {REQUEST_NETWORK_TEST, &RequestNetworkTest},
            {RELEASE_NETWORK_TEST, &ReleaseNetworkTest},
            {APN_CHANGED_TEST, &HandleApnChangedTest},
            {GET_DEFAULT_SLOT_ID, &GetDefaultSlotId},
            {SET_DEFAULT_SLOT_ID, &SetDefaultSlotId},
            {GET_DATA_FLOW_TYPE, &GetDataFlowType},
            {TOUCH_OR_RM_DATA_PACKAGE, &TouchOrRmDataPackage},
        };
        auto it = testFunMap.find(testId);
        if (it != testFunMap.end()) {
            (*(it->second))();
        } else if (EXIT_CELLULAR_DATA_TEST == testId) {
            std::cout << "exit..." << std::endl;
            return false;
        } else {
            std::cout << "please input correct number..." << std::endl;
        }
        return true;
    }

private:
    const int32_t IS_CELLULAR_DATA_ENABLED_TEST = 0;
    const int32_t ENABLE_CELLULAR_DATA_TEST = 1;
    const int32_t GET_CELLULAR_DATA_STATE_TEST = 2;
    const int32_t IS_DATA_ROAMING_ENABLED_TEST = 3;
    const int32_t ENABLE_DATA_ROAMING_TEST = 4;
    const int32_t REQUEST_NETWORK_TEST = 5;
    const int32_t RELEASE_NETWORK_TEST = 6;
    const int32_t APN_CHANGED_TEST = 7;
    const int32_t GET_DEFAULT_SLOT_ID = 8;
    const int32_t SET_DEFAULT_SLOT_ID = 9;
    const int32_t GET_DATA_FLOW_TYPE = 10;
    const int32_t TOUCH_OR_RM_DATA_PACKAGE = 11;
    const int32_t EXIT_CELLULAR_DATA_TEST = 1000;
    static const int32_t TEST_DATA_PACKAGE_UP = 0;
    static const int32_t TEST_DATA_PACKAGE_DOWN = 1;
    static const int32_t TEST_DATA_PACKAGE_UP_DOWN = 2;
    static const int32_t TEST_DATA_PACKAGE_EXIT = 100;
};
} // namespace Telephony
} // namespace OHOS

int main()
{
    auto &cellularDataClient = OHOS::Telephony::CellularDataClient::GetInstance();
    if (cellularDataClient.GetProxy() == nullptr) {
        std::cout << "\n--- telephonyService == nullptr\n" << std::endl;
        return 0;
    }
    int32_t inputCMD = 0;
    OHOS::Telephony::CellularDataCodeTest cellularDataCodeTest;
    while (true) {
        std::cout << "\n-----------start--------------\nusage:please input a cmd num:\n"
             "0:IsCellularDataEnabledTest\n"
             "1:EnableCellularDataTest\n"
             "2:GetCellularDataStateTest\n"
             "3:IsCellularDataRoamingEnabledTest\n"
             "4:EnableCellularDataRoamingTest\n"
             "5:RequestNetwork\n"
             "6:ReleaseNetwork\n"
             "7:ApnChangedTest\n"
             "8:GetDefaultSlotId\n"
             "9:SetDefaultSlotId\n"
             "10:GetCellularDataFlowType\n"
             "11:TouchOrRmDataPackage\n"
             "1000:exit\n"
             << std::endl;
        std::cin >> inputCMD;
        std::cout << "inputCMD is [" << inputCMD << "]" << std::endl;
        if (!cellularDataCodeTest.UnitTest(inputCMD)) {
            break;
        }
    }
    return 0;
}