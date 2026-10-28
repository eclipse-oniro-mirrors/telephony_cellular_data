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

#ifndef TEST_ASYNC_UTILS_H
#define TEST_ASYNC_UTILS_H

#include <unistd.h>

#include "cellular_data_handler.h"
#include "state_machine.h"
#include "tel_event_handler.h"

namespace OHOS {
namespace Telephony {
/*
 * DT 单测防 Signal 6（SIGABRT）辅助。
 *
 * 背景：TelEventHandler 的 TelEventQueue 基于 ffrt::queue，事件由 FFRT worker 线程异步执行。
 * 单测中若用例结束时立即析构 handler/状态机（shared_ptr 归零触发 ~TelEventQueue，
 * cancel/wait + 销毁 ffrt::queue），与仍在执行的 FFRT 任务（PopEvent/submit_h）存在生命周期竞态，
 * 会概率性触发进程 abort（DT 上表现为 is_crash=1 / Signal 6，所有用例被判 unavailable）。
 *
 * 用法：用例（或 fixture TearDown /工厂 deleter）在释放 handler/状态机前调用本工具排空：
 *   1. StateMachine 先 Quit()（投递 MSG_STATE_MACHINE_QUIT，停止状态机事件处理）；
 *   2. TelEventHandler RemoveAllEvents() 清空待处理事件；
 *   3. 等待一个排空窗口，让 FFRT worker 把在飞任务执行完毕；
 *   4. 之后再走正常析构。
 */
class TestAsyncUtils {
public:
    static constexpr uint32_t DEFAULT_DRAIN_WAIT_MS = 100;

    static void WaitMs(uint32_t waitMs)
    {
        usleep(waitMs * 1000); // NOLINT
    }

    // 排空单个 TelEventHandler 自身队列（RemoveAllEvents + 等待在飞任务落地）
    static void DrainTelEventHandler(TelEventHandler *handler, uint32_t waitMs = DEFAULT_DRAIN_WAIT_MS)
    {
        if (handler == nullptr) {
            return;
        }
        handler->RemoveAllEvents();
        WaitMs(waitMs);
    }

    // 停机并排空 StateMachine（Quit 异步投递，等待 FFRT 处理完 MSG_STATE_MACHINE_QUIT）
    static void DrainStateMachine(StateMachine *machine, uint32_t waitMs = DEFAULT_DRAIN_WAIT_MS)
    {
        if (machine == nullptr) {
            return;
        }
        machine->Quit();
        WaitMs(waitMs);
    }

    /*
     * 停机并排空 CellularDataHandler 及其持有的 DataConnectionManager 状态机。
     * 注意：connectionManager_ 为 private 成员，仅可在 "#define private public" 生效的
     * 测试编译单元中使用。
     */
    static void DrainCellularDataHandler(CellularDataHandler *handler, uint32_t waitMs = DEFAULT_DRAIN_WAIT_MS)
    {
        if (handler == nullptr) {
            return;
        }
        if (handler->connectionManager_ != nullptr) {
            handler->connectionManager_->Quit();
        }
        handler->RemoveAllEvents();
        WaitMs(waitMs);
    }

    /*
     * 创建带"排空后析构"删除器的 CellularDataHandler，替代测试中的
     * std::make_shared<CellularDataHandler>(slotId)：引用计数归零时先排空 FFRT 队列再 delete，
     * 消除用例结束瞬间的析构竞态。仅可在 "#define private public" 生效的测试编译单元中使用。
     */
    static std::shared_ptr<CellularDataHandler> MakeDrainedHandler(int32_t slotId,
        uint32_t waitMs = DEFAULT_DRAIN_WAIT_MS)
    {
        CellularDataHandler *raw = new (std::nothrow) CellularDataHandler(slotId);
        if (raw == nullptr) {
            return nullptr;
        }
        return std::shared_ptr<CellularDataHandler>(raw, [waitMs](CellularDataHandler *handler) {
            DrainCellularDataHandler(handler, waitMs);
            delete handler;
        });
    }

    /*
     * 泛型：任意 StateMachine 派生类（CellularDataStateMachine / DataConnectionManager /
     * IncallDataStateMachine 等）的"排空后析构"工厂，替代测试中的 std::make_shared<T>(...)。
     */
    template<typename T, typename... Args>
    static std::shared_ptr<T> MakeDrainedStateMachine(Args &&...args)
    {
        T *raw = new (std::nothrow) T(std::forward<Args>(args)...);
        if (raw == nullptr) {
            return nullptr;
        }
        return std::shared_ptr<T>(raw, [](T *machine) {
            DrainStateMachine(machine);
            delete machine;
        });
    }

    /*
     * 泛型：任意 TelEventHandler 派生类（测试辅助 handler 等）的"排空后析构"工厂，
     * 替代测试中的 std::make_shared<T>(...)。
     */
    template<typename T, typename... Args>
    static std::shared_ptr<T> MakeDrainedTelEventHandler(Args &&...args)
    {
        T *raw = new (std::nothrow) T(std::forward<Args>(args)...);
        if (raw == nullptr) {
            return nullptr;
        }
        return std::shared_ptr<T>(raw, [](T *handler) {
            DrainTelEventHandler(handler);
            delete handler;
        });
    }
};
} // namespace Telephony
} // namespace OHOS
#endif // TEST_ASYNC_UTILS_H
