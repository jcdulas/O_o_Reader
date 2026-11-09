#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace oor::model
{

// The cards of one type kept as whole values, one per row, for the card types whose cards are long records of many
// optional fields and sections (BCBODY, BCTABLE, NLSTEP): a column for each of their fields would be hundreds of lines
// that nobody reads as a column. A table that uses it keeps a Column for the ID (or whatever the query engine reads)
// next to it and keeps the two in step.
//
// A table copy copies the cards (the columns share their buffers, the rows of this one do not): these cards are few.
template <typename CardT>
class CardRows
{
public:
    [[nodiscard]] std::size_t size() const noexcept { return m_cards.size(); }

    void push(CardT card) { m_cards.push_back(std::move(card)); }
    [[nodiscard]] const CardT& get(std::uint32_t row) const { return m_cards[row]; }
    void set(std::uint32_t row, CardT card) { m_cards[row] = std::move(card); }
    // For a visitor that rewrites the references of a card in place.
    [[nodiscard]] CardT& edit(std::uint32_t row) { return m_cards[row]; }

    // Keeps only the rows listed (increasing): the compaction of a table.
    void keepRows(const std::vector<std::uint32_t>& rows)
    {
        std::vector<CardT> kept;
        kept.reserve(rows.size());
        for (const std::uint32_t row : rows)
            kept.push_back(std::move(m_cards[row]));
        m_cards = std::move(kept);
    }

private:
    std::vector<CardT> m_cards;
};

} // namespace oor::model
