#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/base/threading/ThreadPool.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <numeric>
#include <stdexcept>
#include <vector>

using namespace oor::base;

namespace
{

// Sum of squares of each chunk of [0, itemCount), computed with `workerCount` workers.
std::vector<unsigned long long> chunkSums(std::size_t workerCount, std::size_t itemCount)
{
    ThreadPool pool(workerCount);
    const std::vector<IndexRange> chunks = splitIntoChunks(itemCount, 1000);
    std::vector<unsigned long long> sums(chunks.size(), 0);
    parallelFor(pool, chunks.size(),
                [&](std::size_t chunk)
                {
                    unsigned long long sum = 0;
                    for (std::size_t item = chunks[chunk].begin; item < chunks[chunk].end; ++item)
                        sum += static_cast<unsigned long long>(item) * item;
                    sums[chunk] = sum;
                });
    return sums;
}

} // namespace

TEST_CASE("chunks cover the items once, whatever the target size", "[base][threading]")
{
    for (std::size_t target : {1u, 7u, 1000u, 5000u})
    {
        const auto chunks = splitIntoChunks(4321, target);
        std::size_t expectedBegin = 0;
        for (const IndexRange& chunk : chunks)
        {
            REQUIRE(chunk.begin == expectedBegin);
            REQUIRE(chunk.end > chunk.begin);
            expectedBegin = chunk.end;
        }
        REQUIRE(expectedBegin == 4321);
    }
    REQUIRE(splitIntoChunks(0, 10).empty());
}

TEST_CASE("the result is the same for 0, 1, 2 and 11 workers", "[base][threading]")
{
    const auto reference = chunkSums(0, 50000);
    REQUIRE(reference.size() == 50);
    for (std::size_t workers : {1u, 2u, 11u})
        REQUIRE(chunkSums(workers, 50000) == reference);
}

TEST_CASE("every chunk runs exactly once", "[base][threading]")
{
    ThreadPool pool(4);
    std::vector<std::atomic<int>> counts(200);
    parallelFor(pool, counts.size(), [&](std::size_t chunk) { counts[chunk].fetch_add(1); });
    for (const auto& count : counts)
        REQUIRE(count.load() == 1);
}

TEST_CASE("an exception in a chunk is rethrown to the caller", "[base][threading]")
{
    ThreadPool pool(3);
    REQUIRE_THROWS_AS(parallelFor(pool, 100,
                                  [](std::size_t chunk)
                                  {
                                      if (chunk == 40)
                                          throw std::runtime_error("boom");
                                  }),
                      std::runtime_error);
}

TEST_CASE("cancellation stops the work within a chunk", "[base][threading]")
{
    ThreadPool pool(3);
    CancellationToken token;
    std::atomic<int> started{0};
    REQUIRE_THROWS_AS(parallelFor(
                          pool, 100000,
                          [&](std::size_t)
                          {
                              if (started.fetch_add(1) == 50)
                                  token.requestCancellation();
                          },
                          &token),
                      OperationCancelled);
    REQUIRE(started.load() < 100000);
    REQUIRE(token.isCancelled());
    REQUIRE_THROWS_AS(token.throwIfCancelled(), OperationCancelled);
}

TEST_CASE("an empty loop returns at once", "[base][threading]")
{
    ThreadPool pool(2);
    parallelFor(pool, 0, [](std::size_t) { FAIL("must not be called"); });
}

TEST_CASE("progress counts what workers add", "[base][threading]")
{
    ProgressCounter progress;
    progress.beginStage("lexing", 1000);
    ThreadPool pool(4);
    parallelFor(pool, 10, [&](std::size_t) { progress.add(100); });
    const auto snapshot = progress.snapshot();
    REQUIRE(snapshot.stage == "lexing");
    REQUIRE(snapshot.done == 1000);
    REQUIRE(snapshot.total == 1000);
}

TEST_CASE("the default worker count is at least one", "[base][threading]")
{
    REQUIRE(ThreadPool::defaultWorkerCount() >= 1);
}
