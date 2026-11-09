#pragma once

#include "oor/model/cards/nonlinear_contact/contactAutoParams/ContactAutoParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ContactAutoParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactAutoParamsCard& card);
    [[nodiscard]] static std::optional<ContactAutoParamsCard> read(FieldReader& reader);
    static void write(const ContactAutoParamsCard& card, FieldWriter& writer);

    // The parameters the card allows: its own and those of BCPARA,
    // BCONPRP, BCONPRG and BCBDPRP, in that order (the first rule of a name counts).
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the BCAUTOP cards. Only one is allowed in a deck or a module: the PID (0) is unique among them
// (IdSpace::PerCardType, category Other). The cards are kept whole (CardRows); the query engine reads the PID only. The
// SET1 IDs of the primary parameters ESET, SEEDESET, SEEDGSET, PROPSET and RIGIDSET are references.
class ContactAutoParamsTable final : public TypedTable<ContactAutoParamsTable, ContactAutoParamsCard>
{
public:
    static constexpr std::string_view NAME = "ContactAutoParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactAutoParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactAutoParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactAutoParamsCard& card, base::CardKey key);
    [[nodiscard]] ContactAutoParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactAutoParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactAutoParamsCard> m_cards;
};

} // namespace oor::model
