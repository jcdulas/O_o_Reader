#pragma once

#include "oor/model/cards/loads/axisymConePressureLoad/AxisymConePressureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConePressureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConePressureLoadCard& card);
    [[nodiscard]] static std::optional<AxisymConePressureLoadCard> read(FieldReader& reader);
    static void write(const AxisymConePressureLoadCard& card, FieldWriter& writer);
};

// All the PRESAX cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). RID1 and RID2
// name RINGAX cards, points of the node ID space (node references).
class AxisymConePressureLoadTable final : public TypedTable<AxisymConePressureLoadTable, AxisymConePressureLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConePressureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxisymConePressureLoadFormat;

    AxisymConePressureLoadTable()
        : m_phiAngle1(AxisymConePressureLoadCard::DEFAULT_PHI_ANGLE1),
          m_phiAngle2(AxisymConePressureLoadCard::DEFAULT_PHI_ANGLE2)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const AxisymConePressureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymConePressureLoadCard& card, base::CardKey key);
    [[nodiscard]] AxisymConePressureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConePressureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& pressureColumn() noexcept { return m_pressure; }
    [[nodiscard]] Column<base::EntityId>& ringId1Column() noexcept { return m_ringId1; }
    [[nodiscard]] Column<base::EntityId>& ringId2Column() noexcept { return m_ringId2; }
    [[nodiscard]] Column<double>& phiAngle1Column() noexcept { return m_phiAngle1; }
    [[nodiscard]] Column<double>& phiAngle2Column() noexcept { return m_phiAngle2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_pressure;
    Column<base::EntityId> m_ringId1;
    Column<base::EntityId> m_ringId2;
    Column<double> m_phiAngle1;
    Column<double> m_phiAngle2;
};

} // namespace oor::model
