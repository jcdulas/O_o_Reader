#pragma once

#include "oor/model/cards/nonlinear_contact/contactBodyMove/ContactBodyMoveCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactBodyMoveFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBodyMoveCard& card);
    [[nodiscard]] static std::optional<ContactBodyMoveCard> read(FieldReader& reader);
    static void write(const ContactBodyMoveCard& card, FieldWriter& writer);
};

// All the BCMOVE cards. The ID is unique among them (only one BCMOVE per load case): IdSpace::PerCardType, category
// Other. The bodies are references to BCBODY and BCBODY1 cards.
class ContactBodyMoveTable final : public TypedTable<ContactBodyMoveTable, ContactBodyMoveCard>
{
public:
    static constexpr std::string_view NAME = "ContactBodyMove";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactBodyMoveFormat;

    ContactBodyMoveTable() : m_moveKind("Approach") {}

    [[nodiscard]] static base::EntityId cardId(const ContactBodyMoveCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBodyMoveCard& card, base::CardKey key);
    [[nodiscard]] ContactBodyMoveCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBodyMoveCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& moveKindColumn() noexcept { return m_moveKind; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& bodiesColumn() noexcept { return m_bodies; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_moveKind;
    VariableLengthColumn<base::EntityId> m_bodies;
};

} // namespace oor::model
