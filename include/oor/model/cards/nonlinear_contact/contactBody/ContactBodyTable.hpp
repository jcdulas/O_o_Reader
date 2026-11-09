#pragma once

#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactBodyFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBodyCard& card);
    [[nodiscard]] static std::optional<ContactBodyCard> read(FieldReader& reader);
    static void write(const ContactBodyCard& card, FieldWriter& writer);

    // The names of the fields of the HEAT lines, with what each holds.
    [[nodiscard]] static std::span<const ContactParameterRule> heatRules();
};

// All the BCBODY cards. The BID is unique among them (IdSpace::PerCardType, category ContactBody, shared with BCBODY1,
// BCSURF and BCGRID). The cards are kept whole (CardRows): the sections are optional and few cards have more than the
// first line; the query engine reads the ID only. BSID, the grid points of the rigid body and of its patches and Bezier
// surface are references; the table IDs among the values are not (a table may be a TABL3D).
class ContactBodyTable final : public TypedTable<ContactBodyTable, ContactBodyCard>
{
public:
    static constexpr std::string_view NAME = "ContactBody";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactBody, base::IdSpace::PerCardType};
    using Format = ContactBodyFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactBodyCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBodyCard& card, base::CardKey key);
    [[nodiscard]] ContactBodyCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactBodyCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactBodyCard> m_cards;
};

} // namespace oor::model
