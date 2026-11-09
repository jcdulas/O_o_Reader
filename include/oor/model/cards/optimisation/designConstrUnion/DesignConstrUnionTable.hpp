#pragma once

#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignConstrUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignConstrUnionCard& card);
    [[nodiscard]] static std::optional<DesignConstrUnionCard> read(FieldReader& reader);
    static void write(const DesignConstrUnionCard& card, FieldWriter& writer);
};

// All the DCONADD cards. The DCID is a set ID (IdSpace::Group, category DesignConstraint, shared with the DCONSTR
// cards); each DCi names such a set.
class DesignConstrUnionTable final : public TypedTable<DesignConstrUnionTable, DesignConstrUnionCard>
{
public:
    static constexpr std::string_view NAME = "DesignConstrUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignConstraint, base::IdSpace::Group};
    using Format = DesignConstrUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignConstrUnionCard& card) noexcept
    {
        return card.constraintSetId;
    }
    std::uint32_t append(const DesignConstrUnionCard& card, base::CardKey key);
    [[nodiscard]] DesignConstrUnionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignConstrUnionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_constraintSetId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& constraintSetIdColumn() noexcept { return m_constraintSetId; }

private:
    Column<base::EntityId> m_constraintSetId;
    CardRows<DesignConstrUnionCard> m_cards;
};

} // namespace oor::model
