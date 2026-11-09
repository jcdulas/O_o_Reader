#pragma once

#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotordynamicParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotordynamicParamsCard& card);
    [[nodiscard]] static std::optional<RotordynamicParamsCard> read(FieldReader& reader);
    static void write(const RotordynamicParamsCard& card, FieldWriter& writer);
};

// All the RGYRO cards. The ID is a set ID shared by the RGYRO cards (IdSpace::Group): all the cards of a RID are
// analysed together. The reference rotor REFROTR is a Rotor reference (a ROTORG, ROTOR, ROTORAX or
// ROTORSE) when ROTRSEID is 0; a rotor of another superelement is a card of that part and is not looked up.
class RotordynamicParamsTable final : public TypedTable<RotordynamicParamsTable, RotordynamicParamsCard>
{
public:
    static constexpr std::string_view NAME = "RotordynamicParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = RotordynamicParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const RotordynamicParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotordynamicParamsCard& card, base::CardKey key);
    [[nodiscard]] RotordynamicParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RotordynamicParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& synchronousColumn() noexcept { return m_synchronous; }
    [[nodiscard]] Column<base::EntityId>& referenceRotorIdColumn() noexcept { return m_referenceRotorId; }
    [[nodiscard]] Column<std::uint8_t>& speedUnitColumn() noexcept { return m_speedUnit; }
    [[nodiscard]] Column<double>& speedLowColumn() noexcept { return m_speedLow; }
    [[nodiscard]] Column<double>& speedHighColumn() noexcept { return m_speedHigh; }
    [[nodiscard]] Column<double>& speedColumn() noexcept { return m_speed; }
    [[nodiscard]] Column<std::int32_t>& rotorSuperelementIdColumn() noexcept { return m_rotorSuperelementId; }
    [[nodiscard]] Column<double>& viscousDampingFrequencyColumn() noexcept { return m_viscousDampingFrequency; }
    [[nodiscard]] Column<double>& structuralDampingFrequencyColumn() noexcept { return m_structuralDampingFrequency; }
    [[nodiscard]] Column<double>& hybridDampingFrequencyColumn() noexcept { return m_hybridDampingFrequency; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_synchronous;
    Column<base::EntityId> m_referenceRotorId;
    Column<std::uint8_t> m_speedUnit;
    Column<double> m_speedLow;
    Column<double> m_speedHigh{99999.0};
    Column<double> m_speed;
    Column<std::int32_t> m_rotorSuperelementId;
    Column<double> m_viscousDampingFrequency;
    Column<double> m_structuralDampingFrequency;
    Column<double> m_hybridDampingFrequency;
};

} // namespace oor::model
