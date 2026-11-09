#pragma once

#include "oor/analysis/mass/SectionFormulas.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace oor::analysis
{

// The library sections computed so far, by type, dimensions and rule: a caller that asks for the sections of many
// properties (Container.sections(), sections_table) solves each distinct section once, whatever the number of
// properties that share it. A section is a function of its key alone (SectionFormulas::standard), so a value taken from
// the cache has the bits of one computed afresh, and a cache may serve any model: the cache a model keeps
// (ComponentCache::sections) lives across its edits and is shared with its copies, so that the sections of a sized
// copy whose bars did not change are not solved again. The dimensions are compared by their bits (-0.0 is not 0.0, a
// NaN is a key like another).
//
// Safe to share between threads: a section asked for by several threads at once is computed by one of them, the
// others waiting for it. A batch of requests (the sections of one query) starts with nextBatch(); trim() then drops the
// sections the batch did not use when the cache holds more than MAX_SECTIONS, so that a cache that lives with a model
// keeps at least the sections of its last batch and does not grow without end.
class SectionCache
{
public:
    // The most sections kept beyond those of the last batch (some 2 MB).
    static constexpr std::size_t MAX_SECTIONS = 4096;

    // What the cache has done since it was made, for the tests and the performance notes.
    struct Statistics
    {
        std::uint64_t computed = 0; // sections solved (SectionFormulas::standard)
        std::uint64_t reused = 0;   // requests answered by a section already computed
        std::uint64_t dropped = 0;  // sections dropped by trim()
    };

    // SectionFormulas::standard, computed on the first call for these arguments.
    [[nodiscard]] SectionProperties standard(std::string_view type, std::span<const double> dimensions,
                                             ShearFactorRule rule);

    // Starts a batch of requests: the sections asked for from now on are those of the batch.
    void nextBatch();
    // Drops the sections the current batch did not ask for, when the cache holds more than MAX_SECTIONS.
    void trim();

    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] Statistics statistics() const;

private:
    using Key = std::tuple<std::string, std::vector<std::uint64_t>, ShearFactorRule>;
    struct Entry
    {
        std::once_flag computed;
        SectionProperties section;
        std::uint64_t batch = 0; // the last batch that asked for it
    };

    mutable std::mutex m_mutex;
    std::map<Key, std::shared_ptr<Entry>> m_sections;
    std::uint64_t m_batch = 0;
    Statistics m_statistics;
};

} // namespace oor::analysis
