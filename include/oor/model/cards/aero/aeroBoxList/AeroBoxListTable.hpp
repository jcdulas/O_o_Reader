#pragma once

#include "oor/model/cards/aero/aeroBoxList/AeroBoxListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroBoxListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroBoxListCard& card);
    [[nodiscard]] static std::optional<AeroBoxListCard> read(FieldReader& reader);
    static void write(const AeroBoxListCard& card, FieldWriter& writer);
};

// All the AELIST cards, in the category AeroList. The SID is unique among the AELIST cards (IdSpace::PerCardType) and
// is what an AESURF, an AECOMP, a SPLINE4 or a SPLINE5 names (ReferenceTarget::AeroList). The items of every card are
// one column of (first, last) runs; each item is an aerodynamic box (ReferenceTarget::AeroBox, which may name nothing).
class AeroBoxListTable final : public TypedTable<AeroBoxListTable, AeroBoxListCard>
{
public:
    static constexpr std::string_view NAME = "AeroBoxList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroList, base::IdSpace::PerCardType};
    using Format = AeroBoxListFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroBoxListCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroBoxListCard& card, base::CardKey key);
    [[nodiscard]] AeroBoxListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroBoxListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
