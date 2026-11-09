#pragma once

#include "oor/query/named/results/ResultQuery.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// The values of one quantity at one place along the steps of one table: a frequency response, a time history.
struct Series
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    std::int32_t id = 0;
    results::StepKind stepKind = results::StepKind::None;
    std::vector<double> steps;
    std::vector<double> real;
    std::vector<double> imaginary; // empty for real values
    Precision precision = Precision::Single;
    bool magnitudePhase = false;

    [[nodiscard]] bool isComplex() const noexcept { return !imaginary.empty(); }
};

// The quantities a series query may name: "displacement", "velocity", "acceleration", "spcForce", "appliedLoad" and
// "temperature" (the components `t1` ... `r3`, or `temperature`), and the element quantities "stress", "strain" and
// "force" (the component is the column of the table: `oxx`, `vonMises`, `axial`, `fx`, ...; the row of an element is
// its centre at the first layer).
[[nodiscard]] bool isSeriesQuantity(const std::string& quantity) noexcept;

// The series of `quantity` and `component` at `ids` (all places when `ids` is null) in the tables whose steps are of
// `kind` (frequencies for a frequency response, times for a history), in the order of the tables, then of the places.
[[nodiscard]] std::vector<Series> collectSeries(const ResultQueryContext& context, const std::string& quantity,
                                                const std::string& component,
                                                const std::optional<std::vector<std::int64_t>>& ids,
                                                results::StepKind kind);

} // namespace oor::query
