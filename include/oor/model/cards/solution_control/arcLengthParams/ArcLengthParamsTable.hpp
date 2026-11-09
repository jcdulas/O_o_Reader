#pragma once

#include "oor/model/cards/solution_control/arcLengthParams/ArcLengthParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ArcLengthParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ArcLengthParamsCard& card);
    [[nodiscard]] static std::optional<ArcLengthParamsCard> read(FieldReader& reader);
    static void write(const ArcLengthParamsCard& card, FieldWriter& writer);
};

// All the NLPCI cards. The ID is the ID of the NLPARM it goes with: unique among the NLPCI cards
// (IdSpace::PerCardType) and numbering, in the category NonlinearStatic, the same selection as that NLPARM, so that a
// renumbering of the category moves the two together.
class ArcLengthParamsTable final : public TypedTable<ArcLengthParamsTable, ArcLengthParamsCard>
{
public:
    static constexpr std::string_view NAME = "ArcLengthParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearStatic, base::IdSpace::PerCardType};
    using Format = ArcLengthParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ArcLengthParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ArcLengthParamsCard& card, base::CardKey key);
    [[nodiscard]] ArcLengthParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ArcLengthParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // see the class comment: no reference check

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] Column<double>& minArcLengthRatioColumn() noexcept { return m_minArcLengthRatio; }
    [[nodiscard]] Column<double>& maxArcLengthRatioColumn() noexcept { return m_maxArcLengthRatio; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<std::int32_t>& designIterationColumn() noexcept { return m_designIteration; }
    [[nodiscard]] Column<std::int32_t>& maxIncrementsColumn() noexcept { return m_maxIncrements; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_type;
    Column<double> m_minArcLengthRatio{0.25};
    Column<double> m_maxArcLengthRatio{4.0};
    Column<double> m_scaleFactor;
    Column<std::int32_t> m_designIteration{12};
    Column<std::int32_t> m_maxIncrements{20};
};

} // namespace oor::model
