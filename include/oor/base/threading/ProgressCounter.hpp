#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace oor::base
{

// Progress of a long operation: the stage, the amount done and the total. Workers add to `done` once per chunk (one
// relaxed atomic add, so no cost worth measuring); the calling thread reads a snapshot to call the Python
// `progress=` callable, at most ten times per second.
class ProgressCounter
{
public:
    struct Snapshot
    {
        std::string stage;
        std::uint64_t done = 0;
        std::uint64_t total = 0;
    };

    // Starts a stage; `total` is 0 when unknown. Called by the thread that runs the operation.
    void beginStage(std::string stage, std::uint64_t total)
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_stage = std::move(stage);
        m_total.store(total, std::memory_order_relaxed);
        m_done.store(0, std::memory_order_relaxed);
    }
    void add(std::uint64_t amount) noexcept { m_done.fetch_add(amount, std::memory_order_relaxed); }
    // The total grows when more work is found (an INCLUDE file).
    void addTotal(std::uint64_t amount) noexcept { m_total.fetch_add(amount, std::memory_order_relaxed); }
    // Read by the thread that watches the operation.
    [[nodiscard]] Snapshot snapshot() const
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        return Snapshot{m_stage, m_done.load(std::memory_order_relaxed), m_total.load(std::memory_order_relaxed)};
    }

private:
    mutable std::mutex m_mutex;
    std::string m_stage;
    std::atomic<std::uint64_t> m_done{0};
    std::atomic<std::uint64_t> m_total{0};
};

} // namespace oor::base
