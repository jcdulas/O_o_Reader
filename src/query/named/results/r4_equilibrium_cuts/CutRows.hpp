#pragma once

#include "oor/query/named/QueryValue.hpp"
#include "oor/query/providers/quantities/CutConnections.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// What the cut queries (freebody, sectionCut, sectionCuts) share: the resultants of a cut by the provider chain, the
// combinations of subcases, the axes of a plane cut and the rows of their tables.

namespace oor::query
{

// The resultants of one cut, a step of a subcase of a result set each, from the chain of the cuts (grid point forces,
// then the integration of the shell forces). `attempts` says what each way did.
[[nodiscard]] std::vector<ResultantRow> resolveCut(const ResultQueryContext& context, const ResultantRequest& request,
                                                   std::vector<ProviderInfo>& providers,
                                                   std::vector<ProviderAttempt>& attempts);

// Adds the rows of the combinations of subcases (`{name: {subcase: factor}}`, the linear combination of
// static subcases), one for each combination and result set, after the rows of the subcases. A combination that names a
// subcase the rows do not have, or one that is not static, raises a QueryError.
void addCombinations(std::vector<ResultantRow>& rows, const std::vector<NamedCombination>& combinations);

// The axes of a plane cut as the columns of a matrix in the basic system: z along `normal`, x the projection of `xAxis`
// on the plane (by default the basic x axis, the basic y axis when the normal is along x), y completing the
// right-handed system. A QueryError when `xAxis` is along the normal.
[[nodiscard]] base::Mat3 cutAxes(const std::array<double, 3>& normal,
                                 const std::optional<std::array<double, 3>>& xAxis);

// The values of the rows (and of their grid point forces) turned to the axes `axes` (columns in the basic system).
void turnRows(std::vector<ResultantRow>& rows, const base::Mat3& axes);
// The same to the axes of the coordinate system `system` of the model (0: nothing changes).
void turnRowsToSystem(const ResultQueryContext& context, std::vector<ResultantRow>& rows, std::int64_t system);

// True when something carries load across the cut: a grid point of the cut, or a rigid element or an MPC equation (of
// any set) that links the side to the rest (a part hung on a rigid element has no grid point in common with the rest).
// `connections` keeps the rigid elements and MPC equations of the model for the cuts of a call (made on first use).
[[nodiscard]] bool hasLoadPath(const ResultQueryContext& context, const CutDefinition& cut,
                               std::unique_ptr<CutConnections>* connections = nullptr);

// The x axis a call gives (`xAxis`), or nothing.
[[nodiscard]] std::optional<std::array<double, 3>> xAxisOf(const QueryArguments& arguments);

// The extra columns a block of cut rows carries.
struct CutColumns
{
    std::string countColumn = "nodeCount";
    std::string elementCountColumn; // empty: none
    bool withCutName = false;       // `cut` and `station` (sectionCuts)
    std::string cutName;
    double station = std::nan("");
};

// Adds the resultant rows of a cut to an accumulator, in the order design cycle, result set, combination, subcase,
// step, with `combination`, `subcase`, the step, `fx fy fz mx my mz` (complex when the table is), `pointX pointY
// pointZ`, the counts, `isComplete`, `uncountedCards`, `uncountedIds` and `source`. `providerIndex` maps the
// providers of the chain to those of the accumulator (`registerCutProviders`).
void addCutRows(const std::vector<ResultantRow>& rows, const std::vector<ProviderInfo>& providers,
                const std::array<double, 3>& point, const CutColumns& columns, RowAccumulator& accumulator,
                const std::vector<std::int16_t>& providerIndex);
// The blocks addCutRows adds, in order (made on any thread: they read only their arguments).
[[nodiscard]] std::vector<RowBlock> cutBlocks(const std::vector<ResultantRow>& rows,
                                              const std::vector<ProviderInfo>& providers,
                                              const std::array<double, 3>& point, const CutColumns& columns,
                                              const std::vector<std::int16_t>& providerIndex);

// Adds the rows of the grid points of a cut (`detail="nodes"`): `combination`, `subcase`, the step, `nodeId`, `fx fy fz
// mx my mz`, `contribution`.
void addCutNodeRows(const std::vector<ResultantRow>& rows, RowAccumulator& accumulator, std::int16_t provider);

// The providers of the chain registered with an accumulator (once), in order.
[[nodiscard]] std::vector<std::int16_t> registerCutProviders(const std::vector<ProviderInfo>& providers,
                                                             RowAccumulator& accumulator);

} // namespace oor::query
