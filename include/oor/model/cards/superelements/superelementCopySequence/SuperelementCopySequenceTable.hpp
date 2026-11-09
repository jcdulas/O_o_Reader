#pragma once

#include "oor/model/cards/superelements/superelementCopySequence/SuperelementCopySequenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementCopySequenceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementCopySequenceCard& card);
    [[nodiscard]] static std::optional<SuperelementCopySequenceCard> read(FieldReader& reader);
    static void write(const SuperelementCopySequenceCard& card, FieldWriter& writer);
};

// All the SEQSEP cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of their SSIDs.
class SuperelementCopySequenceTable final
    : public TypedTable<SuperelementCopySequenceTable, SuperelementCopySequenceCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementCopySequence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementCopySequenceFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementCopySequenceCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementCopySequenceCard& card, base::CardKey key);
    [[nodiscard]] SuperelementCopySequenceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementCopySequenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // PSID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementCopySequenceCard> m_cards;
};

} // namespace oor::model
