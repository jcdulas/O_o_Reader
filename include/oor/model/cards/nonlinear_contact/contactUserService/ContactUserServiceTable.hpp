#pragma once

#include "oor/model/cards/nonlinear_contact/contactUserService/ContactUserServiceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactUserServiceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactUserServiceCard& card);
    [[nodiscard]] static std::optional<ContactUserServiceCard> read(FieldReader& reader);
    static void write(const ContactUserServiceCard& card, FieldWriter& writer);
};

// All the BCONUDS cards. The BID names a body that other BCONUDS cards (with other subroutines) may name too, so a card
// has no ID of its own (IdSpace::None, category Other). The cards are kept whole (CardRows); the query engine reads the
// BID. The BID is a reference to a contact body.
class ContactUserServiceTable final : public TypedTable<ContactUserServiceTable, ContactUserServiceCard>
{
public:
    static constexpr std::string_view NAME = "ContactUserService";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ContactUserServiceFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactUserServiceCard&) noexcept { return 0; }
    std::uint32_t append(const ContactUserServiceCard& card, base::CardKey key);
    [[nodiscard]] ContactUserServiceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactUserServiceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& bodyIdColumn() noexcept { return m_bodyId; }

private:
    Column<base::EntityId> m_bodyId;
    CardRows<ContactUserServiceCard> m_cards;
};

} // namespace oor::model
