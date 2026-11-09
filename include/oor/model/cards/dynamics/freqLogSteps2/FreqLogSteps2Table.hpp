#pragma once

#include "oor/model/cards/dynamics/freqLogSteps2/FreqLogSteps2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqLogSteps2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqLogSteps2Card& card);
    [[nodiscard]] static std::optional<FreqLogSteps2Card> read(FieldReader& reader);
    static void write(const FreqLogSteps2Card& card, FieldWriter& writer);
};

// All the FREQ2 cards. The ID is a frequency set ID shared by the FREQ, FREQ1 and FREQ2 cards (IdSpace::Group).
class FreqLogSteps2Table final : public TypedTable<FreqLogSteps2Table, FreqLogSteps2Card>
{
public:
    static constexpr std::string_view NAME = "FreqLogSteps2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqLogSteps2Format;

    [[nodiscard]] static base::EntityId cardId(const FreqLogSteps2Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqLogSteps2Card& card, base::CardKey key);
    [[nodiscard]] FreqLogSteps2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqLogSteps2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ2 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequency2Column() noexcept { return m_frequency2; }
    [[nodiscard]] Column<std::int32_t>& intervalCountColumn() noexcept { return m_intervalCount; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_frequency1;
    Column<double> m_frequency2;
    Column<std::int32_t> m_intervalCount{1};
};

} // namespace oor::model
