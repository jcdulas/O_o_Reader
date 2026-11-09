#pragma once

#include "oor/model/cards/dynamics/freqLinearSteps1/FreqLinearSteps1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqLinearSteps1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqLinearSteps1Card& card);
    [[nodiscard]] static std::optional<FreqLinearSteps1Card> read(FieldReader& reader);
    static void write(const FreqLinearSteps1Card& card, FieldWriter& writer);
};

// All the FREQ1 cards. The ID is a frequency set ID shared by the FREQ, FREQ1 and FREQ2 cards (IdSpace::Group).
class FreqLinearSteps1Table final : public TypedTable<FreqLinearSteps1Table, FreqLinearSteps1Card>
{
public:
    static constexpr std::string_view NAME = "FreqLinearSteps1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqLinearSteps1Format;

    [[nodiscard]] static base::EntityId cardId(const FreqLinearSteps1Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqLinearSteps1Card& card, base::CardKey key);
    [[nodiscard]] FreqLinearSteps1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqLinearSteps1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ1 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequencyIncrementColumn() noexcept { return m_frequencyIncrement; }
    [[nodiscard]] Column<std::int32_t>& incrementCountColumn() noexcept { return m_incrementCount; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_frequency1;
    Column<double> m_frequencyIncrement;
    Column<std::int32_t> m_incrementCount{1};
};

} // namespace oor::model
