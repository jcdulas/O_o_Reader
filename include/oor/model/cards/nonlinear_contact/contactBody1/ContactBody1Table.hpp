#pragma once

#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactBody1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBody1Card& card);
    [[nodiscard]] static std::optional<ContactBody1Card> read(FieldReader& reader);
    static void write(const ContactBody1Card& card, FieldWriter& writer);
};

// All the BCBODY1 cards. The BID is unique among them (IdSpace::PerCardType, category ContactBody, shared with BCSURF
// and BCGRID, which a BCONECT also names). The parameter set, the surface and the rigid body description are
// references.
class ContactBody1Table final : public TypedTable<ContactBody1Table, ContactBody1Card>
{
public:
    static constexpr std::string_view NAME = "ContactBody1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactBody, base::IdSpace::PerCardType};
    using Format = ContactBody1Format;

    ContactBody1Table() : m_dimension("ThreeDimensional"), m_behavior("Deformable") {}

    [[nodiscard]] static base::EntityId cardId(const ContactBody1Card& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBody1Card& card, base::CardKey key);
    [[nodiscard]] ContactBody1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBody1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& bodyParamsIdColumn() noexcept { return m_bodyParamsId; }
    [[nodiscard]] Column<std::string>& dimensionColumn() noexcept { return m_dimension; }
    [[nodiscard]] Column<std::string>& behaviorColumn() noexcept { return m_behavior; }
    [[nodiscard]] Column<base::EntityId>& surfaceIdColumn() noexcept { return m_surfaceId; }
    [[nodiscard]] Column<base::EntityId>& rigidMotionIdColumn() noexcept { return m_rigidMotionId; }
    [[nodiscard]] Column<std::int32_t>& forceReferencePointColumn() noexcept { return m_forceReferencePoint; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_bodyParamsId;
    Column<std::string> m_dimension;
    Column<std::string> m_behavior;
    Column<base::EntityId> m_surfaceId;
    Column<base::EntityId> m_rigidMotionId;
    Column<std::int32_t> m_forceReferencePoint;
};

} // namespace oor::model
