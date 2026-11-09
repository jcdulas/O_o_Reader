#pragma once

#include "oor/model/cards/nonlinear_contact/contactNurbsSurface/ContactNurbsSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactNurbsSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactNurbsSurfaceCard& card);
    [[nodiscard]] static std::optional<ContactNurbsSurfaceCard> read(FieldReader& reader);
    static void write(const ContactNurbsSurfaceCard& card, FieldWriter& writer);
};

// All the BCNURBS cards (category ContactSurface, unique among them). The cards are kept whole (CardRows); the query
// engine reads the ID only. The grid points are references, and so are the trimming curves.
class ContactNurbsSurfaceTable final : public TypedTable<ContactNurbsSurfaceTable, ContactNurbsSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "ContactNurbsSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactNurbsSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactNurbsSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactNurbsSurfaceCard& card, base::CardKey key);
    [[nodiscard]] ContactNurbsSurfaceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactNurbsSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactNurbsSurfaceCard> m_cards;
};

} // namespace oor::model
