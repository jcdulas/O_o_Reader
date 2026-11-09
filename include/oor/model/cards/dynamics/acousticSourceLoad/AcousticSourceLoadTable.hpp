#pragma once

#include "oor/model/cards/dynamics/acousticSourceLoad/AcousticSourceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSourceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSourceLoadCard& card);
    [[nodiscard]] static std::optional<AcousticSourceLoadCard> read(FieldReader& reader);
    static void write(const AcousticSourceLoadCard& card, FieldWriter& writer);
};

// All the ACSRCE cards. The ID is a dynamic load set ID shared by many cards (IdSpace::Group, category DynamicLoad):
// the DLOAD Case Control command selects it.
class AcousticSourceLoadTable final : public TypedTable<AcousticSourceLoadTable, AcousticSourceLoadCard>
{
public:
    static constexpr std::string_view NAME = "AcousticSourceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = AcousticSourceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticSourceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticSourceLoadCard& card, base::CardKey key);
    [[nodiscard]] AcousticSourceLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticSourceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& excitationIdColumn() noexcept { return m_excitationId; }
    [[nodiscard]] Column<base::EntityId>& delayIdColumn() noexcept { return m_delayId; }
    [[nodiscard]] Column<double>& delayColumn() noexcept { return m_delay; }
    [[nodiscard]] Column<base::EntityId>& phaseIdColumn() noexcept { return m_phaseId; }
    [[nodiscard]] Column<double>& phaseColumn() noexcept { return m_phase; }
    [[nodiscard]] Column<base::EntityId>& powerTableIdColumn() noexcept { return m_powerTableId; }
    [[nodiscard]] Column<double>& powerColumn() noexcept { return m_power; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& bulkModulusColumn() noexcept { return m_bulkModulus; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_delayId;
    Column<double> m_delay;
    Column<base::EntityId> m_phaseId;
    Column<double> m_phase;
    Column<base::EntityId> m_powerTableId;
    Column<double> m_power;
    Column<double> m_density;
    Column<double> m_bulkModulus;
};

} // namespace oor::model
