#pragma once

#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct AeroMachFreqPairs2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroMachFreqPairs2Card& card);
    [[nodiscard]] static std::optional<AeroMachFreqPairs2Card> read(FieldReader& reader);
    static void write(const AeroMachFreqPairs2Card& card, FieldWriter& writer);
};

// All the MKAERO2 cards, in the category Aero. A card has no ID (IdSpace::None) and a model may have as many as it
// likes. The pairs (Mach number, frequency) of all the cards are one column of lists.
class AeroMachFreqPairs2Table final : public TypedTable<AeroMachFreqPairs2Table, AeroMachFreqPairs2Card>
{
public:
    static constexpr std::string_view NAME = "AeroMachFreqPairs2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroMachFreqPairs2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroMachFreqPairs2Card&) noexcept { return 0; }
    std::uint32_t append(const AeroMachFreqPairs2Card& card, base::CardKey key);
    [[nodiscard]] AeroMachFreqPairs2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroMachFreqPairs2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the numbers are reals

    // The pairs of a card without copying them: (Mach number, frequency).
    [[nodiscard]] std::span<const std::array<double, 2>> pairsAt(std::uint32_t row) const { return m_pairs.get(row); }

    [[nodiscard]] VariableLengthColumn<std::array<double, 2>>& pairsColumn() noexcept { return m_pairs; }

private:
    VariableLengthColumn<std::array<double, 2>> m_pairs;
};

} // namespace oor::model
