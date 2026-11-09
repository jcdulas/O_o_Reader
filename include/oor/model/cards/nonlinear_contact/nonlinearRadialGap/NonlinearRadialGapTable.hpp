#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearRadialGap/NonlinearRadialGapCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct NonlinearRadialGapFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearRadialGapCard& card);
    [[nodiscard]] static std::optional<NonlinearRadialGapCard> read(FieldReader& reader);
    static void write(const NonlinearRadialGapCard& card, FieldWriter& writer);
};

// All the NLRGAP cards. The SID is a nonlinear load set ID shared by the NOLIN cards and the other nonlinear loads
// (IdSpace::Group, category DynamicLoad): the NONLINEAR Case Control command selects it. GA and GB are grid points; the
// three tables are TABLEDi cards (the table of TABK is the absolute value of the field).
class NonlinearRadialGapTable final : public TypedTable<NonlinearRadialGapTable, NonlinearRadialGapCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearRadialGap";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = NonlinearRadialGapFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearRadialGapCard& card) noexcept { return card.setId; }
    std::uint32_t append(const NonlinearRadialGapCard& card, base::CardKey key);
    [[nodiscard]] NonlinearRadialGapCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearRadialGapCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_setId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::string>& planeColumn() noexcept { return m_plane; }
    [[nodiscard]] Column<base::EntityId>& stiffnessTableIdColumn() noexcept { return m_stiffnessTableId; }
    [[nodiscard]] Column<base::EntityId>& clearanceTableIdColumn() noexcept { return m_clearanceTableId; }
    [[nodiscard]] Column<base::EntityId>& frictionTableIdColumn() noexcept { return m_frictionTableId; }
    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<std::uint8_t>& smoothingColumn() noexcept { return m_smoothing; }
    [[nodiscard]] Column<double>& smoothingValueColumn() noexcept { return m_smoothingValue; }

private:
    Column<base::EntityId> m_setId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::string> m_plane{"PlaneXy"};
    Column<base::EntityId> m_stiffnessTableId;
    Column<base::EntityId> m_clearanceTableId;
    Column<base::EntityId> m_frictionTableId;
    Column<double> m_radius;
    Column<std::uint8_t> m_smoothing;
    Column<double> m_smoothingValue{1.0E6};
};

} // namespace oor::model
