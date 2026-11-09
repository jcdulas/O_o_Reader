#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>

namespace oor::base
{

std::vector<IndexRange> splitIntoChunks(std::size_t itemCount, std::size_t targetChunkSize)
{
    std::vector<IndexRange> chunks;
    if (itemCount == 0)
        return chunks;
    const std::size_t chunkSize = std::max<std::size_t>(1, targetChunkSize);
    chunks.reserve(itemCount / chunkSize + 1);
    for (std::size_t begin = 0; begin < itemCount; begin += chunkSize)
        chunks.push_back(IndexRange{begin, std::min(itemCount, begin + chunkSize)});
    return chunks;
}

namespace
{

// State shared between the calling thread and the helper tasks. Held by shared_ptr so that a helper that starts after
// parallelFor returned never touches a destroyed object.
//
// The calling thread takes chunks too, and when none is left it closes the loop: it waits only for the helpers that
// came in before, and a helper that starts later (its pool thread was busy, or not yet scheduled on a loaded machine)
// leaves at once, without the body. A short loop thus never waits for the pool to wake up.
struct ParallelState
{
    std::atomic<std::size_t> nextChunk{0};
    std::atomic<bool> failed{false};
    std::mutex mutex;
    std::condition_variable finished;
    std::size_t helpersInside = 0; // the helpers running chunks now
    bool closed = false;           // the calling thread is done: the helpers that come now leave
    std::exception_ptr firstError;
};

void runChunks(ParallelState& state, std::size_t chunkCount, const std::function<void(std::size_t)>& body,
               const CancellationToken* token)
{
    while (!state.failed.load(std::memory_order_relaxed) && !(token != nullptr && token->isCancelled()))
    {
        const std::size_t chunk = state.nextChunk.fetch_add(1, std::memory_order_relaxed);
        if (chunk >= chunkCount)
            return;
        try
        {
            body(chunk);
        }
        catch (...)
        {
            const std::lock_guard<std::mutex> lock(state.mutex);
            if (!state.firstError)
                state.firstError = std::current_exception();
            state.failed.store(true, std::memory_order_relaxed);
        }
    }
}

} // namespace

void parallelFor(ThreadPool& pool, std::size_t chunkCount, const std::function<void(std::size_t)>& body,
                 const CancellationToken* token)
{
    if (chunkCount == 0)
        return;
    auto state = std::make_shared<ParallelState>();
    const std::size_t helpers = std::min(pool.workerCount(), chunkCount - 1);
    for (std::size_t helper = 0; helper < helpers; ++helper)
    {
        // The body and the token outlive the helpers that come in before the loop is closed: parallelFor waits for
        // them below. A helper that comes later uses neither.
        try
        {
            pool.submit(
                [state, chunkCount, &body, token]
                {
                    {
                        const std::lock_guard<std::mutex> lock(state->mutex);
                        if (state->closed)
                            return;
                        ++state->helpersInside;
                    }
                    runChunks(*state, chunkCount, body, token);
                    const std::lock_guard<std::mutex> lock(state->mutex);
                    if (--state->helpersInside == 0)
                        state->finished.notify_all();
                });
        }
        catch (...)
        {
            // No memory for another task: fewer helpers, the calling thread runs their chunks. Leaving here would
            // let the helpers already submitted run the body after it is gone.
            break;
        }
    }
    runChunks(*state, chunkCount, body, token);
    {
        std::unique_lock<std::mutex> lock(state->mutex);
        state->closed = true;
        state->finished.wait(lock, [&state] { return state->helpersInside == 0; });
    }
    if (state->firstError)
        std::rethrow_exception(state->firstError);
    if (token != nullptr)
        token->throwIfCancelled();
}

} // namespace oor::base
