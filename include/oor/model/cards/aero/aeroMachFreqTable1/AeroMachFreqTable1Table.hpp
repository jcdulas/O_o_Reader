#pragma once

#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct AeroMachFreqTable1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroMachFreqTable1Card& card);
    [[nodiscard]] static std::optional<AeroMachFreqTable1Card> read(FieldReader& reader);
    static void write(const AeroMachFreqTable1Card& card, FieldWriter& writer);
};

// All the MKAERO1 cards, in the category Aero. A card has no ID (IdSpace::None) and a model may have as many as it
// likes. The Mach numbers and the frequencies of the cards are two columns of lists.
class AeroMachFreqTable1Table final : public TypedTable<AeroMachFreqTable1Table, AeroMachFreqTable1Card>
{
public:
    static constexpr std::string_view NAME = "AeroMachFreqTable1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroMachFreqTable1Format;

    [[nodiscard]] static base::EntityId cardId(const AeroMachFreqTable1Card&) noexcept { return 0; }
    std::uint32_t append(const AeroMachFreqTable1Card& card, base::CardKey key);
    [[nodiscard]] AeroMachFreqTable1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroMachFreqTable1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the numbers are reals

    // The numbers of a card without copying them.
    [[nodiscard]] std::span<const double> machsAt(std::uint32_t row) const { return m_machs.get(row); }
    [[nodiscard]] std::span<const double> frequenciesAt(std::uint32_t row) const { return m_frequencies.get(row); }

    [[nodiscard]] VariableLengthColumn<double>& machsColumn() noexcept { return m_machs; }
    [[nodiscard]] VariableLengthColumn<double>& frequenciesColumn() noexcept { return m_frequencies; }

private:
    VariableLengthColumn<double> m_machs;
    VariableLengthColumn<double> m_frequencies;
};

} // namespace oor::model
