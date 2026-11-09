#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/providers/CardColumns.hpp"
#include "oor/query/providers/ModelSource.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oor::query
{

// The cards a named query reports on: card types, rows, IDs and card keys, in the order of the IDs (cards that share an
// ID stay in file order). Made from a model at one moment; row numbers are valid until the model is compacted.
struct CardList
{
    std::vector<std::uint16_t> types;
    std::vector<std::uint32_t> rows;
    std::vector<std::int32_t> ids;
    std::vector<std::uint32_t> keys;

    [[nodiscard]] std::size_t size() const noexcept { return rows.size(); }
    [[nodiscard]] model::CardLocation location(std::size_t index) const noexcept
    {
        return model::CardLocation{types[index], rows[index]};
    }
};

// The live cards of these card types, by ID. `only` keeps the cards whose ID is in the list (an ID that no card has is
// ignored).
[[nodiscard]] CardList listCards(const model::Model& model, std::span<const std::uint16_t> types,
                                 const std::optional<std::vector<std::int64_t>>& only = std::nullopt);

// The card types of a category whose cards share the ID space of the category (the properties and materials proper, not
// the cards that complete them: MATT1, MATS1, PBUSHT ... have IDs of their own per card type).
[[nodiscard]] std::vector<std::uint16_t> typesInSpace(model::CardCategory category, base::IdSpace space);

// The type indexes of the cards named (any case); throws QueryError for a name that is not a card type of `category`.
// `what` says what the names are for, in the message ("types").
[[nodiscard]] std::vector<std::uint16_t> typesNamed(const std::vector<std::string>& names, model::CardCategory category,
                                                    const std::string& what);

// The name of a card type ("ShellQuad4").
[[nodiscard]] std::string_view typeName(std::uint16_t type);

// The sorted, distinct IDs of a parameter; nothing for "all".
[[nodiscard]] std::optional<std::vector<std::int64_t>> sortedIds(const std::optional<std::vector<std::int64_t>>& ids);

// The card keys of a list, to say which cards the rows of a table come from.
struct ListOrigin
{
    std::vector<std::uint16_t> types;
    std::vector<std::uint32_t> keys;
};
[[nodiscard]] ListOrigin originOf(const CardList& list);

// The computed and stored columns of the cards of a list, as the query engine gives them (the same code: area, mass,
// the quality metrics, the positions of grid points, the stored `propId`). A column is made once; several that come out
// of one computation (centerX, centerY, centerZ) are kept together.
class ListColumns
{
public:
    ListColumns(std::shared_ptr<model::Model> model, const CardList& list, ExecutionContext& context);

    // The column `name` for the cards of the list (one value per card, null or NaN where the card has none).
    [[nodiscard]] const QueryColumn& get(const std::string& name);
    // True when the column exists for the cards of the list: a stored column of one of their card types, or a computed
    // one.
    [[nodiscard]] bool has(const std::string& name);
    // The values of a column of doubles (NaN for a null), or of IDs as int32 (0 for a null).
    [[nodiscard]] std::vector<double> reals(const std::string& name);
    [[nodiscard]] std::vector<std::int32_t> integers(const std::string& name);

private:
    std::shared_ptr<model::Model> m_model;
    const CardList& m_list;
    ExecutionContext& m_context;
    std::vector<std::uint16_t> m_distinctTypes;
    std::unique_ptr<CardColumns> m_engine;
    std::map<std::string, QueryColumn> m_columns;
};

// The nodes of each card of a list (corner and mid-side nodes, in the order of the card) in compressed rows. A card
// without nodes has an empty row.
struct NodeLists
{
    std::vector<std::int64_t> offsets; // size + 1
    std::vector<std::int32_t> nodes;

    [[nodiscard]] std::span<const std::int32_t> of(std::size_t index) const noexcept
    {
        return std::span<const std::int32_t>(nodes).subspan(
            static_cast<std::size_t>(offsets[index]), static_cast<std::size_t>(offsets[index + 1] - offsets[index]));
    }
};
[[nodiscard]] NodeLists nodesOfCards(const model::Model& model, const CardList& list, ExecutionContext& context);

} // namespace oor::query
