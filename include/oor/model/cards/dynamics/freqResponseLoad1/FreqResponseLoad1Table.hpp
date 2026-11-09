#pragma once

#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqResponseLoad1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqResponseLoad1Card& card);
    [[nodiscard]] static std::optional<FreqResponseLoad1Card> read(FieldReader& reader);
    static void write(const FreqResponseLoad1Card& card, FieldWriter& writer);
};

// All the RLOAD1 cards. The ID is a dynamic load set ID shared by the RLOAD, TLOAD and DAREA cards (IdSpace::Group).
class FreqResponseLoad1Table final : public TypedTable<FreqResponseLoad1Table, FreqResponseLoad1Card>
{
public:
    static constexpr std::string_view NAME = "FreqResponseLoad1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = FreqResponseLoad1Format;

    [[nodiscard]] static base::EntityId cardId(const FreqResponseLoad1Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqResponseLoad1Card& card, base::CardKey key);
    [[nodiscard]] FreqResponseLoad1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqResponseLoad1Card& card);
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
    [[nodiscard]] Column<base::EntityId>& realTableIdColumn() noexcept { return m_realTableId; }
    [[nodiscard]] Column<double>& realValueColumn() noexcept { return m_realValue; }
    [[nodiscard]] Column<base::EntityId>& imaginaryTableIdColumn() noexcept { return m_imaginaryTableId; }
    [[nodiscard]] Column<double>& imaginaryValueColumn() noexcept { return m_imaginaryValue; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_delayId;
    Column<double> m_delay;
    Column<base::EntityId> m_phaseId;
    Column<double> m_phase;
    Column<base::EntityId> m_realTableId;
    Column<double> m_realValue;
    Column<base::EntityId> m_imaginaryTableId;
    Column<double> m_imaginaryValue;
    Column<std::uint8_t> m_kind;
};

} // namespace oor::model
