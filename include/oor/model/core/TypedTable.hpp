#pragma once

#include "oor/model/core/CardTable.hpp"

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

// The part of a card table that is the same for every card type, so that each table class only has the code that is
// specific to its card: its columns (append, card, keepRows), its ID column and its references.
//
// A table class `XTable` derives from TypedTable<XTable, XCard> and provides:
//   static constexpr std::string_view NAME;          the card name
//   static const CardTypeInfo INFO;                  category and ID space
//   struct Format { static std::optional<XCard> read(FieldReader&); static void write(const XCard&, FieldWriter&); };
//                                                    (a card that defines several objects gives std::vector<XCard>
//                                                    from Format::readAll instead of read)
//   static EntityId cardId(const XCard&);           the ID of a card value
//   std::uint32_t append(const XCard&, CardKey);     adds a row (columns, row book), returns it
//   XCard card(std::uint32_t row) const;             the values of a row
//   EntityId idAtRow(row) const; void setIdAtRow(row, id);
//   void keepRows(const std::vector<std::uint32_t>&) drops the other rows from every column
//   void visitReferences(row, visitor)               (declared in CardTable)
// This is plain sharing of boilerplate: nothing here describes the fields of a card.
template <typename Derived, typename CardT>
class TypedTable : public CardTable
{
public:
    using CardType = CardT;

    [[nodiscard]] const CardTypeInfo& info() const noexcept final { return Derived::INFO; }

    [[nodiscard]] bool readCards(FieldReader& reader, KeySource& keys, std::vector<std::uint32_t>& rows) final
    {
        rows.clear();
        if constexpr (requires { Derived::Format::readAll(reader); })
        {
            // A card that defines several objects: Format::readAll gives all of them.
            std::vector<CardT> cards = Derived::Format::readAll(reader);
            if (cards.empty() || reader.failed())
                return false;
            for (const CardT& card : cards)
                rows.push_back(derived().append(card, keys.next()));
            return true;
        }
        else
        {
            std::optional<CardT> card = Derived::Format::read(reader);
            if (!card || reader.failed())
                return false;
            rows.push_back(derived().append(*card, keys.next()));
            return true;
        }
    }

    void writeCard(std::uint32_t row, FieldWriter& writer) const final
    {
        Derived::Format::write(derived().card(row), writer);
    }

    void compact() final
    {
        const std::vector<std::uint32_t> live = m_rows.liveRows();
        derived().keepRows(live);
        m_rows.applyCompaction(live);
    }

    [[nodiscard]] std::unique_ptr<CardTable> clone() const final { return std::make_unique<Derived>(derived()); }

    void moveLastRowTo(std::uint32_t row) final
    {
        // The rows after `row` are taken out (with what the row book knows of them) and put back after the new card:
        // the table only offers append and keepRows, which keep the rows in order.
        struct SavedRow
        {
            CardT card;
            base::CardKey key;
            bool alive;
            std::vector<std::uint8_t> digits;
        };
        const auto last = static_cast<std::uint32_t>(m_rows.rowCount() - 1);
        const auto save = [&](std::uint32_t from)
        {
            const std::span<const std::uint8_t> digits = m_rows.significantDigits(from);
            return SavedRow{derived().card(from), m_rows.keyAt(from), m_rows.isAlive(from),
                            std::vector<std::uint8_t>(digits.begin(), digits.end())};
        };
        const CardT replacement = derived().card(last);
        const base::CardKey key = m_rows.keyAt(row);
        std::vector<SavedRow> after;
        for (std::uint32_t from = row + 1; from < last; ++from)
            after.push_back(save(from));
        std::vector<std::uint32_t> before(row);
        std::vector<std::uint32_t> deadBefore;
        for (std::uint32_t kept = 0; kept < row; ++kept)
        {
            before[kept] = kept;
            if (!m_rows.isAlive(kept))
                deadBefore.push_back(kept);
        }
        derived().keepRows(before);
        m_rows.applyCompaction(before); // every row kept is alive again: the dead ones are killed below
        for (const std::uint32_t dead : deadBefore)
            m_rows.kill(dead);
        (void)derived().append(replacement, key);
        for (const SavedRow& saved : after)
        {
            const std::uint32_t added = derived().append(saved.card, saved.key);
            m_rows.setSource(added, saved.digits);
            if (!saved.alive)
                m_rows.kill(added);
        }
    }

    [[nodiscard]] base::EntityId idAt(std::uint32_t row) const final { return derived().idAtRow(row); }
    void setIdAt(std::uint32_t row, base::EntityId id) final { derived().setIdAtRow(row, id); }

protected:
    TypedTable() = default;

private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

} // namespace oor::model
