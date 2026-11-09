#pragma once

#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct AeroFlutterFactorsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroFlutterFactorsCard& card);
    [[nodiscard]] static std::optional<AeroFlutterFactorsCard> read(FieldReader& reader);
    static void write(const AeroFlutterFactorsCard& card, FieldWriter& writer);
};

// The FMID of a THRU card as the analysis uses it: the value given when it lies strictly between F1 and FNF (in
// either order), else their middle. A value equal to F1 or FNF is not between them, and would leave the
// formula with 0 / 0.
[[nodiscard]] double aeroFlutterFactorsMiddle(const AeroFlutterFactorsCard& card);

// The factors of a card, in the order of the analysis: the list as it is, or the NF numbers of the THRU form, from F1
// to FNF, with the one in the middle at FMID. The first is F1 and the last FNF; a single factor is F1,
// and F1 = FNF gives NF times the same number.
[[nodiscard]] std::vector<double> aeroFlutterFactorsValues(const AeroFlutterFactorsCard& card);

// All the FLFACT cards, in the category AeroFlutterFactor. The SID is unique among the FLFACT cards (an AEFACT may have
// the same number: IdSpace::PerCardType); a FLUTTER names a list by it (ReferenceTarget::AeroFlutterFactor). The
// factors of the first form of every card are one column of lists; the members of the THRU form are columns, a blank
// FMID is a NaN.
class AeroFlutterFactorsTable final : public TypedTable<AeroFlutterFactorsTable, AeroFlutterFactorsCard>
{
public:
    static constexpr std::string_view NAME = "AeroFlutterFactors";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroFlutterFactor, base::IdSpace::PerCardType};
    using Format = AeroFlutterFactorsFormat;

    AeroFlutterFactorsTable();

    [[nodiscard]] static base::EntityId cardId(const AeroFlutterFactorsCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroFlutterFactorsCard& card, base::CardKey key);
    [[nodiscard]] AeroFlutterFactorsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroFlutterFactorsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The factors of the first form of a card without copying them.
    [[nodiscard]] std::span<const double> factorsAt(std::uint32_t row) const { return m_factors.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& factorsColumn() noexcept { return m_factors; }
    [[nodiscard]] Column<std::uint8_t>& isRangeColumn() noexcept { return m_isRange; }
    [[nodiscard]] Column<double>& factor1Column() noexcept { return m_factor1; }
    [[nodiscard]] Column<double>& factorLastColumn() noexcept { return m_factorLast; }
    [[nodiscard]] Column<std::int32_t>& factorCountColumn() noexcept { return m_factorCount; }
    [[nodiscard]] Column<double>& factorMidColumn() noexcept { return m_factorMid; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_factors;
    Column<std::uint8_t> m_isRange;
    Column<double> m_factor1;
    Column<double> m_factorLast;
    Column<std::int32_t> m_factorCount;
    Column<double> m_factorMid;
};

} // namespace oor::model
