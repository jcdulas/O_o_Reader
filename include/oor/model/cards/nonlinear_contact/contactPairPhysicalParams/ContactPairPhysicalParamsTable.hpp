#pragma once

#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactPairPhysicalParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactPairPhysicalParamsCard& card);
    [[nodiscard]] static std::optional<ContactPairPhysicalParamsCard> read(FieldReader& reader);
    static void write(const ContactPairPhysicalParamsCard& card, FieldWriter& writer);

    // The parameters the card allows, each with what it holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the BCONPRP cards. The PID is unique among them (IdSpace::PerCardType, category ContactPhysicalParameters); a
// BCONECT names it. The cards are kept whole (CardRows); the query engine reads the ID only. The table IDs among the
// values (FRIC, HCT, ...) are not visited as references: a table may be a TABL3D, which no card types yet.
class ContactPairPhysicalParamsTable final
    : public TypedTable<ContactPairPhysicalParamsTable, ContactPairPhysicalParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactPairPhysicalParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactPhysicalParameters, base::IdSpace::PerCardType};
    using Format = ContactPairPhysicalParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactPairPhysicalParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactPairPhysicalParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactPairPhysicalParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactPairPhysicalParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactPairPhysicalParamsCard> m_cards;
};

} // namespace oor::model
