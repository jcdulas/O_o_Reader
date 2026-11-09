#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::analysis
{

// The quantities derived from the components of a stress or strain table.
enum class DerivedKind : std::uint8_t
{
    VonMises,  // the equivalent stress (plane stress for a shell or a ply, the six components for a solid); for a
               // strain the equivalent strain of the solver's tables
    Principal, // a shell or a ply: major, minor and the angle of the major axis from the x axis (degrees); a solid: o1
               // >= o2 >= o3
    MaxShear,  // shell, ply: half the difference of the principal stresses (the difference of the strains);
               // solid: the same with the extreme principal values
    OctahedralShear // solids: the octahedral shear stress (strain)
};

[[nodiscard]] std::string_view derivedKindName(DerivedKind kind) noexcept;

enum class Surface : std::uint8_t
{
    Z1,
    Z2,
    Middle // the mean of the two fibres
};

// The scalars of one stress or strain tensor, as the tables of the solver hold them: the equivalent (von Mises) value,
// the principal values (a shell: major, minor and the angle of the major axis from the x axis in degrees; a solid: the
// three, decreasing), the maximum shear and the octahedral shear (solids). NaN where the family has none.
struct TensorScalars
{
    double vonMises = 0.0;
    double first = 0.0;  // o1: the major principal value
    double second = 0.0; // o2: the minor one (a shell) or the middle one (a solid)
    double third = 0.0;  // o3 (a solid); NaN for a shell
    double angle = 0.0;  // a shell: the angle of the major axis, degrees; NaN for a solid
    double maxShear = 0.0;
    double octahedralShear = 0.0; // NaN for a shell
};

// Strided columns of the components xx yy zz xy yz xz of tensors: the component `c` of the row `r` is
// `data[c][r * stride[c]]`, a stride of 0 repeating one value over the rows (an argument broadcast to every row).
struct TensorColumns
{
    std::array<const double*, 6> data{};
    std::array<std::ptrdiff_t, 6> stride{};
};

// Quantities derived from the values a table holds: von Mises, principal values, shears, the
// surface of a shell, the layers, the extremes over layers and over steps. They are computed from the components
// stored (the stored von Mises or principal values are never trusted nor read: that is how they are checked), in C++
// over the rows, in parallel; the result is a new table marked as derived, never written to the file.
//
// Real tables only for the arithmetic (a complex stress has no von Mises stress): take `atPhase` or `magnitude` first.
class DerivedQuantities
{
public:
    // True when the table has the components of a stress or a strain tensor the quantities are made from.
    [[nodiscard]] static bool isTensorTable(const results::ResultTableMeta& meta) noexcept;

    // The scalars of one row: `components` are xx yy zz xy yz xz as the tables store them (the shears of a strain are
    // the engineering ones; the out-of-plane ones of a shell are ignored with `solid` false). The same arithmetic as
    // the tables of `compute`, which equal the values the solver stores.
    [[nodiscard]] static TensorScalars scalarsOf(bool strain, bool solid, const double components[6]);
    // The von Mises values of the rows `begin` to `end` of `columns`, into `out[0]` to `out[end - begin - 1]`: for each
    // row the `vonMises` of `scalarsOf` (the same arithmetic, so the same bits), without the other scalars. Only xx, yy
    // and xy are read when `solid` is false.
    static void vonMisesOf(bool strain, bool solid, const TensorColumns& columns, std::size_t begin, std::size_t end,
                           double* out) noexcept;

    // One derived quantity of every row, for every step. Components: "vonMises"; "major", "minor", "angle" (shells and
    // plies: "angle" in degrees) or "o1", "o2", "o3" ("e1"... for a strain); "maxShear"; "octahedralShear". Throws
    // UsageError for a table that is not a stress or strain table, a complex table, a missing component, or a quantity
    // the family does not have (the octahedral shear of a shell).
    [[nodiscard]] static std::shared_ptr<results::ResultTable> compute(const results::ResultTable& table,
                                                                       DerivedKind kind,
                                                                       base::ThreadPool* pool = nullptr,
                                                                       const results::DecodeContext& context = {});

    // The rows of one fibre of a shell, or the mean of both. Shell stress and strain tables of centre or corner rows
    // only.
    [[nodiscard]] static std::shared_ptr<results::ResultTable> surface(const results::ResultTable& table, Surface which,
                                                                       const results::DecodeContext& context = {});
    // The layers a table has (the fibres of a shell, the plies of a laminate, the stations of a beam), increasing.
    [[nodiscard]] static std::vector<std::int32_t> layersOf(const results::ResultTable& table,
                                                            const results::DecodeContext& context = {});
    // One table per layer.
    [[nodiscard]] static std::vector<std::pair<std::int32_t, std::shared_ptr<results::ResultTable>>> byLayer(
        const results::ResultTable& table, const results::DecodeContext& context = {});
    // The largest value of a quantity over the layers of each place (element and grid point) and the layer where it is:
    // components {quantity, "layer"}. `quantity` is a component of the table or "vonMises", "maxShear",
    // "octahedralShear", "major", "minor". The layer of a row of the result is 0 (it is all of them).
    [[nodiscard]] static std::shared_ptr<results::ResultTable> maxOverLayers(
        const results::ResultTable& table, const std::string& quantity, base::ThreadPool* pool = nullptr,
        const results::DecodeContext& context = {});
    // The extremes of components over the steps: for each component c, "cMax", "cMaxStep", "cMin", "cMinStep"
    // (the step value of the extreme), one step. Real tables only.
    [[nodiscard]] static std::shared_ptr<results::ResultTable> maxMin(const results::ResultTable& table,
                                                                      const std::vector<std::string>& components,
                                                                      const results::DecodeContext& context = {});
    // A complex table as real: the values at a phase (the real part of v times e^(i phase), phase in degrees), or the
    // magnitudes.
    [[nodiscard]] static std::shared_ptr<results::ResultTable> atPhase(const results::ResultTable& table,
                                                                       double phaseInDegrees,
                                                                       const results::DecodeContext& context = {});
    [[nodiscard]] static std::shared_ptr<results::ResultTable> magnitude(const results::ResultTable& table,
                                                                         const results::DecodeContext& context = {});
};

} // namespace oor::analysis
