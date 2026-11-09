#pragma once

#include "oor/model/cards/nonlinear_contact/contactSegment/ContactSegmentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactSegmentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactSegmentCard& card);
    [[nodiscard]] static std::optional<ContactSegmentCard> read(FieldReader& reader);
    static void write(const ContactSegmentCard& card, FieldWriter& writer);
};

// All the BCSEG cards. The ID of the table is the IBODY, the surface the segments make: several cards share it
// (IdSpace::Group, category ContactSurface), so that the BSID of a BCBODY finds it. The segment number is a column of
// its own.
class ContactSegmentTable final : public TypedTable<ContactSegmentTable, ContactSegmentCard>
{
public:
    static constexpr std::string_view NAME = "ContactSegment";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::Group};
    using Format = ContactSegmentFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactSegmentCard& card) noexcept { return card.surfaceId; }
    std::uint32_t append(const ContactSegmentCard& card, base::CardKey key);
    [[nodiscard]] ContactSegmentCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactSegmentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_surfaceId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_surfaceId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& surfaceIdColumn() noexcept { return m_surfaceId; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_surfaceId;
    Column<std::array<base::EntityId, 4>> m_nodes;
};

} // namespace oor::model
