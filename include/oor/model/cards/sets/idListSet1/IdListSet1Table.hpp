#pragma once

#include "oor/model/cards/sets/idListSet1/IdListSet1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct IdListSet1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IdListSet1Card& card);
    [[nodiscard]] static std::optional<IdListSet1Card> read(FieldReader& reader);
    static void write(const IdListSet1Card& card, FieldWriter& writer);
};

// All the SET1 cards, in the category Set. The ID is a set ID shared by the cards that have it (IdSpace::Group); the
// ReferenceTarget::Set of other cards resolves to it. The IDs of every card are one column of (first, last) runs. The
// members are grid points or elements, which the card cannot tell: they are not references and a renumbering leaves
// them alone.
class IdListSet1Table final : public TypedTable<IdListSet1Table, IdListSet1Card>
{
public:
    static constexpr std::string_view NAME = "IdListSet1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::Group};
    using Format = IdListSet1Format;
    // The SKIN keyword in field 2.
    static constexpr std::string_view SKIN = "SkinElems";

    [[nodiscard]] static base::EntityId cardId(const IdListSet1Card& card) noexcept { return card.id; }
    std::uint32_t append(const IdListSet1Card& card, base::CardKey key);
    [[nodiscard]] IdListSet1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const IdListSet1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& skinColumn() noexcept { return m_skin; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_skin;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
