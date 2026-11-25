// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "platform/MainThread.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <vector>

#include <Windows.h>

#include "ll/api/memory/Hook.h"

#include "mc/client/game/MinecraftGame.h"

namespace jade::platform {

namespace {

std::mutex                        gMutex;
std::deque<std::function<void()>> gQueue;
std::vector<MainThread::Tick>     gTicks;
std::atomic<bool>                 gRunning{false};
std::atomic<std::uint64_t>        gFrames{0};
std::atomic<DWORD>                gThreadId{0};

}

LL_TYPE_INSTANCE_HOOK(JadeFrameHook, ll::memory::HookPriority::Normal, MinecraftGame, &MinecraftGame::$update, void) {
    origin();
    MainThread::get().onFrame();
}

MainThread& MainThread::get() {
    static MainThread instance;
    return instance;
}

bool MainThread::start() {
    gRunning = true;
    return JadeFrameHook::hook() == 0;
}

void MainThread::stop() {
    gRunning = false;
    JadeFrameHook::unhook();
    std::lock_guard lock(gMutex);
    gQueue.clear();
    gTicks.clear();
}

void MainThread::addTick(Tick tick) {
    std::lock_guard lock(gMutex);
    gTicks.push_back(std::move(tick));
}

void MainThread::post(std::function<void()> fn) {
    if (!gRunning) return;
    std::lock_guard lock(gMutex);
    gQueue.push_back(std::move(fn));
}

std::uint64_t MainThread::frames() const { return gFrames.load(); }
bool          MainThread::isMainThread() const { return ::GetCurrentThreadId() == gThreadId.load(); }

void MainThread::onFrame() {
    if (!gRunning) return;
    gThreadId  = ::GetCurrentThreadId();
    auto frame = ++gFrames;

    std::vector<Tick>                 ticks;
    std::deque<std::function<void()>> queue;
    {
        std::lock_guard lock(gMutex);
        ticks = gTicks;
        queue.swap(gQueue);
    }
    for (auto& tick : ticks) tick(frame);
    for (auto& fn : queue) fn();
}

}
