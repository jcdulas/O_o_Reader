#pragma once

#include "oor/model/cards/materials/failureMat/FailureMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FailureMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FailureMatCard& card);
    [[nodiscard]] static std::optional<FailureMatCard> read(FieldReader& reader);
    static void write(const FailureMatCard& card, FieldWriter& writer);
};

// All the MATF cards. The ID space is the card type's own: the ID is the one of the material that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). The cards are kept whole (CardRows:
// the criteria have a count of their own); the query engine reads the ID only.
class FailureMatTable final : public TypedTable<FailureMatTable, FailureMatCard>
{
public:
    static constexpr std::string_view NAME = "FailureMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = FailureMatFormat;

    [[nodiscard]] static base::EntityId cardId(const FailureMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const FailureMatCard& card, base::CardKey key);
    [[nodiscard]] FailureMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FailureMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FailureMatCard> m_cards;
};

} // namespace oor::model
