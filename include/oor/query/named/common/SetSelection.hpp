#pragma once

#include "oor/analysis/components/LoadSets.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/QueryValue.hpp"
#include "oor/query/named/common/CardList.hpp"

#include <algorithm>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace oor::query
{

// The sets a query was asked about, by `setIds` and by `subcases` (the set that the Case Control command `command` of
// each of those subcases selects): sorted and distinct. Nothing when the caller gave neither.
[[nodiscard]] std::optional<std::vector<base::EntityId>> askedSets(const model::Model& model,
                                                                   const QueryArguments& arguments,
                                                                   std::string_view command);

// The IDs of the load sets of the model (the cards of the Load category), sorted and distinct.
[[nodiscard]] std::vector<base::EntityId> allLoadSets(const model::Model& model);

// The type indexes of these card tables, sorted: the `types` of LoadSetListing::loadsOf.
template <typename... TableT>
[[nodiscard]] std::vector<std::uint16_t> sortedTypesOf()
{
    std::vector<std::uint16_t> types{model::typeIndexOf<TableT>()...};
    std::sort(types.begin(), types.end());
    return types;
}

// The argument of the load queries that leaves the LOAD combinations unexpanded.
inline constexpr std::string_view EXPAND_COMBINATIONS = "expandCombinations";

// The parameter `expandCombinations` of a load query, true by default.
[[nodiscard]] ParameterInfo expandCombinationsParameter();

// The load sets a load query lists and the loads of each, as its argument `expandCombinations` asks (true by
// default). Expanded: the sets asked for, each with the loads LoadSets::expand gives, its LOAD combinations expanded
// down to the cards (the set asked for in `setId`, the set of the card in `sourceSetId`, the product of the factors in
// `scale`). Unexpanded: the sets those expansions reach (LoadSets::reachedSets: a LOAD combination replaced by the sets
// its terms name, nested combinations followed), each once with the loads of its own cards (LoadSets::ownLoads: its
// `setId` the set of its cards, its scale 1), sorted; loadCombinationsTable gives the factors.
class LoadSetListing
{
public:
    LoadSetListing(const model::Model& model, const analysis::LoadSetIndex& index, const QueryArguments& arguments,
                   std::vector<base::EntityId> asked);
    // The same with the form given (`expands` false: each set reached once with its own cards).
    LoadSetListing(const model::Model& model, const analysis::LoadSetIndex& index, std::vector<base::EntityId> asked,
                   bool expands);

    [[nodiscard]] const std::vector<base::EntityId>& sets() const noexcept { return m_sets; }
    [[nodiscard]] bool expands() const noexcept { return m_expands; }
    // Unexpanded, the cards of the sets listed of these `types` (sorted; every type when empty): a row a card for most,
    // what a query reserves; 0 expanded.
    [[nodiscard]] std::size_t cardCount(std::span<const std::uint16_t> types = {}) const;
    // `types` (sorted), when not empty: the query lists the cards of these types only, and the others of an
    // unexpanded set are not read (the expansion reads them all, as before).
    [[nodiscard]] std::vector<analysis::ElementaryLoad> loadsOf(base::EntityId sid,
                                                                std::span<const std::uint16_t> types = {}) const;

private:
    const model::Model& m_model;
    const analysis::LoadSetIndex& m_index;
    std::vector<base::EntityId> m_sets;
    bool m_expands = true;
};

// The element IDs of a group of an `IdSets` argument: the IDs given, or the members of the SET1 cards of its ID, sorted
// and distinct. A SET1 that does not exist throws QueryError, the message starting with `query`.
[[nodiscard]] std::vector<std::int32_t> idSetMembers(const model::Model& model, const NamedIdSet& set,
                                                     std::string_view query);

// The IDs of `sortedIds` (increasing) that lie in the run first..last.
[[nodiscard]] std::span<const std::int32_t> idsInRun(std::span<const std::int32_t> sortedIds, std::int64_t first,
                                                     std::int64_t last);

} // namespace oor::query
