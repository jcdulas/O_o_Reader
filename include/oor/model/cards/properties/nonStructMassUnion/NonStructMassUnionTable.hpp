#pragma once

#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonStructMassUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonStructMassUnionCard& card);
    [[nodiscard]] static std::optional<NonStructMassUnionCard> read(FieldReader& reader);
    static void write(const NonStructMassUnionCard& card, FieldWriter& writer);
};

// All the NSMADD cards. The ID is a set ID (IdSpace::Group), the sets of every card are one CSR column.
class NonStructMassUnionTable final : public TypedTable<NonStructMassUnionTable, NonStructMassUnionCard>
{
public:
    static constexpr std::string_view NAME = "NonStructMassUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = NonStructMassUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const NonStructMassUnionCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonStructMassUnionCard& card, base::CardKey key);
    [[nodiscard]] NonStructMassUnionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonStructMassUnionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& setColumn() noexcept { return m_sets; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_sets;
};

} // namespace oor::model
