#pragma once

#include "oor/model/cards/superelements/superelementSupport/SuperelementSupportCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementSupportFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementSupportCard& card);
    [[nodiscard]] static std::optional<SuperelementSupportCard> read(FieldReader& reader);
    static void write(const SuperelementSupportCard& card, FieldWriter& writer);
};

// All the SESUP cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of their SEIDs.
class SuperelementSupportTable final : public TypedTable<SuperelementSupportTable, SuperelementSupportCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementSupport";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementSupportFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementSupportCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementSupportCard& card, base::CardKey key);
    [[nodiscard]] SuperelementSupportCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementSupportCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementSupportCard> m_cards;
};

} // namespace oor::model
