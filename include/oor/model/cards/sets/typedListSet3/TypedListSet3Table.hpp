#pragma once

#include "oor/model/cards/sets/typedListSet3/TypedListSet3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct TypedListSet3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TypedListSet3Card& card);
    [[nodiscard]] static std::optional<TypedListSet3Card> read(FieldReader& reader);
    static void write(const TypedListSet3Card& card, FieldWriter& writer);

    // The word of the DES field, and the kind a word names.
    [[nodiscard]] static std::string_view kindName(TypedListSet3Kind kind);
    [[nodiscard]] static std::optional<TypedListSet3Kind> kindFromName(std::string_view name);
};

// All the SET3 cards, in the category Set. The ID is a set ID shared by the cards that have it (IdSpace::Group); the
// ReferenceTarget::Set of other cards resolves to it. The IDs of every card are one column of (first, last) runs, and
// they are references of the kind DES says: grid points, elements (also the rigid elements of RBEin and RBEex) and
// properties follow a renumbering; points and modules are not references.
class TypedListSet3Table final : public TypedTable<TypedListSet3Table, TypedListSet3Card>
{
public:
    static constexpr std::string_view NAME = "TypedListSet3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::Group};
    using Format = TypedListSet3Format;

    [[nodiscard]] static base::EntityId cardId(const TypedListSet3Card& card) noexcept { return card.id; }
    std::uint32_t append(const TypedListSet3Card& card, base::CardKey key);
    [[nodiscard]] TypedListSet3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TypedListSet3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_kind;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
