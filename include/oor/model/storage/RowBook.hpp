#pragma once

#include "oor/base/ids/CardKey.hpp"
#include "oor/model/storage/Column.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oor::model
{

// The rows of one card table: which are alive, their keys, structural epoch and numeric source precision.
//
// Card keys increase with the order cards were created (in a file, that is file order), so the rows of a table are
// sorted by key, and the key of a row is found by binary search: no key index is stored. Deleting a card leaves a
// tombstone (the row stays, marked dead) so that other rows keep their place; compact() drops the dead rows.
//
// The epoch changes whenever rows are added, deleted or moved. A handle remembers (row, epoch) and trusts the row as
// long as the epoch is the same: one integer comparison per access instead of a search.
//
// Numeric source precision records the significant digits of each parsed real in internal slot order, with 0 for a
// blank or non-real slot. The companion solver uses it when interpreting precision-sensitive input. Records are
// indexed by card key and stored only when digits are available; edits discard the previous source precision.
class RowBook
{
public:
    [[nodiscard]] std::size_t rowCount() const noexcept { return m_keys.size(); }
    [[nodiscard]] std::size_t aliveCount() const noexcept { return m_aliveCount; }
    [[nodiscard]] std::uint64_t epoch() const noexcept { return m_epoch; }

    [[nodiscard]] bool isAlive(std::size_t row) const noexcept { return m_alive.get(row) != 0; }
    [[nodiscard]] base::CardKey keyAt(std::size_t row) const noexcept { return base::CardKey{m_keys.get(row)}; }

    // Appends a live row with this key (keys must increase). Returns the row.
    std::uint32_t append(base::CardKey key)
    {
        m_keys.push(key.value);
        m_alive.push(1);
        ++m_aliveCount;
        ++m_epoch;
        return static_cast<std::uint32_t>(m_keys.size() - 1);
    }

    void kill(std::size_t row)
    {
        if (m_alive.get(row) != 0)
        {
            m_alive.set(row, 0);
            --m_aliveCount;
            ++m_epoch;
        }
    }

    // The row of a card key, or nothing when no row (alive or dead) has it: after compaction a deleted card is gone.
    [[nodiscard]] std::int64_t findRow(base::CardKey key) const noexcept
    {
        std::size_t low = 0;
        std::size_t high = m_keys.size();
        while (low < high)
        {
            const std::size_t middle = low + (high - low) / 2;
            if (m_keys.get(middle) < key.value)
                low = middle + 1;
            else
                high = middle;
        }
        if (low < m_keys.size() && m_keys.get(low) == key.value)
            return static_cast<std::int64_t>(low);
        return -1;
    }

    // The first row (alive or dead) whose key is >= `key`; rowCount() when there is none.
    [[nodiscard]] std::size_t lowerBound(std::uint32_t key) const noexcept
    {
        std::size_t low = 0;
        std::size_t high = m_keys.size();
        while (low < high)
        {
            const std::size_t middle = low + (high - low) / 2;
            if (m_keys.get(middle) < key)
                low = middle + 1;
            else
                high = middle;
        }
        return low;
    }

    // The live row of a card key, or -1 when the card is dead or gone.
    [[nodiscard]] std::int64_t findLiveRow(base::CardKey key) const noexcept
    {
        const std::int64_t row = findRow(key);
        return row >= 0 && isAlive(static_cast<std::size_t>(row)) ? row : -1;
    }

    // The rows that survive compaction, in order. The table applies the same list to each of its columns with
    // keepRows(), then calls applyCompaction().
    [[nodiscard]] std::vector<std::uint32_t> liveRows() const
    {
        std::vector<std::uint32_t> rows;
        rows.reserve(m_aliveCount);
        for (std::size_t row = 0; row < m_alive.size(); ++row)
        {
            if (m_alive.get(row) != 0)
                rows.push_back(static_cast<std::uint32_t>(row));
        }
        return rows;
    }

    void applyCompaction(const std::vector<std::uint32_t>& liveRows)
    {
        m_keys.keepRows(liveRows);
        m_alive = Column<std::uint8_t>(1);
        m_alive.resize(liveRows.size());
        m_aliveCount = liveRows.size();
        ++m_epoch;
        // The digit records of the cards that are gone are dropped with them (a key is never given again).
        const auto isGone = [this](const DigitRecord& record) { return findRow(base::CardKey{record.key}) < 0; };
        if (m_digits && std::any_of(m_digits->records.begin(), m_digits->records.end(), isGone))
            std::erase_if(writableDigits().records, isGone);
    }

    // Appends the rows of `other` (partial table read in parallel); its keys must all be greater than ours.
    void append(RowBook& other)
    {
        m_keys.append(other.m_keys);
        m_alive.append(other.m_alive);
        m_aliveCount += other.m_aliveCount;
        if (other.m_digits)
        {
            for (const DigitRecord& record : other.m_digits->records)
            {
                const auto digits =
                    std::span<const std::uint8_t>(other.m_digits->bytes).subspan(record.begin, record.length);
                addDigitRecord(record.key, digits);
            }
        }
        ++m_epoch;
    }

    [[nodiscard]] Column<std::uint32_t>& keys() noexcept { return m_keys; }

    // --- numeric source precision
    // ----------------------------------------------------------------------------------

    // The significant digits of the fields of the card in `row`, field 1 first (0 for a field that is blank or not a
    // real); empty when the card has no numeric source precision or was made or edited since.
    [[nodiscard]] std::span<const std::uint8_t> significantDigits(std::size_t row) const noexcept
    {
        if (!m_digits)
            return {};
        const std::vector<DigitRecord>& records = m_digits->records;
        const auto found = std::lower_bound(records.begin(), records.end(), m_keys.get(row), byKey);
        if (found == records.end() || found->key != m_keys.get(row))
            return {};
        return std::span<const std::uint8_t>(m_digits->bytes).subspan(found->begin, found->length);
    }

    // Records how the card of `row` was written (the reader calls it for each row a card gave).
    void setSource(std::size_t row, std::span<const std::uint8_t> digits)
    {
        if (!digits.empty())
            addDigitRecord(m_keys.get(row), digits);
    }

    // An edited card no longer carries its previous numeric source precision.
    void forgetSource(std::size_t row)
    {
        if (!m_digits)
            return;
        const std::uint32_t key = m_keys.get(row);
        const std::vector<DigitRecord>& records = m_digits->records;
        const auto found = std::lower_bound(records.begin(), records.end(), key, byKey);
        if (found == records.end() || found->key != key)
            return; // nothing to forget: a shared record list stays shared
        const auto offset = found - records.begin();
        std::vector<DigitRecord>& writable = writableDigits().records;
        writable.erase(writable.begin() + offset);
    }

private:
    // The digits of one source card: `length` bytes of m_digits from `begin`. Sorted by key (cards are read in key
    // order; a record for a smaller key, which a read never makes, is put in its place).
    struct DigitRecord
    {
        std::uint32_t key = 0;
        std::uint32_t begin = 0;
        std::uint32_t length = 0;
    };

    // The digit records and their bytes, shared by the copies of a model until one of them changes them (as the
    // buffers of a Column are): a copy costs no work per card.
    struct Digits
    {
        std::vector<DigitRecord> records;
        std::vector<std::uint8_t> bytes;
    };

    static bool byKey(const DigitRecord& record, std::uint32_t wanted) noexcept { return record.key < wanted; }

    // The digits, allocated on the first source record and cloned when a copy shares them.
    Digits& writableDigits()
    {
        if (!m_digits)
            m_digits = std::make_shared<Digits>();
        else if (m_digits.use_count() > 1)
            m_digits = std::make_shared<Digits>(*m_digits);
        return *m_digits;
    }

    void addDigitRecord(std::uint32_t key, std::span<const std::uint8_t> digits)
    {
        Digits& stored = writableDigits();
        const DigitRecord record{key, static_cast<std::uint32_t>(stored.bytes.size()),
                                 static_cast<std::uint32_t>(digits.size())};
        stored.bytes.insert(stored.bytes.end(), digits.begin(), digits.end());
        std::vector<DigitRecord>& records = stored.records;
        if (records.empty() || records.back().key < key)
        {
            records.push_back(record);
            return;
        }
        const auto place = std::lower_bound(records.begin(), records.end(), key, byKey);
        if (place != records.end() && place->key == key)
            *place = record;
        else
            records.insert(place, record);
    }

    Column<std::uint32_t> m_keys;
    // 1 for a live row, 0 for a dead one: unallocated (and shared by nothing) while no card was removed.
    Column<std::uint8_t> m_alive{1};
    std::size_t m_aliveCount = 0;
    std::uint64_t m_epoch = 1;        // 0 means "no row remembered"
    std::shared_ptr<Digits> m_digits; // null while no source card has digits
};

} // namespace oor::model
