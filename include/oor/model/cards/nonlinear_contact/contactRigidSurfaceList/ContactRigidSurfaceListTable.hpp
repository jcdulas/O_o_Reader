#pragma once

#include "oor/model/cards/nonlinear_contact/contactRigidSurfaceList/ContactRigidSurfaceListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactRigidSurfaceListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactRigidSurfaceListCard& card);
    [[nodiscard]] static std::optional<ContactRigidSurfaceListCard> read(FieldReader& reader);
    static void write(const ContactRigidSurfaceListCard& card, FieldWriter& writer);
};

// All the BCRGSRF cards (category ContactSurface, unique among them). The RBIDs are references to contact surfaces.
class ContactRigidSurfaceListTable final : public TypedTable<ContactRigidSurfaceListTable, ContactRigidSurfaceListCard>
{
public:
    static constexpr std::string_view NAME = "ContactRigidSurfaceList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactRigidSurfaceListFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactRigidSurfaceListCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactRigidSurfaceListCard& card, base::CardKey key);
    [[nodiscard]] ContactRigidSurfaceListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactRigidSurfaceListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& surfacesColumn() noexcept { return m_surfaces; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_surfaces;
};

} // namespace oor::model
