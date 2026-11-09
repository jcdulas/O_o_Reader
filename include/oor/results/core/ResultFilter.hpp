#pragma once

#include "oor/results/core/ResultLayout.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oor::results
{

// The steps between two values, both included; with a `kind`, only the tables whose steps are of that kind (the times
// of a transient analysis, the frequencies of a frequency response, ...).
struct StepRange
{
    double lower = 0.0;
    double upper = 0.0;
    std::optional<StepKind> kind;
};

// A Python-style slice of the step positions.
struct IndexSlice
{
    std::optional<std::int64_t> start;
    std::optional<std::int64_t> stop;
    std::int64_t step = 1;
};

// What to keep of the results, at reading time (the data that are not asked for are never decoded) and afterwards
// (`ResultTable::select`). Every member that is set restricts; the ones that are not set keep everything; all the set
// ones must hold together.
//
//   subcases      the subcases to keep
//   tables        the kinds of tables ("displacements") or the names the tables were given, in any case
//   ids           the grid point IDs to keep (nodal tables; element tables ignore it); an ID the table does not have is
//                 ignored
//   elementIds    the element IDs to keep (element tables; nodal tables ignore it); an ID the table does not have is
//                 ignored
//   layers        the layers to keep (element tables), numbered from 1: 1 and 2 are the fibres Z1 and Z2 of a shell, a
//                 ply of a laminate, the station of a beam; a table that has no such layer keeps nothing
//   designCycles  the design cycles to keep (SOL 200 files); a table without a design cycle is kept only when the
//                 filter has no such member
//   variants      the result variants to keep: "psd", "rms", "modalContribution", ... ("" or "plain" for the results
//   of
//                 an analysis themselves); without it every variant is kept
//   components    the components to keep ("t1", "r3"), in the order given; a name the table does not have is ignored
//   steps         the steps to keep, by value (a mode number, a time, a frequency, a load factor), to the precision
//                 of the file (single precision in a 32-bit file)
//   stepRange     the steps between two values
//   stepIndices   the steps by position in the table (from 0; negative from the end)
//   stepSlice     the steps by a slice of positions
//
// The rows (IDs) and the steps keep the order of the file.
struct ResultFilter
{
    std::optional<std::vector<std::int32_t>> subcases;
    std::optional<std::vector<std::string>> tables;
    std::optional<std::vector<std::int32_t>> ids;
    std::optional<std::vector<std::int32_t>> elementIds;
    std::optional<std::vector<std::int32_t>> layers;
    std::optional<std::vector<std::int32_t>> designCycles;
    std::optional<std::vector<std::string>> variants;
    std::optional<std::vector<std::string>> components;
    std::optional<std::vector<double>> steps;
    std::optional<StepRange> stepRange;
    std::optional<std::vector<std::int64_t>> stepIndices;
    std::optional<IndexSlice> stepSlice;

    [[nodiscard]] bool hasStepSelection() const noexcept { return steps || stepRange || stepIndices || stepSlice; }
    // Something that a table applies while it decodes: IDs, layers, or (SORT2 tables) steps.
    [[nodiscard]] bool hasRowSelection() const noexcept { return ids || elementIds || layers; }
    // Nothing is set: the filter keeps everything.
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return !subcases && !tables && !ids && !elementIds && !layers && !designCycles && !variants && !components &&
               !hasStepSelection();
    }
    // The members that are set, as text, for messages ("subcases [1, 2], ids [5]").
    [[nodiscard]] std::string describe() const;
};

// The positions (increasing) of the steps that the filter keeps among `steps`, whose kind is `kind`. Only the step
// members of the filter are looked at.
[[nodiscard]] std::vector<std::uint32_t> selectStepPositions(const ResultFilter& filter, std::span<const double> steps,
                                                             StepKind kind);

// The positions (increasing) of the IDs of `ids` that appear in `wanted`.
[[nodiscard]] std::vector<std::uint32_t> selectIdPositions(std::span<const std::int32_t> wanted,
                                                           std::span<const std::int32_t> ids);

// The positions, in the order of `wanted`, of the components of `names` that `wanted` names (any case; each once).
[[nodiscard]] std::vector<std::uint32_t> selectComponentPositions(std::span<const std::string> wanted,
                                                                  std::span<const std::string> names);

// True when two steps are the same number to the precision of a 32-bit file.
[[nodiscard]] bool sameStepValue(double left, double right) noexcept;

} // namespace oor::results
