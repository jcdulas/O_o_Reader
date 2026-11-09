#pragma once

#include "oor/model/cards/nonlinear_contact/contactBezierSurface/ContactBezierSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactBezierSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBezierSurfaceCard& card);
    [[nodiscard]] static std::optional<ContactBezierSurfaceCard> read(FieldReader& reader);
    static void write(const ContactBezierSurfaceCard& card, FieldWriter& writer);
};

// All the BCBZIER cards (category ContactSurface, unique among them). The grid points are references.
class ContactBezierSurfaceTable final : public TypedTable<ContactBezierSurfaceTable, ContactBezierSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "ContactBezierSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactBezierSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactBezierSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBezierSurfaceCard& card, base::CardKey key);
    [[nodiscard]] ContactBezierSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBezierSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& pointCount1Column() noexcept { return m_pointCount1; }
    [[nodiscard]] Column<std::int32_t>& pointCount2Column() noexcept { return m_pointCount2; }
    [[nodiscard]] Column<std::int32_t>& subdivisions1Column() noexcept { return m_subdivisions1; }
    [[nodiscard]] Column<std::int32_t>& subdivisions2Column() noexcept { return m_subdivisions2; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_pointCount1;
    Column<std::int32_t> m_pointCount2;
    Column<std::int32_t> m_subdivisions1;
    Column<std::int32_t> m_subdivisions2;
    VariableLengthColumn<base::EntityId> m_nodes;
};

} // namespace oor::model
