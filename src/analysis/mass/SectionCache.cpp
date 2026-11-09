#include "oor/analysis/mass/SectionCache.hpp"

#include <bit>

namespace oor::analysis
{

SectionProperties SectionCache::standard(std::string_view type, std::span<const double> dimensions,
                                         ShearFactorRule rule)
{
    std::vector<std::uint64_t> bits;
    bits.reserve(dimensions.size());
    for (const double dimension : dimensions)
        bits.push_back(std::bit_cast<std::uint64_t>(dimension));
    Key key{std::string(type), std::move(bits), rule};
    // The entry is held while it is computed: trim() may drop it from the map meanwhile (the next request computes it
    // again), never from under this call.
    std::shared_ptr<Entry> entry;
    {
        const std::scoped_lock lock(m_mutex);
        auto [found, inserted] = m_sections.try_emplace(std::move(key));
        if (inserted)
            found->second = std::make_shared<Entry>();
        entry = found->second;
        entry->batch = m_batch;
        if (inserted)
            ++m_statistics.computed;
        else
            ++m_statistics.reused;
    }
    // Computed outside the lock, once: a thread that asks for the section while another computes it waits for it.
    std::call_once(entry->computed, [&] { entry->section = SectionFormulas::standard(type, dimensions, rule); });
    return entry->section;
}

void SectionCache::nextBatch()
{
    const std::scoped_lock lock(m_mutex);
    ++m_batch;
}

void SectionCache::trim()
{
    const std::scoped_lock lock(m_mutex);
    if (m_sections.size() <= MAX_SECTIONS)
        return;
    for (auto entry = m_sections.begin(); entry != m_sections.end();)
    {
        if (entry->second->batch != m_batch)
        {
            entry = m_sections.erase(entry);
            ++m_statistics.dropped;
        }
        else
        {
            ++entry;
        }
    }
}

std::size_t SectionCache::size() const
{
    const std::scoped_lock lock(m_mutex);
    return m_sections.size();
}

SectionCache::Statistics SectionCache::statistics() const
{
    const std::scoped_lock lock(m_mutex);
    return m_statistics;
}

} // namespace oor::analysis
