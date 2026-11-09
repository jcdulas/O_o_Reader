#pragma once

#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignPropRelation2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignPropRelation2Card& card);
    [[nodiscard]] static std::optional<DesignPropRelation2Card> read(FieldReader& reader);
    static void write(const DesignPropRelation2Card& card, FieldWriter& writer);
};

// All the DVPREL2 cards. The ID is unique among the DVPREL2 cards (IdSpace::PerCardType) and in the category
// PropertyRelation with the DVPREL1 cards; the PID is a property of the type TYPE names, the EQID a DEQATN and the
// DVIDs design variables.
class DesignPropRelation2Table final : public TypedTable<DesignPropRelation2Table, DesignPropRelation2Card>
{
public:
    static constexpr std::string_view NAME = "DesignPropRelation2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::PropertyRelation, base::IdSpace::PerCardType};
    using Format = DesignPropRelation2Format;

    [[nodiscard]] static base::EntityId cardId(const DesignPropRelation2Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignPropRelation2Card& card, base::CardKey key);
    [[nodiscard]] DesignPropRelation2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignPropRelation2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignPropRelation2Card> m_cards;
};

} // namespace oor::model
