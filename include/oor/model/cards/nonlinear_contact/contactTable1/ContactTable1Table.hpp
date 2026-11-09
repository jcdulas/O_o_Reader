#pragma once

#include "oor/model/cards/nonlinear_contact/contactTable1/ContactTable1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactTable1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactTable1Card& card);
    [[nodiscard]] static std::optional<ContactTable1Card> read(FieldReader& reader);
    static void write(const ContactTable1Card& card, FieldWriter& writer);
};

// All the BCTABL1 cards. The BCID is unique among them (IdSpace::PerCardType, category ContactTable, shared with
// BCTABLE), which the BCONTACT command names. The pairs are references to BCONECT cards.
class ContactTable1Table final : public TypedTable<ContactTable1Table, ContactTable1Card>
{
public:
    static constexpr std::string_view NAME = "ContactTable1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactTable, base::IdSpace::PerCardType};
    using Format = ContactTable1Format;

    [[nodiscard]] static base::EntityId cardId(const ContactTable1Card& card) noexcept { return card.id; }
    std::uint32_t append(const ContactTable1Card& card, base::CardKey key);
    [[nodiscard]] ContactTable1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactTable1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
