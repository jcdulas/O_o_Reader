#pragma once

#include "oor/model/cards/loads/beam3DistributedLoad/Beam3DistributedLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct Beam3DistributedLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Beam3DistributedLoadCard& card);
    [[nodiscard]] static std::optional<Beam3DistributedLoadCard> read(FieldReader& reader);
    static void write(const Beam3DistributedLoadCard& card, FieldWriter& writer);
};

// All the PLOADB3 cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load): the LOAD Case
// Control command, an LSEQ or the EXCITEID of a dynamic load names it. The EID is an element, and the CID, when it is
// the ID of a coordinate system, a coordinate system. The cards are kept whole next to a column of their SIDs (the CID
// is a word or a number, so the card is one value).
class Beam3DistributedLoadTable final : public TypedTable<Beam3DistributedLoadTable, Beam3DistributedLoadCard>
{
public:
    static constexpr std::string_view NAME = "Beam3DistributedLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = Beam3DistributedLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const Beam3DistributedLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const Beam3DistributedLoadCard& card, base::CardKey key);
    [[nodiscard]] Beam3DistributedLoadCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const Beam3DistributedLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<Beam3DistributedLoadCard> m_cards;
};

} // namespace oor::model
