#pragma once

#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactPairGeomParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactPairGeomParamsCard& card);
    [[nodiscard]] static std::optional<ContactPairGeomParamsCard> read(FieldReader& reader);
    static void write(const ContactPairGeomParamsCard& card, FieldWriter& writer);

    // The parameters the card allows, each with what it holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the BCONPRG cards. The BCGPID is unique among them (IdSpace::PerCardType, category ContactGeometricParameters); a
// BCONECT names it. The cards are kept whole (CardRows); the query engine reads the ID only. The table IDs and grid IDs
// among the values are not visited as references: a table may be a TABL3D, which no card types yet.
class ContactPairGeomParamsTable final : public TypedTable<ContactPairGeomParamsTable, ContactPairGeomParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactPairGeomParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactGeometricParameters, base::IdSpace::PerCardType};
    using Format = ContactPairGeomParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactPairGeomParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactPairGeomParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactPairGeomParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactPairGeomParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactPairGeomParamsCard> m_cards;
};

} // namespace oor::model
