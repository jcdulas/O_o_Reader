#pragma once

#include "oor/model/cards/dynamics/freqResponseLoad2/FreqResponseLoad2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqResponseLoad2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqResponseLoad2Card& card);
    [[nodiscard]] static std::optional<FreqResponseLoad2Card> read(FieldReader& reader);
    static void write(const FreqResponseLoad2Card& card, FieldWriter& writer);
};

// All the RLOAD2 cards. The ID is a dynamic load set ID shared by the RLOAD, TLOAD and DAREA cards (IdSpace::Group).
class FreqResponseLoad2Table final : public TypedTable<FreqResponseLoad2Table, FreqResponseLoad2Card>
{
public:
    static constexpr std::string_view NAME = "FreqResponseLoad2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = FreqResponseLoad2Format;

    [[nodiscard]] static base::EntityId cardId(const FreqResponseLoad2Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqResponseLoad2Card& card, base::CardKey key);
    [[nodiscard]] FreqResponseLoad2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqResponseLoad2Card& card);
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
    [[nodiscard]] Column<base::EntityId>& amplitudeTableIdColumn() noexcept { return m_amplitudeTableId; }
    [[nodiscard]] Column<double>& amplitudeColumn() noexcept { return m_amplitude; }
    [[nodiscard]] Column<base::EntityId>& phaseTableIdColumn() noexcept { return m_phaseTableId; }
    [[nodiscard]] Column<double>& phaseAngleColumn() noexcept { return m_phaseAngle; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_delayId;
    Column<double> m_delay;
    Column<base::EntityId> m_phaseId;
    Column<double> m_phase;
    Column<base::EntityId> m_amplitudeTableId;
    Column<double> m_amplitude;
    Column<base::EntityId> m_phaseTableId;
    Column<double> m_phaseAngle;
    Column<std::uint8_t> m_kind;
};

} // namespace oor::model
