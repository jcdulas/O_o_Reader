#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/base/threading/ThreadPool.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace oor::query
{

// What an operator runs on: the pool, the flag that cancels, the counter that reports progress, and the list of the
// computed columns that were evaluated (for the report of the table).
//
// Every operator cuts its rows into chunks of a size that depends only on the number of rows and on the operator, never
// on the number of threads, and each chunk writes only to its own part of the result: a result is the same for any
// number of threads.
class ExecutionContext
{
public:
    ExecutionContext() = default;
    ExecutionContext(base::ThreadPool* pool, const base::CancellationToken* token, base::ProgressCounter* progress)
        : m_pool(pool), m_token(token), m_progress(progress)
    {
    }

    [[nodiscard]] base::ThreadPool* pool() const noexcept { return m_pool; }
    [[nodiscard]] const base::CancellationToken* token() const noexcept { return m_token; }

    // Runs `body` on chunks of `chunkSize` items of [0, itemCount), spread over the pool and the calling thread. The
    // exceptions of a chunk and a cancellation come out of this call.
    void parallel(std::size_t itemCount, std::size_t chunkSize,
                  const std::function<void(base::IndexRange)>& body) const;

    // Starts a stage for the progress callable (`total` items to do).
    void beginStage(const std::string& stage, std::size_t total) const;
    // Says that the work of a stage is done in part.
    void advance(std::size_t amount) const noexcept
    {
        if (m_progress != nullptr)
            m_progress->add(amount);
    }
    void checkCancelled() const
    {
        if (m_token != nullptr)
            m_token->throwIfCancelled();
    }

    // The computed columns evaluated so far, in order of first evaluation.
    void noteComputed(const std::string& column) { m_computed.push_back(column); }
    [[nodiscard]] const std::vector<std::string>& computed() const noexcept { return m_computed; }

private:
    base::ThreadPool* m_pool = nullptr;
    const base::CancellationToken* m_token = nullptr;
    base::ProgressCounter* m_progress = nullptr;
    std::vector<std::string> m_computed;
};

// The rows per chunk of the operators that do little work per row (a copy, a comparison): large enough that the cost of
// a chunk is not the cost of handing it out, a multiple of 8 so that chunks never share a byte of a bitmap.
inline constexpr std::size_t LIGHT_CHUNK = 32768;
// The rows per chunk of the operators that evaluate an expression (the temporary vectors stay in cache).
inline constexpr std::size_t EXPRESSION_CHUNK = 4096;

} // namespace oor::query
