#pragma once

#include "oor/model/cards/nonlinear_contact/contactMaterialRegion/ContactMaterialRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactMaterialRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactMaterialRegionCard& card);
    [[nodiscard]] static std::optional<ContactMaterialRegionCard> read(FieldReader& reader);
    static void write(const ContactMaterialRegionCard& card, FieldWriter& writer);
};

// All the BCMATL cards (category ContactSurface, unique among them). The materials are references to material cards.
class ContactMaterialRegionTable final : public TypedTable<ContactMaterialRegionTable, ContactMaterialRegionCard>
{
public:
    static constexpr std::string_view NAME = "ContactMaterialRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactMaterialRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactMaterialRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactMaterialRegionCard& card, base::CardKey key);
    [[nodiscard]] ContactMaterialRegionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactMaterialRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& materialsColumn() noexcept { return m_materials; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_materials;
};

} // namespace oor::model
