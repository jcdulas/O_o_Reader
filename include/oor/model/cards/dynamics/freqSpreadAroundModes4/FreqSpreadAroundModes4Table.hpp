#pragma once

#include "oor/model/cards/dynamics/freqSpreadAroundModes4/FreqSpreadAroundModes4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqSpreadAroundModes4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqSpreadAroundModes4Card& card);
    [[nodiscard]] static std::optional<FreqSpreadAroundModes4Card> read(FieldReader& reader);
    static void write(const FreqSpreadAroundModes4Card& card, FieldWriter& writer);
};

// All the FREQ4 cards. The ID is a frequency set ID shared by the FREQ, FREQ1 to FREQ5 cards (IdSpace::Group).
class FreqSpreadAroundModes4Table final : public TypedTable<FreqSpreadAroundModes4Table, FreqSpreadAroundModes4Card>
{
public:
    static constexpr std::string_view NAME = "FreqSpreadAroundModes4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqSpreadAroundModes4Format;

    [[nodiscard]] static base::EntityId cardId(const FreqSpreadAroundModes4Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqSpreadAroundModes4Card& card, base::CardKey key);
    [[nodiscard]] FreqSpreadAroundModes4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqSpreadAroundModes4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ4 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequency2Column() noexcept { return m_frequency2; }
    [[nodiscard]] Column<double>& frequencySpreadColumn() noexcept { return m_frequencySpread; }
    [[nodiscard]] Column<std::int32_t>& frequencyCountColumn() noexcept { return m_frequencyCount; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_frequency1;
    Column<double> m_frequency2{1.0E20};
    Column<double> m_frequencySpread{0.10};
    Column<std::int32_t> m_frequencyCount{3};
};

} // namespace oor::model
