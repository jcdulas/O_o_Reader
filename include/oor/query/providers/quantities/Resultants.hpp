#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace oor::query
{

class CutSession;

// The force of the side on one grid point of a cut (`detail="nodes"`): the elements of the side at a cut grid point, or
// a counted cut connection at one of its grid points on the side.
struct CutNodeForce
{
    std::int32_t node = 0;
    std::string contribution; // element, rigid, mpc
    std::array<double, 6> real{};
    std::array<double, 6> imaginary{};
};

// The resultant force and moment of a set of loads or reactions at one step of one subcase of one result set, about a
// point, in the basic system. A cut adds what it did not count, its grid point forces when asked, and a combination of
// subcases its name.
struct ResultantRow
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    double step = std::nan("");
    double eigenvalue = std::nan("");
    Precision precision = Precision::Double;
    std::int16_t provider = 0;
    std::array<double, 6> value = {std::nan(""), std::nan(""), std::nan(""), std::nan(""), std::nan(""), std::nan("")};
    std::array<double, 6> imaginary = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // the imaginary parts of a complex resultant
    bool isComplex = false;
    bool magnitudePhase = false;   // the table stored magnitude and phase
    std::int64_t count = 0;        // the nodes added up
    std::int64_t elementCount = 0; // the elements that carry the load across a cut
    std::string note;
    bool approximate = false;
    std::string combination;                 // the combination of subcases the row is; empty for a subcase of the file
    bool complete = true;                    // a cut: every load path across it is counted
    std::vector<std::string> uncountedCards; // a cut: what links the two sides and is not counted, with its ID
    std::vector<std::int32_t> uncountedIds;
    std::vector<CutNodeForce> nodes; // a cut: the force on each grid point, when asked
};

// An element of the side of a cut that has a grid point on the cut.
struct CutSideElement
{
    std::int32_t id = 0;
    std::string card;
    bool damper = false; // carries no load in a static subcase
};

// A cut through the structure: the grid points it passes through and the elements of the side whose loads are summed
// (model IDs, sorted). For a plane the cut is also given by the plane, which the integration of the forces needs.
struct CutDefinition
{
    std::vector<std::int32_t> nodes;
    std::vector<std::int32_t> sideElements;
    std::vector<std::int32_t> interiorNodes;       // the grid points of the side elements that are not on the cut
    std::vector<CutSideElement> sideElementsAtCut; // they carry the load across (no mass, no plot element)
    std::shared_ptr<const std::vector<std::int32_t>> elementNodes; // the grid points some element uses, sorted
    std::array<double, 3> centroid = {0.0, 0.0, 0.0};              // of the cut nodes
    bool isPlane = false;
    // A cut given by its grid points and the elements of its side (`nodes`, `sideElements`): the section of a member,
    // the rows of the side elements at the cut grid points, the multiPointConstrForce rows of a rigid element among the
    // side elements included; a rigid element that is not a side element, or an MPC equation, at the other grid points
    // of the side is not part of it (a companion package and a companion package, 2026-10-13)
    bool memberSection = false;
    std::array<double, 3> planePoint = {0.0, 0.0, 0.0};
    std::array<double, 3> planeNormal = {0.0, 0.0, 1.0}; // unit, from the side into the rest
};

// What a query of a resultant asks of its providers: the point the moment is taken about (basic system) and the nodes
// or elements the sum is limited to.
struct ResultantRequest
{
    std::array<double, 3> point = {0.0, 0.0, 0.0};
    std::optional<std::vector<std::int64_t>> nodeIds;
    std::optional<std::vector<std::int64_t>> elementIds; // freebody and section cuts: the elements of one side
    std::shared_ptr<const CutDefinition> cut;            // freebody and section cuts: the cut
    std::string method = "auto";                         // section cuts: "auto", "nodePointForces" or "integration"
    std::shared_ptr<CutSession> session;                 // cuts: what the cuts of the call share (made when null)
    bool nodeDetail = false;                             // cuts: give the force on each grid point too
};

// The resultants the providers of a quantity gave. A resultant of a step that an earlier provider gave is dropped.
class ResultantSet
{
public:
    void beginProvider(std::int16_t index, const ProviderInfo& info);
    [[nodiscard]] std::int64_t rowCount() const noexcept { return static_cast<std::int64_t>(m_rows.size()); }
    // True when a resultant was added (the step had none).
    bool add(ResultantRow&& row);
    [[nodiscard]] bool covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle, double step) const;
    [[nodiscard]] const std::vector<ResultantRow>& rows() const noexcept { return m_rows; }
    [[nodiscard]] std::vector<ResultantRow>& mutableRows() noexcept { return m_rows; }
    [[nodiscard]] const std::vector<ProviderInfo>& providers() const noexcept { return m_providers; }
    [[nodiscard]] std::int16_t currentProvider() const noexcept { return m_current; }

private:
    using Key = std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t>;
    std::vector<ResultantRow> m_rows;
    std::vector<Key> m_keys; // sorted (the rows mostly come in their order: added at the end)
    std::vector<ProviderInfo> m_providers;
    std::int16_t m_current = 0;
};

// The forces and moments at the grid points of one step of one table, in the basic system.
struct StepForces
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    double step = std::nan("");
    double eigenvalue = std::nan("");
    Precision precision = Precision::Double;
    std::vector<std::int32_t> node;
    std::vector<std::array<double, 6>> force;
};

// The forces of a nodal table (SPC forces, applied loads) of one table, a step each, in the basic system (the context
// turned them), the nodes `nodes` names only. Complex tables give nothing.
[[nodiscard]] std::vector<StepForces> nodalForcesOf(const ResultQueryContext& context, const TableRef& ref,
                                                    const std::optional<std::vector<std::int64_t>>& nodes);

// The same from a grid point force table: the rows whose `elementName` label is `label` ("singlePointConstrForce",
// "appliedLoad"), or, with an empty label, the rows of the elements in `elements` (what those elements push on the grid
// points). The nodes are those of `nodes`.
[[nodiscard]] std::vector<StepForces> nodePointForcesOf(const ResultQueryContext& context, const TableRef& ref,
                                                        const std::string& label,
                                                        const std::optional<std::vector<std::int64_t>>& elements,
                                                        const std::optional<std::vector<std::int64_t>>& nodes);

// The resultant of the forces of a step about `point`: the sum of the forces and of the moments (the moments at the
// nodes and the forces times their arms). The moment is NaN when a node has no position (no model, or the node is not
// linked).
[[nodiscard]] ResultantRow resultantOf(const StepForces& forces, const std::array<double, 3>& point,
                                       ResultNodePositions& positions);

// The columns of a resultant row turned to the axes of the coordinate system `system` (0: the basic system).
[[nodiscard]] std::array<double, 6> resultantInSystem(const ResultQueryContext& context, std::size_t set,
                                                      const std::array<double, 6>& basic, std::int64_t system);

using ResultantProvider = QuantityProvider<ResultantRequest, ResultantSet>;
using ResultantChain = ProviderChain<ResultantRequest, ResultantSet>;

// Adds the rows of a resultant set to an accumulator, in the order design cycle, result set, subcase, step, with the
// columns `subcase`, the step, `fx fy fz mx my mz` (along the axes of `system`), `pointX pointY pointZ`,
// `nodeCount` and `source`. `providers` maps the providers of the set to those of the accumulator (made by
// `addProvider`).
void addResultantRows(const ResultQueryContext& context, const ResultantSet& rows, const ResultantRequest& request,
                      std::int64_t system, RowAccumulator& accumulator, const std::vector<std::int16_t>& providers,
                      const std::string& countColumn = "nodeCount", const std::string& elementCountColumn = {});

// The providers of a set registered with an accumulator, in order: the indexes to give to `addResultantRows`.
[[nodiscard]] std::vector<std::int16_t> registerProviders(const ResultantSet& rows, RowAccumulator& accumulator);

// The chains of the resultants: reactions, applied loads, and the loads across a cut (the cuts are made by
// CutGeometry).
[[nodiscard]] const ResultantChain& reactionChain();
[[nodiscard]] const ResultantChain& appliedLoadChain();
[[nodiscard]] const ResultantChain& cutChain();

} // namespace oor::query
