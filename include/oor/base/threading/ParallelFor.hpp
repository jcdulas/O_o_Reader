#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ThreadPool.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace oor::base
{

// A half-open range of item indices [begin, end).
struct IndexRange
{
    std::size_t begin = 0;
    std::size_t end = 0;
};

// Cuts [0, itemCount) into consecutive chunks of about `targetChunkSize` items. The cut depends only on the two
// arguments, never on the number of threads, so results assembled per chunk are identical for any thread count.
[[nodiscard]] std::vector<IndexRange> splitIntoChunks(std::size_t itemCount, std::size_t targetChunkSize);

// Calls body(chunkIndex) for every chunk index in [0, chunkCount), spread over the pool and the calling thread.
// Each chunk must write only to its own slot of the result (typically results[chunkIndex]); then the merged result
// is the same for 0, 1 or 100 workers.
//
// Exceptions: the first exception thrown by a chunk is rethrown here after all running chunks have finished.
// Cancellation: when `token` is cancelled no new chunk starts, and OperationCancelled is thrown at the end.
void parallelFor(ThreadPool& pool, std::size_t chunkCount, const std::function<void(std::size_t)>& body,
                 const CancellationToken* token = nullptr);

} // namespace oor::base
