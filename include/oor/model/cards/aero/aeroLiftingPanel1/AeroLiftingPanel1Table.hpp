#pragma once

#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroLiftingPanel1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroLiftingPanel1Card& card);
    [[nodiscard]] static std::optional<AeroLiftingPanel1Card> read(FieldReader& reader);
    static void write(const AeroLiftingPanel1Card& card, FieldWriter& writer);
};

// All the CAERO1 cards. The EID is in the space of the aerodynamic elements (IdSpace::AeroElement, category
// AeroElement), which CAERO1 shares with CAERO2 to CAERO5 and not with the structural elements: a SPLINE names it
// (ReferenceTarget::AeroElement) and the boxes it takes from the EID on are ReferenceTarget::AeroBox. The PID is a
// PAERO1 (ReferenceTarget::AeroProperty), CP a coordinate system, LSPAN and LCHORD AEFACT cards
// (ReferenceTarget::AeroFactor).
class AeroLiftingPanel1Table final : public TypedTable<AeroLiftingPanel1Table, AeroLiftingPanel1Card>
{
public:
    static constexpr std::string_view NAME = "AeroLiftingPanel1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroElement, base::IdSpace::AeroElement};
    using Format = AeroLiftingPanel1Format;

    [[nodiscard]] static base::EntityId cardId(const AeroLiftingPanel1Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroLiftingPanel1Card& card, base::CardKey key);
    [[nodiscard]] AeroLiftingPanel1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroLiftingPanel1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<std::int32_t>& spanBoxCountColumn() noexcept { return m_spanBoxCount; }
    [[nodiscard]] Column<std::int32_t>& chordBoxCountColumn() noexcept { return m_chordBoxCount; }
    [[nodiscard]] Column<base::EntityId>& spanDivisionListIdColumn() noexcept { return m_spanDivisionListId; }
    [[nodiscard]] Column<base::EntityId>& chordDivisionListIdColumn() noexcept { return m_chordDivisionListId; }
    [[nodiscard]] Column<base::EntityId>& interferenceGroupIdColumn() noexcept { return m_interferenceGroupId; }
    [[nodiscard]] Column<std::array<double, 3>>& point1Column() noexcept { return m_point1; }
    [[nodiscard]] Column<double>& rootChordColumn() noexcept { return m_rootChord; }
    [[nodiscard]] Column<std::array<double, 3>>& point4Column() noexcept { return m_point4; }
    [[nodiscard]] Column<double>& tipChordColumn() noexcept { return m_tipChord; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_positionFrameId;
    Column<std::int32_t> m_spanBoxCount;
    Column<std::int32_t> m_chordBoxCount;
    Column<base::EntityId> m_spanDivisionListId;
    Column<base::EntityId> m_chordDivisionListId;
    Column<base::EntityId> m_interferenceGroupId;
    Column<std::array<double, 3>> m_point1;
    Column<double> m_rootChord;
    Column<std::array<double, 3>> m_point4;
    Column<double> m_tipChord;
};

} // namespace oor::model
