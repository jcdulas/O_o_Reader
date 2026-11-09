#pragma once

#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactElemSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactElemSurfaceCard& card);
    [[nodiscard]] static std::optional<ContactElemSurfaceCard> read(FieldReader& reader);
    static void write(const ContactElemSurfaceCard& card, FieldWriter& writer);
};

// All the BSURF cards. The ID is unique among the cards that make a contact surface (category ContactSurface), which is
// what the BSID of a BCBODY or BCBODY1 names. The element lists are one CSR column of (first, last, by) runs.
class ContactElemSurfaceTable final : public TypedTable<ContactElemSurfaceTable, ContactElemSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "ContactElemSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactElemSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactElemSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactElemSurfaceCard& card, base::CardKey key);
    [[nodiscard]] ContactElemSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactElemSurfaceCard& card);
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
