#pragma once

#include "oor/model/cards/nonlinear_contact/contactPatchSurface/ContactPatchSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactPatchSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactPatchSurfaceCard& card);
    [[nodiscard]] static std::optional<ContactPatchSurfaceCard> read(FieldReader& reader);
    static void write(const ContactPatchSurfaceCard& card, FieldWriter& writer);
};

// All the BCPATCH cards (category ContactSurface, unique among them). The patches are one CSR column of rows of five
// IDs (IDP, G1 to G4); the grid points are references.
class ContactPatchSurfaceTable final : public TypedTable<ContactPatchSurfaceTable, ContactPatchSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "ContactPatchSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactPatchSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactPatchSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactPatchSurfaceCard& card, base::CardKey key);
    [[nodiscard]] ContactPatchSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactPatchSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 5>>& patchesColumn() noexcept { return m_patches; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 5>> m_patches;
};

} // namespace oor::model
