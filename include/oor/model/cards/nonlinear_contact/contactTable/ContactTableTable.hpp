#pragma once

#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactTableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactTableCard& card);
    [[nodiscard]] static std::optional<ContactTableCard> read(FieldReader& reader);
    static void write(const ContactTableCard& card, FieldWriter& writer);

    // The names the sets accept, with what each holds (the fields of the lines of the Card layout).
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the BCTABLE cards. The ID is unique among them and among the BCTABL1 cards (IdSpace::PerCardType, category
// ContactTable): the BCONTACT command names either. The cards are kept whole (CardRows); the query engine reads the ID
// only. The bodies are references to contact bodies; the table IDs among the values (FRIC, HCT, ...) are not references
// (a table may be a TABL3D).
class ContactTableTable final : public TypedTable<ContactTableTable, ContactTableCard>
{
public:
    static constexpr std::string_view NAME = "ContactTable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactTable, base::IdSpace::PerCardType};
    using Format = ContactTableFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactTableCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactTableCard& card, base::CardKey key);
    [[nodiscard]] ContactTableCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactTableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactTableCard> m_cards;
};

} // namespace oor::model
