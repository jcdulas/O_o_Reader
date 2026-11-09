#pragma once

#include "oor/model/cards/elements/userDefined7/UserDefined7Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct UserDefined7Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserDefined7Card& card);
    [[nodiscard]] static std::optional<UserDefined7Card> read(FieldReader& reader);
    static void write(const UserDefined7Card& card, FieldWriter& writer);
};

// All the CDUM7 elements. The ID is an element ID, in the space every element shares (IdSpace::Element). The table
// keeps the fields of the card as two columns (the kind and the number of each field) and, next to them, the grid
// points the model sees: the leading run of positive integers of the list, kept in step with it.
class UserDefined7Table final : public TypedTable<UserDefined7Table, UserDefined7Card>
{
public:
    static constexpr std::string_view NAME = "UserDefined7";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = UserDefined7Format;

    [[nodiscard]] static base::EntityId cardId(const UserDefined7Card& card) noexcept { return card.id; }
    std::uint32_t append(const UserDefined7Card& card, base::CardKey key);
    [[nodiscard]] UserDefined7Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const UserDefined7Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The grid points of the element in `row`, without copying the card (the geometry and the topology read them).
    [[nodiscard]] std::span<const base::EntityId> nodes(std::uint32_t row) const { return m_nodes.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] DummyFieldColumns& fieldColumns() noexcept { return m_fields; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    VariableLengthColumn<base::EntityId> m_nodes;
    DummyFieldColumns m_fields;
};

} // namespace oor::model
