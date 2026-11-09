#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace oor::model
{

// Where a card is: which table (index in the card registry) and which row.
struct CardLocation
{
    std::uint16_t table = 0;
    std::uint32_t row = 0;
    [[nodiscard]] bool operator==(const CardLocation&) const noexcept = default;
};

// Nastran ID -> location, for one ID space (all elements, all properties, ...).
//
// Two forms. While the IDs are dense (they span at most DENSE_SPREAD times their number, plus DENSE_SLACK), the index
// is an array of locations indexed by the ID minus the lowest one: a lookup is one bounds check and one load, and IDs
// that follow each other are neighbours in memory. When an ID would make the array too sparse, the index turns for
// good (until clear()) into a hash table with open addressing and linear probing: a lookup is a hash and usually one
// probe; removal uses backward shifting, so no tombstones accumulate however many edits are made.
//
// Copy on write: the array and the hash table are shared by the copies of a model until one of them changes its
// index, which clones them first (as the buffers of a Column are): a copy of a model does no work per ID.
//
// PERFORMANCE: reading the plate of 8 M CQUAD4 and 8 M GRID spent 40 % of the thread that builds the cards in the
// inserts of the hash table (a cache miss each: the IDs are spread over 256 MB per index). Measured: 16 M inserts of
// IDs that follow each other 51 ns each, now 7 ns; a lookup 20 ns, now 0.6 ns; the read of the plate 5.7 to 5.9 s, now
// 3.6 to 3.7 s, and its model 1.02 GB, now 0.64 GB.
class IdIndex
{
public:
    // The array form holds at most this many slots per ID it holds, plus DENSE_SLACK (8 bytes a slot, against 32 to 64
    // bytes an ID in the hash table).
    static constexpr std::size_t DENSE_SPREAD = 4;
    static constexpr std::size_t DENSE_SLACK = 1024;

    [[nodiscard]] std::size_t size() const noexcept { return m_count; }

    // Inserts or replaces. Returns the previous location when the ID was already there.
    std::optional<CardLocation> insert(base::EntityId id, CardLocation location);
    [[nodiscard]] std::optional<CardLocation> find(base::EntityId id) const noexcept
    {
        if (m_hashed)
            return findHashed(id);
        const std::vector<CardLocation>& dense = storage().dense;
        const std::int64_t offset = static_cast<std::int64_t>(id) - m_denseBase;
        if (offset < 0 || offset >= static_cast<std::int64_t>(dense.size()))
            return std::nullopt;
        const CardLocation location = dense[static_cast<std::size_t>(offset)];
        if (location == NO_LOCATION)
            return std::nullopt;
        return location;
    }
    // Removes the ID; true if it was there.
    bool erase(base::EntityId id);

    void clear() noexcept;
    void reserve(std::size_t entries);

    // True while the index is in its array form (for the tests).
    [[nodiscard]] bool isDense() const noexcept { return !m_hashed; }

    // Calls `visit(id, location)` for every entry, in no particular order.
    template <typename Visitor>
    void forEach(Visitor&& visit) const
    {
        const Storage& stored = storage();
        if (!m_hashed)
        {
            for (std::size_t offset = 0; offset < stored.dense.size(); ++offset)
            {
                if (stored.dense[offset] != NO_LOCATION)
                    visit(static_cast<base::EntityId>(m_denseBase + static_cast<std::int64_t>(offset)),
                          stored.dense[offset]);
            }
            return;
        }
        for (const Slot& slot : stored.slots)
        {
            if (slot.used)
                visit(slot.id, slot.location);
        }
    }

private:
    struct Slot
    {
        base::EntityId id = 0;
        CardLocation location;
        bool used = false;
    };

    // An empty slot of the array form (no table has this index).
    static constexpr CardLocation NO_LOCATION{std::numeric_limits<std::uint16_t>::max(),
                                              std::numeric_limits<std::uint32_t>::max()};

    [[nodiscard]] static std::size_t hashOf(base::EntityId id) noexcept
    {
        // Murmur-style integer finaliser: sequential IDs spread over the table.
        std::uint32_t value = static_cast<std::uint32_t>(id);
        value ^= value >> 16;
        value *= 0x7feb352dU;
        value ^= value >> 15;
        value *= 0x846ca68bU;
        value ^= value >> 16;
        return value;
    }

    [[nodiscard]] std::optional<CardLocation> findHashed(base::EntityId id) const noexcept;
    std::optional<CardLocation> insertHashed(base::EntityId id, CardLocation location);
    // Makes the array cover `id`; false when it would then be too sparse.
    bool coverDense(base::EntityId id);
    void convertToHashed();
    void grow(std::size_t minimumSlots);

    struct Storage
    {
        std::vector<CardLocation> dense; // the array form: the location of ID m_denseBase + i, or NO_LOCATION
        std::vector<Slot> slots;         // the hash table: size is a power of two or zero
    };

    // What the index holds (an empty storage while it holds nothing), and the same made private to this index: cloned
    // first when a copy shares it.
    [[nodiscard]] const Storage& storage() const noexcept { return m_storage ? *m_storage : EMPTY; }
    Storage& writable();

    static const Storage EMPTY;

    std::shared_ptr<Storage> m_storage;
    std::int64_t m_denseBase = 0;
    std::size_t m_count = 0;
    std::size_t m_reserved = 0; // the largest reserve() asked, for the hash table when the array turns into one
    bool m_hashed = false;
};

} // namespace oor::model
