#pragma once

#include "oor/model/cards/loads/superelementLoadCombination/SuperelementLoadCombinationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementLoadCombinationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementLoadCombinationCard& card);
    [[nodiscard]] static std::optional<SuperelementLoadCombinationCard> read(FieldReader& reader);
    static void write(const SuperelementLoadCombinationCard& card, FieldWriter& writer);
};

// All the CLOAD cards. The CID is a load set ID (IdSpace::Group, category Load), as for LOAD; the Case Control CLOAD
// selects it. Each IDVi is the EXCITEID of an LSEQ (ReferenceTarget::ExcitationSet). The card is kept whole next to a
// column of its CID.
class SuperelementLoadCombinationTable final
    : public TypedTable<SuperelementLoadCombinationTable, SuperelementLoadCombinationCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementLoadCombination";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = SuperelementLoadCombinationFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementLoadCombinationCard& card) noexcept { return card.id; }
    std::uint32_t append(const SuperelementLoadCombinationCard& card, base::CardKey key);
    [[nodiscard]] SuperelementLoadCombinationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementLoadCombinationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SuperelementLoadCombinationCard> m_cards;
};

} // namespace oor::model
