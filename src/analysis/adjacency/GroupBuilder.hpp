#pragma once

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/core/Model.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

// The machinery the node, edge and face indexes are built with: the (key, element) entries of every element are made in
// chunks of rows over the thread pool, sorted in parallel, and cut into groups of equal keys. The entries are sorted by
// key and then by element, a total order, so the result is the same for any number of threads.

namespace oor::analysis::detail
{

template <typename Key>
struct Entry
{
    Key key;
    std::uint64_t ref; // the element packed as (card type << 32) | row
};

[[nodiscard]] constexpr std::uint64_t packRef(std::uint16_t type, std::uint32_t row) noexcept
{
    return (static_cast<std::uint64_t>(type) << 32U) | row;
}

template <typename Key>
[[nodiscard]] bool entryLess(const Entry<Key>& left, const Entry<Key>& right) noexcept
{
    if (left.key < right.key)
        return true;
    if (right.key < left.key)
        return false;
    return left.ref < right.ref;
}

// Sorts `values` with `less`: the pieces of a power of two number of pieces are sorted on the pool, then merged two by
// two.
template <typename T, typename Less>
void parallelSort(std::vector<T>& values, base::ThreadPool* pool, const Less& less)
{
    constexpr std::size_t PARALLEL_COUNT = 1U << 16U;
    constexpr std::size_t MAX_PIECES = 64;
    const std::size_t count = values.size();
    const std::size_t workers = pool != nullptr ? pool->workerCount() + 1 : 1;
    if (workers <= 1 || count < PARALLEL_COUNT)
    {
        std::sort(values.begin(), values.end(), less);
        return;
    }
    std::size_t pieces = 1;
    while (pieces < workers && pieces < MAX_PIECES)
        pieces *= 2;
    const std::size_t pieceSize = (count + pieces - 1) / pieces;
    base::parallelFor(*pool, pieces,
                      [&](std::size_t piece)
                      {
                          const std::size_t begin = std::min(piece * pieceSize, count);
                          const std::size_t end = std::min(begin + pieceSize, count);
                          std::sort(values.begin() + static_cast<std::ptrdiff_t>(begin),
                                    values.begin() + static_cast<std::ptrdiff_t>(end), less);
                      });
    std::vector<T> buffer(count);
    std::vector<T>* source = &values;
    std::vector<T>* target = &buffer;
    for (std::size_t width = pieceSize; width < count; width *= 2)
    {
        const std::size_t groups = (count + 2 * width - 1) / (2 * width);
        base::parallelFor(*pool, groups,
                          [&](std::size_t group)
                          {
                              const std::size_t begin = group * 2 * width;
                              const std::size_t middle = std::min(begin + width, count);
                              const std::size_t end = std::min(begin + 2 * width, count);
                              const auto from = source->begin();
                              std::merge(
                                  from + static_cast<std::ptrdiff_t>(begin), from + static_cast<std::ptrdiff_t>(middle),
                                  from + static_cast<std::ptrdiff_t>(middle), from + static_cast<std::ptrdiff_t>(end),
                                  target->begin() + static_cast<std::ptrdiff_t>(begin), less);
                          });
        std::swap(source, target);
    }
    if (source != &values)
        values.swap(*source);
}

// A run of rows of one element table.
struct RowChunk
{
    std::uint16_t type = 0;
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
};

// The rows of every element table whose card type `wanted` accepts, cut into chunks.
template <typename Wanted>
[[nodiscard]] std::vector<RowChunk> elementChunks(const model::Model& model, const Wanted& wanted,
                                                  std::size_t& totalRows)
{
    constexpr std::uint32_t ROWS_PER_CHUNK = 4096;
    std::vector<RowChunk> chunks;
    totalRows = 0;
    for (std::size_t type = 1; type < model::Model::typeCount(); ++type)
    {
        const auto typeIndex = static_cast<std::uint16_t>(type);
        const model::CardTable* table = model.findTable(typeIndex);
        if (table == nullptr || !wanted(typeIndex))
            continue;
        const auto rows = static_cast<std::uint32_t>(table->rows().rowCount());
        totalRows += rows;
        for (std::uint32_t begin = 0; begin < rows; begin += ROWS_PER_CHUNK)
            chunks.push_back(RowChunk{typeIndex, begin, std::min(begin + ROWS_PER_CHUNK, rows)});
    }
    return chunks;
}

// The entries of every live element of the card types `wanted` accepts: `emit(table, type, row, entries)` appends the
// entries of one element.
template <typename Key, typename Wanted, typename Emit>
[[nodiscard]] std::vector<Entry<Key>> collectEntries(const model::Model& model, base::ThreadPool* pool,
                                                     const Wanted& wanted, const Emit& emit)
{
    constexpr std::size_t PARALLEL_ROWS = 8192;
    std::size_t totalRows = 0;
    const std::vector<RowChunk> chunks = elementChunks(model, wanted, totalRows);
    std::vector<std::vector<Entry<Key>>> parts(chunks.size());
    const auto work = [&](std::size_t index)
    {
        const RowChunk& chunk = chunks[index];
        const model::CardTable& table = *model.findTable(chunk.type);
        std::vector<Entry<Key>>& out = parts[index];
        out.reserve(static_cast<std::size_t>(chunk.end - chunk.begin) * 4);
        for (std::uint32_t row = chunk.begin; row < chunk.end; ++row)
        {
            if (table.rows().isAlive(row))
                emit(table, chunk.type, row, out);
        }
    };
    if (pool != nullptr && pool->workerCount() > 0 && totalRows >= PARALLEL_ROWS)
        base::parallelFor(*pool, chunks.size(), work);
    else
    {
        for (std::size_t index = 0; index < chunks.size(); ++index)
            work(index);
    }
    std::size_t total = 0;
    for (const auto& part : parts)
        total += part.size();
    std::vector<Entry<Key>> entries;
    entries.reserve(total);
    for (auto& part : parts)
    {
        entries.insert(entries.end(), part.begin(), part.end());
        std::vector<Entry<Key>>().swap(part);
    }
    return entries;
}

} // namespace oor::analysis::detail
