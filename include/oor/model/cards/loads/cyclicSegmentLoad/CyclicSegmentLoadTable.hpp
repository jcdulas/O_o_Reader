#pragma once

#include "oor/model/cards/loads/cyclicSegmentLoad/CyclicSegmentLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicSegmentLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicSegmentLoadCard& card);
    [[nodiscard]] static std::optional<CyclicSegmentLoadCard> read(FieldReader& reader);
    static void write(const CyclicSegmentLoadCard& card, FieldWriter& writer);
};

// All the LOADCYN cards. The ID is a load set ID shared by many cards (IdSpace::Group): the LOAD Case Control command
// selects it.
class CyclicSegmentLoadTable final : public TypedTable<CyclicSegmentLoadTable, CyclicSegmentLoadCard>
{
public:
    static constexpr std::string_view NAME = "CyclicSegmentLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = CyclicSegmentLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicSegmentLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const CyclicSegmentLoadCard& card, base::CardKey key);
    [[nodiscard]] CyclicSegmentLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CyclicSegmentLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<std::int32_t>& segmentIdColumn() noexcept { return m_segmentId; }
    [[nodiscard]] Column<std::uint8_t>& segmentTypeColumn() noexcept { return m_segmentType; }
    [[nodiscard]] Column<double>& scaleFactor1Column() noexcept { return m_scaleFactor1; }
    [[nodiscard]] Column<base::EntityId>& loadSetId1Column() noexcept { return m_loadSetId1; }
    [[nodiscard]] Column<double>& scaleFactor2Column() noexcept { return m_scaleFactor2; }
    [[nodiscard]] Column<base::EntityId>& loadSetId2Column() noexcept { return m_loadSetId2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_scaleFactor;
    Column<std::int32_t> m_segmentId;
    Column<std::uint8_t> m_segmentType;
    Column<double> m_scaleFactor1;
    Column<base::EntityId> m_loadSetId1;
    Column<double> m_scaleFactor2;
    Column<base::EntityId> m_loadSetId2;
};

} // namespace oor::model
