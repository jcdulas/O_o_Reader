#pragma once

#include "oor/model/cards/dynamics/freqModeFractions5/FreqModeFractions5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqModeFractions5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqModeFractions5Card& card);
    [[nodiscard]] static std::optional<FreqModeFractions5Card> read(FieldReader& reader);
    static void write(const FreqModeFractions5Card& card, FieldWriter& writer);
};

// All the FREQ5 cards. The ID is a frequency set ID shared by the FREQ, FREQ1 to FREQ5 cards (IdSpace::Group). The
// fractions of every card are one list in CSR form.
class FreqModeFractions5Table final : public TypedTable<FreqModeFractions5Table, FreqModeFractions5Card>
{
public:
    static constexpr std::string_view NAME = "FreqModeFractions5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqModeFractions5Format;

    [[nodiscard]] static base::EntityId cardId(const FreqModeFractions5Card& card) noexcept { return card.id; }
    std::uint32_t append(const FreqModeFractions5Card& card, base::CardKey key);
    [[nodiscard]] FreqModeFractions5Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqModeFractions5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ5 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequency2Column() noexcept { return m_frequency2; }
    [[nodiscard]] VariableLengthColumn<double>& fractionsColumn() noexcept { return m_fractions; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_frequency1;
    Column<double> m_frequency2{1.0E20};
    VariableLengthColumn<double> m_fractions;
};

} // namespace oor::model
