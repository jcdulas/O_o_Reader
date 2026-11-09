#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace oor::model
{

// Which rows of a table were edited since a reader last looked, for the readers that keep something derived from a
// column and want to redo only what changed (the cache of node positions in the basic system reads the log of the GRID
// table).
//
// Nothing is recorded until a reader calls enable(): reading a deck of a million cards must not fill a log nobody
// reads. The log is a small list of row numbers; when it would grow too long it says "everything changed" instead, and
// the reader then starts again from the whole table. Row numbers follow a compaction of the table (compact()), so they
// are always the current ones.
//
// The log is copied with the table, so the copy of a model has the same pending changes as the original, and the two
// then evolve independently.
class RowChangeLog
{
public:
    static constexpr std::size_t MAX_ROWS = 1U << 20;

    void enable() noexcept { m_enabled = true; }
    [[nodiscard]] bool enabled() const noexcept { return m_enabled; }

    // A value of this row was edited.
    void noteRow(std::uint32_t row)
    {
        if (!m_enabled || m_everything)
            return;
        if (!m_rows.empty() && m_rows.back() == row)
            return; // the same row edited again and again costs one entry
        if (m_rows.size() >= MAX_ROWS)
        {
            noteEverything();
            return;
        }
        m_rows.push_back(row);
    }
    // The IDs of the rows were changed (a renumbering, a card given another ID): what refers to a row by its ID is
    // stale.
    void noteIdChange() noexcept
    {
        if (m_enabled)
            m_idsChanged = true;
    }
    // Too much changed to say what: the reader starts again.
    void noteEverything()
    {
        if (!m_enabled)
            return;
        m_everything = true;
        m_rows.clear();
    }
    // The table dropped its dead rows: `liveRows` are the rows that stayed, in order. The pending rows are renumbered,
    // and the ones that were dropped are forgotten.
    void noteCompaction(const std::vector<std::uint32_t>& liveRows)
    {
        ++m_compactions;
        if (m_rows.empty())
            return;
        std::vector<std::uint32_t> renumbered;
        renumbered.reserve(m_rows.size());
        for (const std::uint32_t row : m_rows)
        {
            const auto found = std::lower_bound(liveRows.begin(), liveRows.end(), row);
            if (found != liveRows.end() && *found == row)
                renumbered.push_back(static_cast<std::uint32_t>(found - liveRows.begin()));
        }
        m_rows = std::move(renumbered);
    }

    // What a reader needs to know.
    [[nodiscard]] const std::vector<std::uint32_t>& rows() const noexcept { return m_rows; }
    [[nodiscard]] bool everythingChanged() const noexcept { return m_everything; }
    [[nodiscard]] bool idsChanged() const noexcept { return m_idsChanged; }
    // Counts the compactions the table went through; a reader that kept row numbers compares it with its own count.
    [[nodiscard]] std::uint64_t compactions() const noexcept { return m_compactions; }

    // The reader has taken the changes into account.
    void clear() noexcept
    {
        m_rows.clear();
        m_everything = false;
        m_idsChanged = false;
    }

private:
    std::vector<std::uint32_t> m_rows;
    bool m_enabled = false;
    bool m_everything = false;
    bool m_idsChanged = false;
    std::uint64_t m_compactions = 0;
};

} // namespace oor::model
