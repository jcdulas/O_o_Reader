#pragma once

#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace oor::query
{

// A set of IDs that a caller names (the `sets` of `massBySet`): the IDs themselves, or the ID of a SET1 card of the
// model whose members they are (`setId`; `ids` is then empty and the query reads the card). A group of the `groups`
// parameter says in `kind` what its IDs are: "components" or "elements".
struct NamedIdSet
{
    std::string name;
    std::vector<std::int64_t> ids;
    std::optional<std::int64_t> setId;
    std::string kind;
};

// The steps a result query keeps: the values themselves (a time, a frequency, a mode number, a load factor), or a
// Python-style slice of the positions of the steps (`times=slice(0, None, 2)`).
struct StepSelection
{
    std::vector<double> values;
    bool isSlice = false;
    std::optional<std::int64_t> start;
    std::optional<std::int64_t> stop;
    std::int64_t step = 1;
};

// A linear combination of static subcases (the `combinations` of the cut queries): its name and the
// factor of each subcase, in the order given.
struct NamedCombination
{
    std::string name;
    std::vector<std::pair<std::int32_t, double>> factors;
};

// A named cut of `sectionCuts`: a plane (`plane`: the point then the normal, six numbers in the basic system), or a
// line of grid points with the elements of one side.
struct NamedCut
{
    std::string name;
    std::vector<double> plane;
    std::vector<std::int64_t> nodes;
    std::vector<std::int64_t> sideElements;
    std::vector<double> point; // the point the moment is taken about (3 numbers, basic system); empty: the default
    // the axes of the cut (basic system): 9 numbers, a (3, 3) array whose columns are the axes 1, 2, 3, row by row; or
    // 6, the x direction then the normal (z); empty: none
    std::vector<double> frame;
};

// The value of one argument of a named query, as the binding hands it over. None is the empty state.
using QueryValue = std::variant<std::monostate, bool, std::int64_t, double, std::string, std::vector<std::int64_t>,
                                std::vector<std::string>, std::vector<double>, std::vector<NamedIdSet>,
                                std::vector<std::pair<std::string, double>>, std::shared_ptr<model::Model>,
                                StepSelection, std::vector<NamedCombination>, std::vector<NamedCut>>;

// What a parameter holds. The binding converts the Python value to the kind the query declares for it.
enum class ParameterKind : std::uint8_t
{
    Bool,
    Integer,
    Real,
    Text,
    IdList,       // a list, array, container or selection of card IDs (or one ID); None for "all"
    IdListOrText, // IDs, or a text (`retain`: a list of nodes or "loadNodes")
    TextList,     // a text or a list of texts
    Point,        // three reals
    IdSets,       // a dict {name: IDs} or a list of SET1 IDs
    Thresholds,   // a dict {name: real}
    Model,        // another model
    Steps,        // a list of step values or a slice of step positions (result queries)
    Pairs,        // a list of pairs of IDs, kept as one flat list of IDs (`relativeDisplacement`)
    Reals,        // a list of numbers
    Values,       // a dict {key: number}, the keys kept as text (`linearisedPrediction`, `allowables`)
    RealsOrText,  // a text ("x") or a list of numbers (a vector): the `axis` of `fastenerForces`
    Groups,       // a dict {name: {"components": IDs}} or {name: {"elements": IDs}}: the groups of a caller
    Combinations, // a dict {name: {subcase: factor}}: linear combinations of static subcases
    Cuts          // a dict {name: {"plane": (point, normal)}} or {name: {"nodes": IDs, "sideElements": IDs}}
};

[[nodiscard]] std::string_view parameterKindName(ParameterKind kind) noexcept;

// The arguments of one call of a named query: the values the caller gave, by parameter name. A parameter that is not
// there (or is None) takes the default the query gives to the getter. A value of the wrong kind raises QueryTypeError
// naming the parameter.
class QueryArguments
{
public:
    void set(std::string name, QueryValue value) { m_values[std::move(name)] = std::move(value); }
    // True when the caller gave a value that is not None.
    [[nodiscard]] bool has(std::string_view name) const;
    // True when the argument is a text (the other form of an IdListOrText parameter).
    [[nodiscard]] bool isText(std::string_view name) const;
    [[nodiscard]] const std::map<std::string, QueryValue, std::less<>>& values() const noexcept { return m_values; }

    [[nodiscard]] bool boolOr(std::string_view name, bool fallback) const;
    [[nodiscard]] std::int64_t integerOr(std::string_view name, std::int64_t fallback) const;
    [[nodiscard]] double realOr(std::string_view name, double fallback) const;
    [[nodiscard]] std::string textOr(std::string_view name, std::string fallback) const;
    [[nodiscard]] std::optional<std::string> text(std::string_view name) const;
    // The value of a parameter that selects a form of the result: its text, or "True" / "False" for a flag; empty when
    // not given.
    [[nodiscard]] std::string formLabel(std::string_view name) const;
    // The IDs, or nothing when the parameter is None (every card).
    [[nodiscard]] std::optional<std::vector<std::int64_t>> ids(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> texts(std::string_view name) const;
    [[nodiscard]] std::optional<std::array<double, 3>> point(std::string_view name) const;
    [[nodiscard]] std::vector<NamedIdSet> idSets(std::string_view name) const;
    // The groups of a `Groups` argument, each with its kind; empty when it is None.
    [[nodiscard]] std::vector<NamedIdSet> groups(std::string_view name) const;
    [[nodiscard]] std::vector<std::pair<std::string, double>> thresholds(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<model::Model> model(std::string_view name) const;
    // The steps a `times`, `freqs`, `modes` or `loadSteps` argument selects; nothing when it is None.
    [[nodiscard]] std::optional<StepSelection> steps(std::string_view name) const;
    // The pairs of IDs of a `Pairs` argument.
    [[nodiscard]] std::vector<std::pair<std::int64_t, std::int64_t>> pairs(std::string_view name) const;
    // The numbers of a `Reals` argument (empty when it is None).
    [[nodiscard]] std::vector<double> reals(std::string_view name) const;
    // The entries of a `Values` argument (the same storage as `thresholds`).
    [[nodiscard]] std::vector<std::pair<std::string, double>> values(std::string_view name) const;
    // The combinations of a `Combinations` argument (empty when it is None).
    [[nodiscard]] std::vector<NamedCombination> combinations(std::string_view name) const;
    // The cuts of a `Cuts` argument (empty when it is None).
    [[nodiscard]] std::vector<NamedCut> cuts(std::string_view name) const;

private:
    [[nodiscard]] const QueryValue* find(std::string_view name) const;

    std::map<std::string, QueryValue, std::less<>> m_values;
};

} // namespace oor::query
