#pragma once

#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactParamsCard& card);
    [[nodiscard]] static std::optional<ContactParamsCard> read(FieldReader& reader);
    static void write(const ContactParamsCard& card, FieldWriter& writer);

    // The parameters the card allows, each with what it holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the BCPARA cards. The ID (the subcase, 0 for all) is unique among them (IdSpace::PerCardType, category Other): a
// second BCPARA with ID 0 is a fatal error in the format reference. The cards are kept whole (CardRows): the parameters
// are a list of names and values, and the query engine reads the ID only.
class ContactParamsTable final : public TypedTable<ContactParamsTable, ContactParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no reference: a table ID may be a TABL3D

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactParamsCard> m_cards;
};

} // namespace oor::model
