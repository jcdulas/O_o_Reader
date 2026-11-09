#pragma once

#include "oor/model/cards/properties/shellProp/ShellPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellPropCard& card);
    [[nodiscard]] static std::optional<ShellPropCard> read(FieldReader& reader);
    static void write(const ShellPropCard& card, FieldWriter& writer);
};

class ShellPropTable final : public TypedTable<ShellPropTable, ShellPropCard>
{
public:
    static constexpr std::string_view NAME = "ShellProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ShellPropFormat;

    ShellPropTable() : m_bendingStiffnessRatio(1.0), m_shearThicknessRatio(0.833333) {}

    [[nodiscard]] static base::EntityId cardId(const ShellPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellPropCard& card, base::CardKey key);
    [[nodiscard]] ShellPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& membraneMatIdColumn() noexcept { return m_membraneMatId; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<base::EntityId>& bendingMatIdColumn() noexcept { return m_bendingMatId; }
    [[nodiscard]] Column<double>& bendingRatioColumn() noexcept { return m_bendingStiffnessRatio; }
    [[nodiscard]] Column<base::EntityId>& shearMatIdColumn() noexcept { return m_shearMatId; }
    [[nodiscard]] Column<double>& shearRatioColumn() noexcept { return m_shearThicknessRatio; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& fiberDistance1Column() noexcept { return m_fiberDistance1; }
    [[nodiscard]] Column<double>& fiberDistance2Column() noexcept { return m_fiberDistance2; }
    [[nodiscard]] Column<base::EntityId>& couplingMatIdColumn() noexcept { return m_couplingMatId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_membraneMatId;
    Column<double> m_thickness;
    Column<base::EntityId> m_bendingMatId;
    Column<double> m_bendingStiffnessRatio;
    Column<base::EntityId> m_shearMatId;
    Column<double> m_shearThicknessRatio;
    Column<double> m_nonStructMass;
    Column<double> m_fiberDistance1{
        std::numeric_limits<double>::quiet_NaN()}; // NaN where the card leaves Z1 or Z2 blank
    Column<double> m_fiberDistance2{std::numeric_limits<double>::quiet_NaN()};
    Column<base::EntityId> m_couplingMatId;
};

} // namespace oor::model
