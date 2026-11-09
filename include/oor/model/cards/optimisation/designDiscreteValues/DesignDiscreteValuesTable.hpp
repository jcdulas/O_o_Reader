#pragma once

#include "oor/model/cards/optimisation/designDiscreteValues/DesignDiscreteValuesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignDiscreteValuesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignDiscreteValuesCard& card);
    [[nodiscard]] static std::optional<DesignDiscreteValuesCard> read(FieldReader& reader);
    static void write(const DesignDiscreteValuesCard& card, FieldWriter& writer);
};

// All the DDVAL cards. The ID is unique among them (IdSpace::PerCardType) and has a category of its own,
// DiscreteValueSet: the DDVAL of a DESVAR, the DSVID of a TOMVAR, the DDVALID of a CAMPBLL and the SPTID of an RSPINR
// name it. The cards are kept whole next to the ID column.
class DesignDiscreteValuesTable final : public TypedTable<DesignDiscreteValuesTable, DesignDiscreteValuesCard>
{
public:
    static constexpr std::string_view NAME = "DesignDiscreteValues";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DiscreteValueSet, base::IdSpace::PerCardType};
    using Format = DesignDiscreteValuesFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignDiscreteValuesCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignDiscreteValuesCard& card, base::CardKey key);
    [[nodiscard]] DesignDiscreteValuesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignDiscreteValuesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // nothing in a DDVAL names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignDiscreteValuesCard> m_cards;
};

} // namespace oor::model
