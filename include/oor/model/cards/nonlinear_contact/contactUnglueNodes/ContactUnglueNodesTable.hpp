#pragma once

#include "oor/model/cards/nonlinear_contact/contactUnglueNodes/ContactUnglueNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactUnglueNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactUnglueNodesCard& card);
    [[nodiscard]] static std::optional<ContactUnglueNodesCard> read(FieldReader& reader);
    static void write(const ContactUnglueNodesCard& card, FieldWriter& writer);
};

// All the UNGLUE cards. The ID is the number that a BCONTACT or UNGLUE command selects, shared by the cards of a
// subcase (IdSpace::Group, category Other); BID names a contact body and the grid points are nodes. The cards are kept
// whole next to their ID.
class ContactUnglueNodesTable final : public TypedTable<ContactUnglueNodesTable, ContactUnglueNodesCard>
{
public:
    static constexpr std::string_view NAME = "ContactUnglueNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = ContactUnglueNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactUnglueNodesCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactUnglueNodesCard& card, base::CardKey key);
    [[nodiscard]] ContactUnglueNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactUnglueNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactUnglueNodesCard> m_cards;
};

} // namespace oor::model
