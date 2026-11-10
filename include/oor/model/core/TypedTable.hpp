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
        // Every typed table supplies store(). Updating one card therefore touches only its columns, rather than
        // rebuilding every row after it. The temporary appended row is a tombstone, reclaimed by Model::compact().
        const auto last = static_cast<std::uint32_t>(m_rows.rowCount() - 1);
        if (row == last)
            return;
        const CardT replacement = derived().card(last);
        derived().store(row, replacement);
        m_rows.forgetSource(row);
        m_rows.kill(last);
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
