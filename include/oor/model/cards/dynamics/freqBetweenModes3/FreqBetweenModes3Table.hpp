#pragma once

#include "oor/model/cards/dynamics/freqBetweenModes3/FreqBetweenModes3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqBetweenModes3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqBetweenModes3Card& card);
    [[nodiscard]] static std::optional<FreqBetweenModes3Card> read(FieldReader& reader);
    static void write(const FreqBetweenModes3Card& card, FieldWriter& writer);
};

// All the FREQ3 cards. The ID is a frequency set ID shared by the FREQ, FREQ1 to FREQ5 cards (IdSpace::Group).
class FreqBetweenModes3Table final : public TypedTable<FreqBetweenModes3Table, FreqBetweenModes3Card>
{
public:
    static constexpr std::string_view NAME = "FreqBetweenModes3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqBetweenModes3Format;

    [[nodiscard]] static base::EntityId cardId(const FreqBetweenModes3Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqBetweenModes3Card& card, base::CardKey key);
    [[nodiscard]] FreqBetweenModes3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqBetweenModes3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ3 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequency2Column() noexcept { return m_frequency2; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::int32_t>& frequencyCountColumn() noexcept { return m_frequencyCount; }
    [[nodiscard]] Column<double>& clusteringColumn() noexcept { return m_clustering; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_frequency1;
    Column<double> m_frequency2;
    Column<std::uint8_t> m_kind;
    Column<std::int32_t> m_frequencyCount{10};
    Column<double> m_clustering{1.0};
};

} // namespace oor::model
