#pragma once

#include "oor/model/cards/thermal/trimCouplingTolerances/TrimCouplingTolerancesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TrimCouplingTolerancesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TrimCouplingTolerancesCard& card);
    [[nodiscard]] static std::optional<TrimCouplingTolerancesCard> read(FieldReader& reader);
    static void write(const TrimCouplingTolerancesCard& card, FieldWriter& writer);
};

// All the TRMCPL cards, in the category Other. The TID is the trim component, which no card defines and which the cards
// of the component share (IdSpace::Group: one card for each interface coupling type). The card names no other card, so
// its fields are columns.
class TrimCouplingTolerancesTable final : public TypedTable<TrimCouplingTolerancesTable, TrimCouplingTolerancesCard>
{
public:
    static constexpr std::string_view NAME = "TrimCouplingTolerances";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = TrimCouplingTolerancesFormat;

    TrimCouplingTolerancesTable()
        : m_relativeTolerance(TRIM_TOLERANCES_DEFAULT_RELATIVE),
          m_gapTolerance1(TRIM_TOLERANCES_DEFAULT_GAP),
          m_gapTolerance2(TRIM_TOLERANCES_DEFAULT_GAP),
          m_gapTolerance3(TRIM_TOLERANCES_DEFAULT_GAP),
          m_gapTolerance4(TRIM_TOLERANCES_DEFAULT_GAP)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const TrimCouplingTolerancesCard& card) noexcept { return card.id; }
    std::uint32_t append(const TrimCouplingTolerancesCard& card, base::CardKey key);
    [[nodiscard]] TrimCouplingTolerancesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TrimCouplingTolerancesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TRMCPL refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& couplingKindColumn() noexcept { return m_couplingKind; }
    [[nodiscard]] Column<double>& relativeToleranceColumn() noexcept { return m_relativeTolerance; }
    [[nodiscard]] Column<double>& gapTolerance1Column() noexcept { return m_gapTolerance1; }
    [[nodiscard]] Column<double>& gapTolerance2Column() noexcept { return m_gapTolerance2; }
    [[nodiscard]] Column<double>& gapTolerance3Column() noexcept { return m_gapTolerance3; }
    [[nodiscard]] Column<double>& gapTolerance4Column() noexcept { return m_gapTolerance4; }
    [[nodiscard]] Column<base::EntityId>& regionIdColumn() noexcept { return m_regionId; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_couplingKind;
    Column<double> m_relativeTolerance;
    Column<double> m_gapTolerance1;
    Column<double> m_gapTolerance2;
    Column<double> m_gapTolerance3;
    Column<double> m_gapTolerance4;
    Column<base::EntityId> m_regionId;
};

} // namespace oor::model
