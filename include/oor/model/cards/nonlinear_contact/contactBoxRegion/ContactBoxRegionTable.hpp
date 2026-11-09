#pragma once

#include "oor/model/cards/nonlinear_contact/contactBoxRegion/ContactBoxRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactBoxRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBoxRegionCard& card);
    [[nodiscard]] static std::optional<ContactBoxRegionCard> read(FieldReader& reader);
    static void write(const ContactBoxRegionCard& card, FieldWriter& writer);
};

// All the BCBOX cards (category ContactSurface, unique among them). The grid points of form 1 are references.
class ContactBoxRegionTable final : public TypedTable<ContactBoxRegionTable, ContactBoxRegionCard>
{
public:
    static constexpr std::string_view NAME = "ContactBoxRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactBoxRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactBoxRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBoxRegionCard& card, base::CardKey key);
    [[nodiscard]] ContactBoxRegionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBoxRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& inclusionRuleColumn() noexcept { return m_inclusionRule; }
    [[nodiscard]] Column<std::uint8_t>& byCoordinatesColumn() noexcept { return m_byCoordinates; }
    [[nodiscard]] Column<std::array<base::EntityId, 8>>& cornerNodesColumn() noexcept { return m_cornerNodes; }
    [[nodiscard]] Column<std::array<double, 24>>& cornerCoordinatesColumn() noexcept { return m_cornerCoordinates; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_inclusionRule;
    Column<std::uint8_t> m_byCoordinates;
    Column<std::array<base::EntityId, 8>> m_cornerNodes;
    Column<std::array<double, 24>> m_cornerCoordinates;
};

} // namespace oor::model
