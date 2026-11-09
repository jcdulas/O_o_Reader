#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace oor::base
{

// A fixed set of worker threads fed from a queue. Tasks must not throw (parallelFor catches for them). A pool with
// zero workers is valid: parallelFor then runs everything on the calling thread, which is how "one thread" runs.
class ThreadPool
{
public:
    explicit ThreadPool(std::size_t workerCount);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    [[nodiscard]] std::size_t workerCount() const noexcept { return m_workers.size(); }
    void submit(std::function<void()> task);

    // Logical cores of the machine, at least 1.
    [[nodiscard]] static std::size_t defaultWorkerCount() noexcept;

private:
    void workerLoop(const std::stop_token& stopToken);

    std::mutex m_mutex;
    std::condition_variable_any m_wake;
    std::queue<std::function<void()>> m_tasks;
    std::vector<std::jthread> m_workers;
};

} // namespace oor::base
