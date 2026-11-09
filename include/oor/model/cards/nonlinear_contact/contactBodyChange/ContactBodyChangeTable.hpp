#pragma once

#include "oor/model/cards/nonlinear_contact/contactBodyChange/ContactBodyChangeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactBodyChangeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBodyChangeCard& card);
    [[nodiscard]] static std::optional<ContactBodyChangeCard> read(FieldReader& reader);
    static void write(const ContactBodyChangeCard& card, FieldWriter& writer);
};

// All the BCHANGE cards. Several cards may have the same ID (a body may be entered more than once): IdSpace::Group,
// category Other. The bodies are references to BCBODY and BCBODY1 cards, the ends of the ranges to grid points that may
// not exist (they are the ends of a range). The ranges are one CSR column of (body, N1, N2, INC) rows.
class ContactBodyChangeTable final : public TypedTable<ContactBodyChangeTable, ContactBodyChangeCard>
{
public:
    static constexpr std::string_view NAME = "ContactBodyChange";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = ContactBodyChangeFormat;

    ContactBodyChangeTable() : m_kind("Node") {}

    [[nodiscard]] static base::EntityId cardId(const ContactBodyChangeCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBodyChangeCard& card, base::CardKey key);
    [[nodiscard]] ContactBodyChangeCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBodyChangeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 4>>& rangesColumn() noexcept { return m_ranges; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_kind;
    VariableLengthColumn<std::array<base::EntityId, 4>> m_ranges;
};

} // namespace oor::model
