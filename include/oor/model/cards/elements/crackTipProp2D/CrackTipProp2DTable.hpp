#pragma once

#include "oor/model/cards/elements/crackTipProp2D/CrackTipProp2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CrackTipProp2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrackTipProp2DCard& card);
    [[nodiscard]] static std::optional<CrackTipProp2DCard> read(FieldReader& reader);
    static void write(const CrackTipProp2DCard& card, FieldWriter& writer);
};

// All the PRAC2D properties. The ID is a property ID, in the space every property shares; MID names a material.
class CrackTipProp2DTable final : public TypedTable<CrackTipProp2DTable, CrackTipProp2DCard>
{
public:
    static constexpr std::string_view NAME = "CrackTipProp2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CrackTipProp2DFormat;

    CrackTipProp2DTable()
        : m_planeCondition(CrackTipProp2DCard::DEFAULT_PLANE_CONDITION),
          m_gammaCoefficient(CrackTipProp2DCard::DEFAULT_GAMMA),
          m_phiAngle(CrackTipProp2DCard::DEFAULT_PHI)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const CrackTipProp2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const CrackTipProp2DCard& card, base::CardKey key);
    [[nodiscard]] CrackTipProp2DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CrackTipProp2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<std::int32_t>& planeConditionColumn() noexcept { return m_planeCondition; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& gammaCoefficientColumn() noexcept { return m_gammaCoefficient; }
    [[nodiscard]] Column<double>& phiAngleColumn() noexcept { return m_phiAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_thickness;
    Column<std::int32_t> m_planeCondition;
    Column<double> m_nonStructMass;
    Column<double> m_gammaCoefficient;
    Column<double> m_phiAngle;
};

} // namespace oor::model
