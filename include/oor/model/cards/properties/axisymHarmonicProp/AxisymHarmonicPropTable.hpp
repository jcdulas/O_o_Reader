#pragma once

#include "oor/model/cards/properties/axisymHarmonicProp/AxisymHarmonicPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymHarmonicPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymHarmonicPropCard& card);
    [[nodiscard]] static std::optional<AxisymHarmonicPropCard> read(FieldReader& reader);
    static void write(const AxisymHarmonicPropCard& card, FieldWriter& writer);
};

// All the PAXSYMH properties, kept in columns. The ID is in the space every property type shares (IdSpace::Property),
// so the PID of a CQUADX or a CTRIAX finds it; the MID is a material and the CID a coordinate system.
class AxisymHarmonicPropTable final : public TypedTable<AxisymHarmonicPropTable, AxisymHarmonicPropCard>
{
public:
    static constexpr std::string_view NAME = "AxisymHarmonicProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AxisymHarmonicPropFormat;

    AxisymHarmonicPropTable() : m_harmonicIndex(AxisymHarmonicPropCard::DEFAULT_HARMONIC_INDEX) {}

    [[nodiscard]] static base::EntityId cardId(const AxisymHarmonicPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymHarmonicPropCard& card, base::CardKey key);
    [[nodiscard]] AxisymHarmonicPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymHarmonicPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::int32_t>& harmonicIndexColumn() noexcept { return m_harmonicIndex; }
    [[nodiscard]] Column<std::int32_t>& integrationColumn() noexcept { return m_integration; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<base::EntityId> m_frameId;
    Column<std::int32_t> m_harmonicIndex;
    Column<std::int32_t> m_integration;
};

} // namespace oor::model
