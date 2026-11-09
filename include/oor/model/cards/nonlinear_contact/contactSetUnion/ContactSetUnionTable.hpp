#pragma once

#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactSetUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactSetUnionCard& card);
    [[nodiscard]] static std::optional<ContactSetUnionCard> read(FieldReader& reader);
    static void write(const ContactSetUnionCard& card, FieldWriter& writer);
};

// All the BCTADD cards. The CSID is unique among the BCTADD cards (IdSpace::PerCardType) and is a contact table
// (category ContactTable, like the BCTSET, BCTABL1, BCTABLE and MDBCTB1 that the BCSET and BCONTACT commands select by
// the same kind of number); the sets are BCTSET IDs and are references to contact tables.
class ContactSetUnionTable final : public TypedTable<ContactSetUnionTable, ContactSetUnionCard>
{
public:
    static constexpr std::string_view NAME = "ContactSetUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactTable, base::IdSpace::PerCardType};
    using Format = ContactSetUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactSetUnionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactSetUnionCard& card, base::CardKey key);
    [[nodiscard]] ContactSetUnionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactSetUnionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& setsColumn() noexcept { return m_sets; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_sets;
};

} // namespace oor::model
