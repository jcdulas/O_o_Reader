#include "oor/model/storage/IdIndex.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr std::size_t MINIMUM_SLOTS = 16;

} // namespace

const IdIndex::Storage IdIndex::EMPTY;

IdIndex::Storage& IdIndex::writable()
{
    if (!m_storage)
        m_storage = std::make_shared<Storage>();
    else if (m_storage.use_count() > 1)
        m_storage = std::make_shared<Storage>(*m_storage);
    return *m_storage;
}

void IdIndex::grow(std::size_t minimumSlots)
{
    std::vector<Slot>& table = writable().slots;
    std::size_t slots = std::max(MINIMUM_SLOTS, table.size());
    while (slots < minimumSlots)
        slots *= 2;
    if (slots == table.size())
        return;
    std::vector<Slot> old = std::move(table);
    table.assign(slots, Slot{});
    m_count = 0;
    for (const Slot& slot : old)
    {
        if (slot.used)
            insertHashed(slot.id, slot.location);
    }
}

void IdIndex::reserve(std::size_t entries)
{
    m_reserved = std::max(m_reserved, entries);
    if (m_hashed)
        grow(entries * 2 + 2); // keep the load factor under 0.5
    else
    {
        std::vector<CardLocation>& dense = writable().dense;
        dense.reserve(dense.size() + entries); // right when the IDs to come follow each other
    }
}

void IdIndex::clear() noexcept
{
    m_storage.reset();
    m_denseBase = 0;
    m_count = 0;
    m_reserved = 0;
    m_hashed = false;
}

bool IdIndex::coverDense(base::EntityId id)
{
    std::vector<CardLocation>& dense = writable().dense;
    const std::int64_t wanted = id;
    if (dense.empty())
    {
        m_denseBase = wanted;
        dense.assign(1, NO_LOCATION);
        return true;
    }
    const std::int64_t low = std::min(m_denseBase, wanted);
    const std::int64_t high = std::max(m_denseBase + static_cast<std::int64_t>(dense.size()) - 1, wanted);
    const std::uint64_t span = static_cast<std::uint64_t>(high - low) + 1;
    const std::uint64_t limit = DENSE_SPREAD * (m_count + 1) + DENSE_SLACK;
    if (span > limit)
        return false;
    if (wanted >= m_denseBase)
    {
        dense.resize(static_cast<std::size_t>(wanted - m_denseBase) + 1, NO_LOCATION); // grows by doubling
        return true;
    }
    // Below the array: room for as many IDs again below `id` (IDs read in decreasing order are not copied each time),
    // never below the smallest ID, and never so much that the array alone would be too sparse.
    const std::int64_t room =
        std::min({static_cast<std::int64_t>(dense.size()), wanted - std::numeric_limits<base::EntityId>::min(),
                  static_cast<std::int64_t>(limit - span)});
    const std::int64_t newBase = wanted - room;
    std::vector<CardLocation> moved(static_cast<std::size_t>(m_denseBase - newBase) + dense.size(), NO_LOCATION);
    std::copy(dense.begin(), dense.end(), moved.begin() + (m_denseBase - newBase));
    dense = std::move(moved);
    m_denseBase = newBase;
    return true;
}

void IdIndex::convertToHashed()
{
    Storage& stored = writable();
    const std::vector<CardLocation> dense = std::move(stored.dense);
    stored.dense = {};
    m_hashed = true;
    stored.slots.clear();
    const std::size_t entries = std::max(m_reserved, m_count + 1);
    m_count = 0;
    grow(entries * 2 + 2);
    for (std::size_t offset = 0; offset < dense.size(); ++offset)
    {
        if (dense[offset] != NO_LOCATION)
            insertHashed(static_cast<base::EntityId>(m_denseBase + static_cast<std::int64_t>(offset)), dense[offset]);
    }
}

std::optional<CardLocation> IdIndex::insert(base::EntityId id, CardLocation location)
{
    if (!m_hashed)
    {
        const std::int64_t offset = static_cast<std::int64_t>(id) - m_denseBase;
        const bool inside = offset >= 0 && offset < static_cast<std::int64_t>(storage().dense.size());
        if (inside || coverDense(id))
        {
            std::vector<CardLocation>& dense = writable().dense;
            CardLocation& slot = dense[static_cast<std::size_t>(static_cast<std::int64_t>(id) - m_denseBase)];
            const CardLocation previous = slot;
            slot = location;
            if (previous != NO_LOCATION)
                return previous;
            ++m_count;
            return std::nullopt;
        }
        convertToHashed();
    }
    return insertHashed(id, location);
}

std::optional<CardLocation> IdIndex::insertHashed(base::EntityId id, CardLocation location)
{
    if ((m_count + 1) * 2 > storage().slots.size())
        grow(std::max(MINIMUM_SLOTS, storage().slots.size() * 2));
    std::vector<Slot>& table = writable().slots;
    const std::size_t mask = table.size() - 1;
    std::size_t position = hashOf(id) & mask;
    while (table[position].used)
    {
        if (table[position].id == id)
        {
            const CardLocation previous = table[position].location;
            table[position].location = location;
            return previous;
        }
        position = (position + 1) & mask;
    }
    table[position] = Slot{id, location, true};
    ++m_count;
    return std::nullopt;
}

std::optional<CardLocation> IdIndex::findHashed(base::EntityId id) const noexcept
{
    const std::vector<Slot>& table = storage().slots;
    if (table.empty())
        return std::nullopt;
    const std::size_t mask = table.size() - 1;
    std::size_t position = hashOf(id) & mask;
    while (table[position].used)
    {
        if (table[position].id == id)
            return table[position].location;
        position = (position + 1) & mask;
    }
    return std::nullopt;
}

bool IdIndex::erase(base::EntityId id)
{
    if (!m_hashed)
    {
        const std::vector<CardLocation>& dense = storage().dense;
        const std::int64_t offset = static_cast<std::int64_t>(id) - m_denseBase;
        if (offset < 0 || offset >= static_cast<std::int64_t>(dense.size()) ||
            dense[static_cast<std::size_t>(offset)] == NO_LOCATION)
            return false;
        writable().dense[static_cast<std::size_t>(offset)] = NO_LOCATION;
        --m_count;
        return true;
    }
    const std::vector<Slot>& found = storage().slots;
    if (found.empty())
        return false;
    const std::size_t foundMask = found.size() - 1;
    std::size_t probe = hashOf(id) & foundMask;
    while (found[probe].used && found[probe].id != id)
        probe = (probe + 1) & foundMask;
    if (!found[probe].used)
        return false; // nothing to remove: a shared table stays shared
    std::vector<Slot>& table = writable().slots;
    const std::size_t mask = table.size() - 1;
    const std::size_t position = probe;
    // Backward shift deletion: move later entries of the probe sequence back so that lookups stay correct.
    std::size_t hole = position;
    std::size_t next = (hole + 1) & mask;
    while (table[next].used)
    {
        const std::size_t home = hashOf(table[next].id) & mask;
        const bool canMove = ((next - home) & mask) >= ((next - hole) & mask);
        if (canMove)
        {
            table[hole] = table[next];
            hole = next;
        }
        next = (next + 1) & mask;
    }
    table[hole] = Slot{};
    --m_count;
    return true;
}

} // namespace oor::model
