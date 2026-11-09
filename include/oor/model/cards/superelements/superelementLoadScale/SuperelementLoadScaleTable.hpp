#pragma once

#include "oor/model/cards/superelements/superelementLoadScale/SuperelementLoadScaleCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementLoadScaleFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementLoadScaleCard& card);
    [[nodiscard]] static std::optional<SuperelementLoadScaleCard> read(FieldReader& reader);
    static void write(const SuperelementLoadScaleCard& card, FieldWriter& writer);
};

// All the SELOAD cards. The cards of a load set share their SID (IdSpace::Group, category Load): the Case Control LOAD
// and the EXCITEID of a dynamic load name the set. The cards are kept whole next to a column of their SIDs.
class SuperelementLoadScaleTable final : public TypedTable<SuperelementLoadScaleTable, SuperelementLoadScaleCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementLoadScale";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = SuperelementLoadScaleFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementLoadScaleCard& card) noexcept { return card.id; }
    std::uint32_t append(const SuperelementLoadScaleCard& card, base::CardKey key);
    [[nodiscard]] SuperelementLoadScaleCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementLoadScaleCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // The superelement of each entry.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SuperelementLoadScaleCard> m_cards;
};

} // namespace oor::model
