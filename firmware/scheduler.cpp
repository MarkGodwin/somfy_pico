// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include "picoSomfy.h"
#include "pico/cyw43_arch.h"

#include "scheduler.h"

ScheduledTimer::ScheduledTimer(std::function<uint32_t()> &&callback, uint32_t timeout)
:   _callback(callback)
{

    _worker.next = nullptr;
    _worker.do_work = TimerCallbackEntry;
    _worker.user_data = this;

    if(timeout)
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &_worker, timeout);

}

void ScheduledTimer::ResetTimer(uint32_t timeout)
{
    async_context_remove_at_time_worker(cyw43_arch_async_context(), &_worker);
    if(timeout)
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &_worker, timeout);
}


ScheduledTimer::~ScheduledTimer()
{
    async_context_remove_at_time_worker(cyw43_arch_async_context(), &_worker);
}


void ScheduledTimer::TimerCallbackEntry(async_context_t *context, async_at_time_worker_t *worker) {

    auto pThis = (ScheduledTimer *)worker->user_data;
    pThis->TimerCallback(context);
}

void ScheduledTimer::TimerCallback(async_context_t *context)
{
    auto nextTime = _callback();

    // Restart timer
    if(nextTime)
        async_context_add_at_time_worker_in_ms(context, &_worker, nextTime);
}


PendingWorker::PendingWorker(std::function<void()> &&callback)
:   _callback(callback)
{
    memset(&_worker, 0, sizeof(_worker));
    _worker.next = nullptr;
    _worker.do_work = WorkerCallbackEntry;
    _worker.user_data = this;

    async_context_add_when_pending_worker(cyw43_arch_async_context(), &_worker);
}

PendingWorker::~PendingWorker()
{
    async_context_remove_when_pending_worker(cyw43_arch_async_context(), &_worker);
}

void PendingWorker::ScheduleWork()
{
    async_context_set_work_pending(cyw43_arch_async_context(), &_worker);
}

void PendingWorker::WorkerCallbackEntry(async_context_t *context, async_when_pending_worker_t *worker)
{
    auto pThis = (PendingWorker *)worker->user_data;
    pThis->WorkerCallback(context);

}

void PendingWorker::WorkerCallback(async_context_t *context)
{
    _callback();
}
