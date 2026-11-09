#pragma once

#include "oor/model/cards/dynamics/frfSinglePointConstr/FrfSinglePointConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FrfSinglePointConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfSinglePointConstrCard& card);
    [[nodiscard]] static std::optional<FrfSinglePointConstrCard> read(FieldReader& reader);
    static void write(const FrfSinglePointConstrCard& card, FieldWriter& writer);
};

// All the FRFSPC1 cards. The SID is a constraint set ID shared by many cards (IdSpace::Group, category Constraint, as
// SPC1), so a Case Control SPC finds it (ReferenceTarget::ConstraintSet). A component given by its ID is a reference to
// an FRFCOMP (ReferenceTarget::FrfComponent), one given by its name is kept as written; the points are plain numbers of
// the components' own models. The cards are kept whole next to columns of their IDs and component digits.
class FrfSinglePointConstrTable final : public TypedTable<FrfSinglePointConstrTable, FrfSinglePointConstrCard>
{
public:
    static constexpr std::string_view NAME = "FrfSinglePointConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = FrfSinglePointConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfSinglePointConstrCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfSinglePointConstrCard& card, base::CardKey key);
    [[nodiscard]] FrfSinglePointConstrCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FrfSinglePointConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_components;
    CardRows<FrfSinglePointConstrCard> m_cards;
};

} // namespace oor::model
