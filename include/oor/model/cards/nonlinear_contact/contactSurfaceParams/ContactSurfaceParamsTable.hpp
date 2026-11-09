#pragma once

#include "oor/model/cards/nonlinear_contact/LegacyContactParameters.hpp"
#include "oor/model/cards/nonlinear_contact/contactSurfaceParams/ContactSurfaceParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactSurfaceParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactSurfaceParamsCard& card);
    [[nodiscard]] static std::optional<ContactSurfaceParamsCard> read(FieldReader& reader);
    static void write(const ContactSurfaceParamsCard& card, FieldWriter& writer);

    // The names the card knows, each with what it holds (every other name is a number that may not be blank).
    [[nodiscard]] static std::span<const LegacyParameterRule> rules();
};

// All the BCTPARA cards. The CSID is unique among them (IdSpace::PerCardType, category Other: it is the number of the
// BCTSET it belongs to, so it must not be a contact table of its own) and is a reference to the contact table of that
// number (ReferenceTarget::ContactTable). The cards are kept whole next to their ID.
class ContactSurfaceParamsTable final : public TypedTable<ContactSurfaceParamsTable, ContactSurfaceParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactSurfaceParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactSurfaceParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactSurfaceParamsCard& card) noexcept
    {
        return card.contactSetId;
    }
    std::uint32_t append(const ContactSurfaceParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactSurfaceParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactSurfaceParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_contactSetId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& contactSetIdColumn() noexcept { return m_contactSetId; }

private:
    Column<base::EntityId> m_contactSetId;
    CardRows<ContactSurfaceParamsCard> m_cards;
};

} // namespace oor::model
