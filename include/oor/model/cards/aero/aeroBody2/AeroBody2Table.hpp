#pragma once

#include "oor/model/cards/aero/aeroBody2/AeroBody2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroBody2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroBody2Card& card);
    [[nodiscard]] static std::optional<AeroBody2Card> read(FieldReader& reader);
    static void write(const AeroBody2Card& card, FieldWriter& writer);
};

// All the CAERO2 cards. The EID is in the space of the aerodynamic elements (IdSpace::AeroElement, category
// AeroElement) that CAERO1 to CAERO5 share: a PAERO1 names it as one of its bodies (ReferenceTarget::AeroElement) and a
// spline names the elements it owns from the EID on (ReferenceTarget::AeroBox). The PID is a PAERO2
// (ReferenceTarget::AeroProperty), CP a coordinate system, LSB and LINT AEFACT cards (ReferenceTarget::AeroFactor).
class AeroBody2Table final : public TypedTable<AeroBody2Table, AeroBody2Card>
{
public:
    static constexpr std::string_view NAME = "AeroBody2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroElement, base::IdSpace::AeroElement};
    using Format = AeroBody2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroBody2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroBody2Card& card, base::CardKey key);
    [[nodiscard]] AeroBody2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroBody2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<std::int32_t>& slenderElemCountColumn() noexcept { return m_slenderElemCount; }
    [[nodiscard]] Column<std::int32_t>& interferenceElemCountColumn() noexcept { return m_interferenceElemCount; }
    [[nodiscard]] Column<base::EntityId>& slenderListIdColumn() noexcept { return m_slenderListId; }
    [[nodiscard]] Column<base::EntityId>& interferenceListIdColumn() noexcept { return m_interferenceListId; }
    [[nodiscard]] Column<base::EntityId>& interferenceGroupIdColumn() noexcept { return m_interferenceGroupId; }
    [[nodiscard]] Column<std::array<double, 3>>& point1Column() noexcept { return m_point1; }
    [[nodiscard]] Column<double>& lengthColumn() noexcept { return m_length; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_positionFrameId;
    Column<std::int32_t> m_slenderElemCount;
    Column<std::int32_t> m_interferenceElemCount;
    Column<base::EntityId> m_slenderListId;
    Column<base::EntityId> m_interferenceListId;
    Column<base::EntityId> m_interferenceGroupId;
    Column<std::array<double, 3>> m_point1;
    Column<double> m_length;
};

} // namespace oor::model
