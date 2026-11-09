#pragma once

#include "oor/model/cards/superelements/axisymConeSector/AxisymConeSectorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConeSectorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConeSectorCard& card);
    [[nodiscard]] static std::optional<AxisymConeSectorCard> read(FieldReader& reader);
    static void write(const AxisymConeSectorCard& card, FieldWriter& writer);
};

// All the SECTAX cards, one column for each field. The ID is unique per card type (IdSpace::PerCardType); the format
// reference family of the card is the superelements, but it is a card of the conical shell analysis, so its category is
// Other.
class AxisymConeSectorTable final : public TypedTable<AxisymConeSectorTable, AxisymConeSectorCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConeSector";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = AxisymConeSectorFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConeSectorCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymConeSectorCard& card, base::CardKey key);
    [[nodiscard]] AxisymConeSectorCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConeSectorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    // The ring is a RINGAX, a point of the node ID space.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<double>& phiAngle1Column() noexcept { return m_phiAngle1; }
    [[nodiscard]] Column<double>& phiAngle2Column() noexcept { return m_phiAngle2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<double> m_radius;
    Column<double> m_phiAngle1;
    Column<double> m_phiAngle2;
};

} // namespace oor::model
