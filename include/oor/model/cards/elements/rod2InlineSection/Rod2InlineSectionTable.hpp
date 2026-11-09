#pragma once

#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct Rod2InlineSectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Rod2InlineSectionCard& card);
    [[nodiscard]] static std::optional<Rod2InlineSectionCard> read(FieldReader& reader);
    static void write(const Rod2InlineSectionCard& card, FieldWriter& writer);
};

// All the CONROD elements (in the element ID space): the nodes are one column of two IDs per element.
class Rod2InlineSectionTable final : public TypedTable<Rod2InlineSectionTable, Rod2InlineSectionCard>
{
public:
    static constexpr std::string_view NAME = "Rod2InlineSection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Rod2InlineSectionFormat;

    [[nodiscard]] static base::EntityId cardId(const Rod2InlineSectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const Rod2InlineSectionCard& card, base::CardKey key);
    [[nodiscard]] Rod2InlineSectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const Rod2InlineSectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<double>& torsionConstantColumn() noexcept { return m_torsionConstant; }
    [[nodiscard]] Column<double>& torsionStressCoefficientColumn() noexcept { return m_torsionStressCoefficient; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_nodes;
    Column<base::EntityId> m_matId;
    Column<double> m_area;
    Column<double> m_torsionConstant;
    Column<double> m_torsionStressCoefficient;
    Column<double> m_nonStructMass;
};

} // namespace oor::model
