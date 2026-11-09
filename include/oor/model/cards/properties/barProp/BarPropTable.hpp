#pragma once

#include "oor/model/cards/properties/barProp/BarPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BarPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BarPropCard& card);
    [[nodiscard]] static std::optional<BarPropCard> read(FieldReader& reader);
    static void write(const BarPropCard& card, FieldWriter& writer);
};

// All the PBAR cards. The four stress points and the shear factors are (N, 2) NumPy views; a blank K1 or K2 is -1.
class BarPropTable final : public TypedTable<BarPropTable, BarPropCard>
{
public:
    static constexpr std::string_view NAME = "BarProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BarPropFormat;

    BarPropTable() : m_shearFactor({BarPropCard::NO_SHEAR_FACTOR, BarPropCard::NO_SHEAR_FACTOR}) {}

    [[nodiscard]] static base::EntityId cardId(const BarPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BarPropCard& card, base::CardKey key);
    [[nodiscard]] BarPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BarPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<double>& bendingInertia1Column() noexcept { return m_bendingInertia1; }
    [[nodiscard]] Column<double>& bendingInertia2Column() noexcept { return m_bendingInertia2; }
    [[nodiscard]] Column<double>& torsionConstantColumn() noexcept { return m_torsionConstant; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<std::array<double, 2>>& stressPointCColumn() noexcept { return m_stressPointC; }
    [[nodiscard]] Column<std::array<double, 2>>& stressPointDColumn() noexcept { return m_stressPointD; }
    [[nodiscard]] Column<std::array<double, 2>>& stressPointEColumn() noexcept { return m_stressPointE; }
    [[nodiscard]] Column<std::array<double, 2>>& stressPointFColumn() noexcept { return m_stressPointF; }
    [[nodiscard]] Column<std::array<double, 2>>& shearFactorColumn() noexcept { return m_shearFactor; }
    [[nodiscard]] Column<double>& productInertia12Column() noexcept { return m_productInertia12; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_area;
    Column<double> m_bendingInertia1;
    Column<double> m_bendingInertia2;
    Column<double> m_torsionConstant;
    Column<double> m_nonStructMass;
    Column<std::array<double, 2>> m_stressPointC;
    Column<std::array<double, 2>> m_stressPointD;
    Column<std::array<double, 2>> m_stressPointE;
    Column<std::array<double, 2>> m_stressPointF;
    Column<std::array<double, 2>> m_shearFactor;
    Column<double> m_productInertia12;
};

} // namespace oor::model
