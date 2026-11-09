#pragma once

#include "oor/model/cards/materials/progressiveFailureMat/ProgressiveFailureMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ProgressiveFailureMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ProgressiveFailureMatCard& card);
    [[nodiscard]] static std::optional<ProgressiveFailureMatCard> read(FieldReader& reader);
    static void write(const ProgressiveFailureMatCard& card, FieldWriter& writer);
};

// All the MATCB cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The cards
// are kept whole (CardRows: the lines are a list); the query engine reads the ID only.
class ProgressiveFailureMatTable final : public TypedTable<ProgressiveFailureMatTable, ProgressiveFailureMatCard>
{
public:
    static constexpr std::string_view NAME = "ProgressiveFailureMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = ProgressiveFailureMatFormat;

    [[nodiscard]] static base::EntityId cardId(const ProgressiveFailureMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const ProgressiveFailureMatCard& card, base::CardKey key);
    [[nodiscard]] ProgressiveFailureMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ProgressiveFailureMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MATCB refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ProgressiveFailureMatCard> m_cards;
};

} // namespace oor::model
