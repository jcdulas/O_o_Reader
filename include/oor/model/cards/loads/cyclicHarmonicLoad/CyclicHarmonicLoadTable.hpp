#pragma once

#include "oor/model/cards/loads/cyclicHarmonicLoad/CyclicHarmonicLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct CyclicHarmonicLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicHarmonicLoadCard& card);
    [[nodiscard]] static std::optional<CyclicHarmonicLoadCard> read(FieldReader& reader);
    static void write(const CyclicHarmonicLoadCard& card, FieldWriter& writer);
};

// All the LOADCYH cards. The ID is a load set ID shared by many cards (IdSpace::Group): the LOAD Case Control command
// selects it.
class CyclicHarmonicLoadTable final : public TypedTable<CyclicHarmonicLoadTable, CyclicHarmonicLoadCard>
{
public:
    static constexpr std::string_view NAME = "CyclicHarmonicLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = CyclicHarmonicLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicHarmonicLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const CyclicHarmonicLoadCard& card, base::CardKey key);
    [[nodiscard]] CyclicHarmonicLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CyclicHarmonicLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<std::int32_t>& harmonicIdColumn() noexcept { return m_harmonicId; }
    [[nodiscard]] Column<std::uint8_t>& harmonicTypeColumn() noexcept { return m_harmonicType; }
    [[nodiscard]] Column<double>& scaleFactor1Column() noexcept { return m_scaleFactor1; }
    [[nodiscard]] Column<base::EntityId>& loadSetId1Column() noexcept { return m_loadSetId1; }
    [[nodiscard]] Column<double>& scaleFactor2Column() noexcept { return m_scaleFactor2; }
    [[nodiscard]] Column<base::EntityId>& loadSetId2Column() noexcept { return m_loadSetId2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_scaleFactor;
    Column<std::int32_t> m_harmonicId;
    Column<std::uint8_t> m_harmonicType;
    Column<double> m_scaleFactor1;
    Column<base::EntityId> m_loadSetId1;
    Column<double> m_scaleFactor2;
    Column<base::EntityId> m_loadSetId2;
};

} // namespace oor::model
