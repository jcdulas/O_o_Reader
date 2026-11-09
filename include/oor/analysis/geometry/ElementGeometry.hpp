#pragma once

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace oor::analysis
{

// A quantity that is not defined for an element (the normal of a rod, the volume of a spring, the thickness of a bar)
// is NaN: the same in a single value and in an array, so that a mixed selection gives one array and the undefined
// entries are the NaN ones.
inline constexpr double NOT_DEFINED = std::numeric_limits<double>::quiet_NaN();

[[nodiscard]] inline base::Vec3 undefinedVector() noexcept
{
    return base::Vec3{NOT_DEFINED, NOT_DEFINED, NOT_DEFINED};
}
[[nodiscard]] inline base::Mat3 undefinedFrame() noexcept
{
    base::Mat3 frame;
    frame.values.fill(NOT_DEFINED);
    return frame;
}
[[nodiscard]] inline bool isDefined(const base::Vec3& value) noexcept
{
    return value.x == value.x && value.y == value.y && value.z == value.z;
}

// What can be asked of an element. The values are bits, so that a call says which ones it wants and the others are not
// computed.
enum class Quantity : std::uint8_t
{
    Center = 1,
    Normal = 2,
    Area = 4,
    Volume = 8,
    Length = 16,
    Thickness = 32,
    Frame = 64,
    MaterialFrame = 128
};

[[nodiscard]] constexpr std::uint8_t bit(Quantity quantity) noexcept
{
    return static_cast<std::uint8_t>(quantity);
}
inline constexpr std::uint8_t ALL_QUANTITIES = 255;

// The geometry of one element, in the basic system. Undefined values are NaN.
//
//   center     the average of the corner grid points (a shell, a solid), the middle of the line (a bar, a rod, a
//   spring), the
//              point where the mass is (a point mass), the average of the dependent grid points (a rigid element)
//   normal     the unit normal of a shell, from the right-hand rule on its connection order
//   area       the surface of a shell; the cross-section of a rod or bar when its property gives one
//   volume     a solid (the isoparametric volume, mid-side nodes included), a shell (area times thickness), a rod or
//   bar
//              (cross-section times length)
//   length     a line element, between its end points (offsets included)
//   thickness  a shell: the corner thicknesses of the element if any, else the property's, or the total of a laminate
//   frame      the element coordinate system: the columns are its x, y and z axes in the basic system
//   materialFrame  the material coordinate system of a shell: the x axis is the material direction
struct ElementValues
{
    base::Vec3 center = undefinedVector();
    base::Vec3 normal = undefinedVector();
    double area = NOT_DEFINED;
    double volume = NOT_DEFINED;
    double length = NOT_DEFINED;
    double thickness = NOT_DEFINED;
    base::Mat3 frame = undefinedFrame();
    base::Mat3 materialFrame = undefinedFrame();
};

// The geometry of many elements of one card type: one entry per element, in the order of the rows asked for. Only the
// arrays of the quantities asked for are filled.
struct ElementArrays
{
    std::vector<base::Vec3> centers;
    std::vector<base::Vec3> normals;
    std::vector<double> areas;
    std::vector<double> volumes;
    std::vector<double> lengths;
    std::vector<double> thicknesses;
    std::vector<base::Mat3> frames;
    std::vector<base::Mat3> materialFrames;
};

// Geometry of the elements of a model: shells, solids, bars, rods, springs, dampers, bushes, point masses and rigid
// elements, with the mid-side nodes of the higher-order ones. Node positions come from BasicPositionCache (the basic
// system, through CP), so every value is in the basic system whatever the coordinate systems of the grid points.
//
// A node that does not exist (or is not a grid point where one is needed) raises GeometryError naming the element; a
// scalar point where a line element has a grid point gives NaN.
class ElementGeometry
{
public:
    // True when the card type has geometry (an element or a rigid element with grid points).
    [[nodiscard]] static bool supports(std::uint16_t typeIndex) noexcept;

    // The values of the elements in `rows` of the table of `typeIndex`, which must be live rows. `pool` spreads a large
    // call over threads. A card type with no geometry gives NaN everywhere.
    [[nodiscard]] static ElementArrays compute(const model::Model& model, std::uint16_t typeIndex,
                                               std::span<const std::uint32_t> rows, std::uint8_t quantities,
                                               base::ThreadPool* pool = nullptr);

    // The values of one element.
    [[nodiscard]] static ElementValues of(const model::Model& model, model::CardLocation location,
                                          std::uint8_t quantities = ALL_QUANTITIES);
};

} // namespace oor::analysis
