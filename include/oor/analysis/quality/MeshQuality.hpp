#pragma once

#include "oor/analysis/quality/QualityMetrics.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>

namespace oor::analysis
{

// The quality of the shell elements (CTRIA3, CTRIAR, CTRIA6, CQUAD4, CQUADR, CQUAD8, CQUAD, CSHEAR, the plane strain,
// plane stress and axisymmetric triangles and quadrilaterals): aspect ratio, skew, warp, taper, interior angles,
// Jacobian, edge lengths, mid-side node position. A triangle and a quadrilateral each have a function of their own
// (src/analysis/quality/TriangleQuality.cpp, QuadQuality.cpp).
//
// The nodes come from the cache of positions in the basic system, so the metrics are right whatever the CP of the grid
// points. A node that does not exist raises GeometryError naming the element. A card type that is not a shell gives NaN
// everywhere.
class ShellQuality
{
public:
    [[nodiscard]] static bool supports(std::uint16_t typeIndex) noexcept;

    // The quality of the elements in `rows` of the table of `typeIndex` (live rows), over the pool when the call is
    // large.
    [[nodiscard]] static QualityArrays compute(const model::Model& model, std::uint16_t typeIndex,
                                               std::span<const std::uint32_t> rows, base::ThreadPool* pool = nullptr);
    [[nodiscard]] static QualityValues of(const model::Model& model, model::CardLocation location);
};

// The quality of the solid elements: CTETRA (4 or 10 nodes), CPENTA (6 or 15), CHEXA (8 or 20) and CPYRAM (5 or 13),
// each with a function of its own (TetraQuality.cpp, PentaQuality.cpp, HexaQuality.cpp, PyramidQuality.cpp). Same
// conventions as ShellQuality.
class SolidQuality
{
public:
    [[nodiscard]] static bool supports(std::uint16_t typeIndex) noexcept;

    [[nodiscard]] static QualityArrays compute(const model::Model& model, std::uint16_t typeIndex,
                                               std::span<const std::uint32_t> rows, base::ThreadPool* pool = nullptr);
    [[nodiscard]] static QualityValues of(const model::Model& model, model::CardLocation location);
};

// Shells and solids behind one door: the quality of any element that has some, NaN for the others (bars, springs,
// masses).
class MeshQuality
{
public:
    [[nodiscard]] static bool supports(std::uint16_t typeIndex) noexcept
    {
        return ShellQuality::supports(typeIndex) || SolidQuality::supports(typeIndex);
    }
    [[nodiscard]] static QualityArrays compute(const model::Model& model, std::uint16_t typeIndex,
                                               std::span<const std::uint32_t> rows, base::ThreadPool* pool = nullptr);
    [[nodiscard]] static QualityValues of(const model::Model& model, model::CardLocation location);
};

} // namespace oor::analysis
