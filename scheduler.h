#pragma once

#include "pico/async_context.h"
#include <functional>

/// @brief Trivial wrapper around async_at_time_worker API
/// @remarks We assume the callbacks are in a somewhat safe thread context, but the documentation doesn't make this clear
class ScheduledTimer
{
    public:

        ScheduledTimer(std::function<uint32_t()> &&callback, uint32_t timeout);
        void ResetTimer(uint32_t timeout);

        ~ScheduledTimer();
    
    private:
        std::function<uint32_t()> _callback;

        async_at_time_worker_t _worker;
        static void TimerCallbackEntry(async_context_t *context, async_at_time_worker_t *worker);
        void TimerCallback(async_context_t *context);

};

/// @brief Trivial wrapper around async_when_pending_worker API
/// @remarks We assume the callbacks are somewhat safe thread context, but the documentation doesn't make this clear
class PendingWorker
{
    public:
        PendingWorker(std::function<void()> &&callback);
        ~PendingWorker();

        void ScheduleWork();

    private:
        std::function<void()> _callback;
        async_when_pending_worker_t _worker;
        static void WorkerCallbackEntry(async_context_t *context, async_when_pending_worker_t *worker);
        void WorkerCallback(async_context_t *context);
};
