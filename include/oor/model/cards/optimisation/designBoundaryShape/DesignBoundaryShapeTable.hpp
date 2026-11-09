#pragma once

#include "oor/model/cards/optimisation/designBoundaryShape/DesignBoundaryShapeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignBoundaryShapeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignBoundaryShapeCard& card);
    [[nodiscard]] static std::optional<DesignBoundaryShapeCard> read(FieldReader& reader);
    static void write(const DesignBoundaryShapeCard& card, FieldWriter& writer);
};

// All the DVBSHAP cards (no ID: IdSpace::None). The DVID is a design variable; the auxiliary model is a part of the
// deck, not a card.
class DesignBoundaryShapeTable final : public TypedTable<DesignBoundaryShapeTable, DesignBoundaryShapeCard>
{
public:
    static constexpr std::string_view NAME = "DesignBoundaryShape";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignBoundaryShapeFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignBoundaryShapeCard&) noexcept { return 0; }
    std::uint32_t append(const DesignBoundaryShapeCard& card, base::CardKey key);
    [[nodiscard]] DesignBoundaryShapeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignBoundaryShapeCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<DesignBoundaryShapeCard> m_cards;
};

} // namespace oor::model
