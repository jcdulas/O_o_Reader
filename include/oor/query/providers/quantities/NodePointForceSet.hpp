#pragma once

#include "oor/query/results/ResultSets.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace oor::query
{

// What a row of the grid point force balance is: a row for each element at the grid point, then the applied load, the
// forces of single point and multipoint constraints, and their sum, which is zero at equilibrium. The labels are the
// neutral names the O_o_Solver module writes in the `elementName` texts of its grid point force tables.
enum class NodePointForceKind : std::uint8_t
{
    Element,     // the force an element pushes on the grid point (the label is the result element type, shellQuad4)
    AppliedLoad, // appliedLoad
    Spc,         // singlePointConstrForce
    Mpc,         // multiPointConstrForce: the rigid elements and MPC equations at the grid point, lumped, or one of
                 // them
    Total,       // total
    Other        // any other label
};

// The rows of a grid point force table linked to the grid points and elements of the attached model. The tables of the
// subcases of a result set mostly have the same rows: they share one.
struct NodePointForceLayout
{
    std::vector<std::int32_t> node;    // the grid point of the model (0: not linked)
    std::vector<std::int32_t> element; // the element of the model (0: not an element row, or not linked)
    std::vector<NodePointForceKind> kind;
    // The relation of a multiPointConstrForce row split by relation: the ID its elementIds gives, the element ID
    // of a rigid element or minus the set ID of an MPC equation; 0 for the row of every relation at the grid point
    // (the solver's). Empty when no row of the table is split.
    std::vector<std::int32_t> relation;
    std::unordered_map<std::int32_t, std::vector<std::uint32_t>> rowsOfNode;
    std::set<std::int32_t> elements; // the elements that have a row

    // The relation of row `row` (0: not a split multiPointConstrForce row).
    [[nodiscard]] std::int32_t relationOf(std::uint32_t row) const noexcept
    {
        return relation.empty() ? 0 : relation[row];
    }

    // The rows at a grid point of the model (empty when it has none).
    [[nodiscard]] std::span<const std::uint32_t> rowsAt(std::int32_t modelNode) const noexcept;
};

// The grid point force table of one subcase of one result set, decoded once, its values turned to the basic system and
// its rows linked to the grid points and elements of the attached model: what every cut of a call reads.
struct NodePointForceTable
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    std::vector<double> steps;       // the value of each step; NaN for a static subcase
    std::vector<double> eigenvalues; // NaN where there is none
    Precision precision = Precision::Double;
    bool isComplex = false;
    bool magnitudePhase = false; // the file stored magnitude and phase (the values here are real and imaginary)
    std::size_t rowCount = 0;
    std::shared_ptr<const NodePointForceLayout> layout; // the rows
    std::vector<double> real;                           // (steps, rows, 6): f1 f2 f3 m1 m2 m3 in the basic system
    std::vector<double> imaginary;                      // the same, empty for a real table

    [[nodiscard]] std::size_t stepCount() const noexcept { return steps.size(); }
    [[nodiscard]] bool isStatic() const noexcept { return stepKind == results::StepKind::None; }
    [[nodiscard]] double realAt(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return real[(step * rowCount + row) * 6 + component];
    }
    [[nodiscard]] double imaginaryAt(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return imaginary.empty() ? 0.0 : imaginary[(step * rowCount + row) * 6 + component];
    }
    [[nodiscard]] std::span<const std::uint32_t> rowsAt(std::int32_t modelNode) const noexcept
    {
        return layout->rowsAt(modelNode);
    }
};

// Every grid point force table the call selects, decoded once (on the threads of the call). `sectionCuts` evaluates a
// hundred cuts on one set of tables; `freebody`, `sectionCut` and the computed monitor points read them the same way.
class NodePointForceSet
{
public:
    // The tables of the call (after its subcase, step and analysis selection), in the order of `sortTables`. A table
    // without the six components of a grid point force is left out.
    [[nodiscard]] static std::shared_ptr<const NodePointForceSet> read(const ResultQueryContext& context);

    [[nodiscard]] const std::vector<NodePointForceTable>& tables() const noexcept { return m_tables; }
    [[nodiscard]] bool empty() const noexcept { return m_tables.empty(); }

private:
    std::vector<NodePointForceTable> m_tables;
};

// The kind of a row of the grid point force balance from its label.
[[nodiscard]] NodePointForceKind nodePointForceKindOf(std::string_view label) noexcept;

} // namespace oor::query
