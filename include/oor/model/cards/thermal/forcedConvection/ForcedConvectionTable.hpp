#pragma once

#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ForcedConvectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ForcedConvectionCard& card);
    [[nodiscard]] static std::optional<ForcedConvectionCard> read(FieldReader& reader);
    static void write(const ForcedConvectionCard& card, FieldWriter& writer);
};

// All the CONVM cards. The ID is the element ID of the CHBDYP element the card applies to (its own ID per card type,
// IdSpace::PerCardType), also a reference to that element.
class ForcedConvectionTable final : public TypedTable<ForcedConvectionTable, ForcedConvectionCard>
{
public:
    static constexpr std::string_view NAME = "ForcedConvection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ForcedConvectionFormat;

    ForcedConvectionTable();

    [[nodiscard]] static base::EntityId cardId(const ForcedConvectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ForcedConvectionCard& card, base::CardKey key);
    [[nodiscard]] ForcedConvectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ForcedConvectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& convectionPropIdColumn() noexcept { return m_convectionPropId; }
    [[nodiscard]] Column<base::EntityId>& filmNodeColumn() noexcept { return m_filmNode; }
    [[nodiscard]] Column<base::EntityId>& massFlowNodeColumn() noexcept { return m_massFlowNode; }
    [[nodiscard]] Column<base::EntityId>& ambientNode1Column() noexcept { return m_ambientNode1; }
    [[nodiscard]] Column<base::EntityId>& ambientNode2Column() noexcept { return m_ambientNode2; }
    [[nodiscard]] Column<double>& massFlowFactorColumn() noexcept { return m_massFlowFactor; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_convectionPropId;
    Column<base::EntityId> m_filmNode;
    Column<base::EntityId> m_massFlowNode;
    Column<base::EntityId> m_ambientNode1;
    Column<base::EntityId> m_ambientNode2;
    Column<double> m_massFlowFactor;
};

} // namespace oor::model
