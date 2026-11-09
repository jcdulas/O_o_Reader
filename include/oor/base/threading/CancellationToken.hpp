#pragma once

#include "oor/base/errors/Errors.hpp"

#include <atomic>

namespace oor::base
{

// A flag that long operations check, so that they can be interrupted (Ctrl-C, a progress callback returning false).
// The worker threads read it at the end of each chunk; the calling thread sets it. Not copyable: share by reference.
class CancellationToken
{
public:
    CancellationToken() = default;
    CancellationToken(const CancellationToken&) = delete;
    CancellationToken& operator=(const CancellationToken&) = delete;

    void requestCancellation() noexcept { m_cancelled.store(true, std::memory_order_relaxed); }
    [[nodiscard]] bool isCancelled() const noexcept { return m_cancelled.load(std::memory_order_relaxed); }
    // Throws OperationCancelled when cancellation was requested.
    void throwIfCancelled() const
    {
        if (isCancelled())
            throw OperationCancelled();
    }

private:
    std::atomic<bool> m_cancelled{false};
};

} // namespace oor::base
