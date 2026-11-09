#pragma once

#include "oor/model/cards/nonlinear_contact/contactRigidMotion/ContactRigidMotionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactRigidMotionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactRigidMotionCard& card);
    [[nodiscard]] static std::optional<ContactRigidMotionCard> read(FieldReader& reader);
    static void write(const ContactRigidMotionCard& card, FieldWriter& writer);
};

// All the BCRIGID cards. The BCRGID is unique among them (IdSpace::PerCardType, category ContactRigid); a BCBODY1 or a
// BCSURF names it. The cards are kept whole (CardRows); the query engine reads the ID only. CGID, the grid point of the
// load control and the one of the moments are references; the table IDs among the values are not (a table may be a
// TABL3D).
class ContactRigidMotionTable final : public TypedTable<ContactRigidMotionTable, ContactRigidMotionCard>
{
public:
    static constexpr std::string_view NAME = "ContactRigidMotion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactRigid, base::IdSpace::PerCardType};
    using Format = ContactRigidMotionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactRigidMotionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactRigidMotionCard& card, base::CardKey key);
    [[nodiscard]] ContactRigidMotionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactRigidMotionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactRigidMotionCard> m_cards;
};

} // namespace oor::model
