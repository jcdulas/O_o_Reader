#pragma once

#include "oor/model/cards/nonlinear_contact/contactEllipsoidList/ContactEllipsoidListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactEllipsoidListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactEllipsoidListCard& card);
    [[nodiscard]] static std::optional<ContactEllipsoidListCard> read(FieldReader& reader);
    static void write(const ContactEllipsoidListCard& card, FieldWriter& writer);
};

// All the BCELIPS cards (category ContactSurface, unique among them). The names have no column for the query engine
// (text lists).
class ContactEllipsoidListTable final : public TypedTable<ContactEllipsoidListTable, ContactEllipsoidListCard>
{
public:
    static constexpr std::string_view NAME = "ContactEllipsoidList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactEllipsoidListFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactEllipsoidListCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactEllipsoidListCard& card, base::CardKey key);
    [[nodiscard]] ContactEllipsoidListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactEllipsoidListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no reference

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::string> m_ellipsoidNames;
};

} // namespace oor::model
