#pragma once

#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactPairFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactPairCard& card);
    [[nodiscard]] static std::optional<ContactPairCard> read(FieldReader& reader);
    static void write(const ContactPairCard& card, FieldWriter& writer);
};

// All the BCONECT cards. The ID is unique among them (IdSpace::PerCardType, category ContactPair); a BCTABL1 and the
// BCONTACT command name it. The bodies are references to BCBODY1, BCSURF and BCGRID cards, the parameter sets to
// BCONPRG and BCONPRP cards.
class ContactPairTable final : public TypedTable<ContactPairTable, ContactPairCard>
{
public:
    static constexpr std::string_view NAME = "ContactPair";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactPair, base::IdSpace::PerCardType};
    using Format = ContactPairFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactPairCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactPairCard& card, base::CardKey key);
    [[nodiscard]] ContactPairCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactPairCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& geomParamsIdColumn() noexcept { return m_geomParamsId; }
    [[nodiscard]] Column<base::EntityId>& physicalParamsIdColumn() noexcept { return m_physicalParamsId; }
    [[nodiscard]] Column<base::EntityId>& secondaryBodyIdColumn() noexcept { return m_secondaryBodyId; }
    [[nodiscard]] Column<base::EntityId>& primaryBodyIdColumn() noexcept { return m_primaryBodyId; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& secondaryBodiesColumn() noexcept { return m_secondaryBodies; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& primaryBodiesColumn() noexcept { return m_primaryBodies; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_geomParamsId;
    Column<base::EntityId> m_physicalParamsId;
    Column<base::EntityId> m_secondaryBodyId;
    Column<base::EntityId> m_primaryBodyId;
    VariableLengthColumn<base::EntityId> m_secondaryBodies;
    VariableLengthColumn<base::EntityId> m_primaryBodies;
};

} // namespace oor::model
