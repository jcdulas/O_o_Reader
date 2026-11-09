#pragma once

#include "oor/model/cards/nonlinear_contact/contactFaceRegion/ContactFaceRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactFaceRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactFaceRegionCard& card);
    [[nodiscard]] static std::optional<ContactFaceRegionCard> read(FieldReader& reader);
    static void write(const ContactFaceRegionCard& card, FieldWriter& writer);
};

// All the BCSURF cards. The BID is unique among them (IdSpace::PerCardType, category ContactBody, shared with BCBODY1
// and BCGRID, which a BCONECT also names). The cards are kept whole (CardRows); the query engine reads the ID only. The
// elements, the SET3 of an entry of type SET, the grid points, the parameter set, the rigid surface and the rigid body
// are references.
class ContactFaceRegionTable final : public TypedTable<ContactFaceRegionTable, ContactFaceRegionCard>
{
public:
    static constexpr std::string_view NAME = "ContactFaceRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactBody, base::IdSpace::PerCardType};
    using Format = ContactFaceRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactFaceRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactFaceRegionCard& card, base::CardKey key);
    [[nodiscard]] ContactFaceRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactFaceRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactFaceRegionCard> m_cards;
};

} // namespace oor::model
