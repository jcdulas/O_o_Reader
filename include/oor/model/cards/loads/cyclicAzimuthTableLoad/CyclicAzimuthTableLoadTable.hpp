#pragma once

#include "oor/model/cards/loads/cyclicAzimuthTableLoad/CyclicAzimuthTableLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicAzimuthTableLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicAzimuthTableLoadCard& card);
    [[nodiscard]] static std::optional<CyclicAzimuthTableLoadCard> read(FieldReader& reader);
    static void write(const CyclicAzimuthTableLoadCard& card, FieldWriter& writer);
};

// All the LOADCYT cards. The ID is a load set ID shared by many cards (IdSpace::Group): the LOAD Case Control command
// selects it.
class CyclicAzimuthTableLoadTable final : public TypedTable<CyclicAzimuthTableLoadTable, CyclicAzimuthTableLoadCard>
{
public:
    static constexpr std::string_view NAME = "CyclicAzimuthTableLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = CyclicAzimuthTableLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicAzimuthTableLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const CyclicAzimuthTableLoadCard& card, base::CardKey key);
    [[nodiscard]] CyclicAzimuthTableLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CyclicAzimuthTableLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& tableId1Column() noexcept { return m_tableId1; }
    [[nodiscard]] Column<base::EntityId>& loadSetId1Column() noexcept { return m_loadSetId1; }
    [[nodiscard]] Column<std::int32_t>& methodName1Column() noexcept { return m_methodName1; }
    [[nodiscard]] Column<base::EntityId>& tableId2Column() noexcept { return m_tableId2; }
    [[nodiscard]] Column<base::EntityId>& loadSetId2Column() noexcept { return m_loadSetId2; }
    [[nodiscard]] Column<std::int32_t>& methodName2Column() noexcept { return m_methodName2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_tableId1;
    Column<base::EntityId> m_loadSetId1;
    Column<std::int32_t> m_methodName1;
    Column<base::EntityId> m_tableId2;
    Column<base::EntityId> m_loadSetId2;
    Column<std::int32_t> m_methodName2;
};

} // namespace oor::model
