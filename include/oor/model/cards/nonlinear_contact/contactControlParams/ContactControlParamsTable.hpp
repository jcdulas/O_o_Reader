#pragma once

#include "oor/model/cards/nonlinear_contact/LegacyContactParameters.hpp"
#include "oor/model/cards/nonlinear_contact/contactControlParams/ContactControlParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactControlParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactControlParamsCard& card);
    [[nodiscard]] static std::optional<ContactControlParamsCard> read(FieldReader& reader);
    static void write(const ContactControlParamsCard& card, FieldWriter& writer);

    // The names the card knows, each with what it holds (every other name is a number).
    [[nodiscard]] static std::span<const LegacyParameterRule> rules();
};

// All the BCTPARM cards. The CSID is unique among them (IdSpace::PerCardType, category Other: it is the number of the
// BCTSET it belongs to, so it must not be a contact table of its own) and is a reference to the contact table of that
// number (ReferenceTarget::ContactTable). The cards are kept whole next to their ID.
class ContactControlParamsTable final : public TypedTable<ContactControlParamsTable, ContactControlParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactControlParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactControlParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactControlParamsCard& card) noexcept
    {
        return card.contactSetId;
    }
    std::uint32_t append(const ContactControlParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactControlParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactControlParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_contactSetId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& contactSetIdColumn() noexcept { return m_contactSetId; }

private:
    Column<base::EntityId> m_contactSetId;
    CardRows<ContactControlParamsCard> m_cards;
};

} // namespace oor::model
