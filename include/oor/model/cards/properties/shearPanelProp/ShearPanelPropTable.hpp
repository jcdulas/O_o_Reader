#pragma once

#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ShearPanelPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShearPanelPropCard& card);
    [[nodiscard]] static std::optional<ShearPanelPropCard> read(FieldReader& reader);
    static void write(const ShearPanelPropCard& card, FieldWriter& writer);
};

class ShearPanelPropTable final : public TypedTable<ShearPanelPropTable, ShearPanelPropCard>
{
public:
    static constexpr std::string_view NAME = "ShearPanelProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ShearPanelPropFormat;

    [[nodiscard]] static base::EntityId cardId(const ShearPanelPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShearPanelPropCard& card, base::CardKey key);
    [[nodiscard]] ShearPanelPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShearPanelPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& extensionalFactor1Column() noexcept { return m_extensionalFactor1; }
    [[nodiscard]] Column<double>& extensionalFactor2Column() noexcept { return m_extensionalFactor2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_thickness;
    Column<double> m_nonStructMass;
    Column<double> m_extensionalFactor1;
    Column<double> m_extensionalFactor2;
};

} // namespace oor::model
