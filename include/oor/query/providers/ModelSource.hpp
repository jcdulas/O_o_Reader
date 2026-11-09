#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/plan/QuerySource.hpp"
#include "oor/query/providers/CardColumns.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// A card chosen by its type and its card key: how a selection of a container names its cards (a key never changes, so a
// selection still finds its card after a renumbering, and a card deleted since is simply not there).
struct SourceCard
{
    std::uint16_t type = 0;
    std::uint32_t key = 0;
};

// The type indexes of every card type of a category, in registry order.
[[nodiscard]] std::vector<std::uint16_t> typesOfCategory(model::CardCategory category);

// The cards of a model as the source of a query: the live cards of some card types, or of a selection. A snapshot taken
// when it is made: the cards that exist then, in the order of their IDs (ties by card key, which is file order); a
// query that is run again makes a new one, so it sees the model as it is by then.
//
// The columns are the stored columns of the cards, the card type, and the computed columns of the cards (CardColumns).
// A join to "property", "material", "node", "element" or "coordinate" follows the IDs a column holds to the cards of
// that kind, and gives their columns (their computed columns too: the positions of a grid point).
class ModelSource final : public QuerySource
{
public:
    // `link` (optional) is what the tables made from the source use to find the model again, for `apply`.
    ModelSource(std::shared_ptr<model::Model> model, std::string description, std::vector<std::uint16_t> types,
                std::optional<std::vector<SourceCard>> selection = std::nullopt,
                std::shared_ptr<const ModelLink> link = nullptr);

    [[nodiscard]] std::string description() const override { return m_description; }
    [[nodiscard]] std::size_t rowCount() const override { return m_entryRows.size(); }
    [[nodiscard]] std::vector<ColumnDescription> columns() const override { return m_columns.columns(); }
    [[nodiscard]] std::vector<NamedColumn> fetch(std::string_view name, const RowSelection& rows,
                                                 ExecutionContext& context) const override;
    [[nodiscard]] RowOrigin originOf(const RowSelection& rows) const override;
    [[nodiscard]] std::unique_ptr<JoinTarget> joinTarget(std::string_view target) const override;
    [[nodiscard]] std::vector<std::string> defaultJoinKeys(std::string_view target) const override;
    [[nodiscard]] int contractVersion() const override;

    [[nodiscard]] const CardColumns& cardColumns() const noexcept { return m_columns; }

private:
    std::shared_ptr<model::Model> m_model;
    std::string m_description;
    CardColumns m_columns;
    std::vector<std::uint16_t> m_entryTypes;
    std::vector<std::uint32_t> m_entryRows;
    std::vector<std::uint32_t> m_entryKeys;
    std::shared_ptr<const ModelLink> m_link;
    bool m_wholeTable = false;
};

} // namespace oor::query
