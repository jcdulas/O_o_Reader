#pragma once

#include "oor/model/cards/elements/barArbitrarySectionProp/BarArbitrarySectionPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BarArbitrarySectionPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BarArbitrarySectionPropCard& card);
    [[nodiscard]] static std::optional<BarArbitrarySectionPropCard> read(FieldReader& reader);
    static void write(const BarArbitrarySectionPropCard& card, FieldWriter& writer);
};

// All the PBRSECT cards, kept whole next to a column of their PIDs. A PBRSECT is a property (IdSpace::Property): the
// PID of a CBAR finds it. It names a material and the SET1 or SET3 cards of its perimeters and branches.
class BarArbitrarySectionPropTable final : public TypedTable<BarArbitrarySectionPropTable, BarArbitrarySectionPropCard>
{
public:
    static constexpr std::string_view NAME = "BarArbitrarySectionProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BarArbitrarySectionPropFormat;

    [[nodiscard]] static base::EntityId cardId(const BarArbitrarySectionPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BarArbitrarySectionPropCard& card, base::CardKey key);
    [[nodiscard]] BarArbitrarySectionPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BarArbitrarySectionPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BarArbitrarySectionPropCard> m_cards;
};

} // namespace oor::model
