#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace oor::analysis
{

// Thin-walled cross-sections made of straight walls (a companion package and R7), in the (y, z) coordinates of the section
// (y vertical, z horizontal, as the format reference figures of PBARL draw them). A wall joins two points; the walls form a
// connected graph, whose independent loops are the closed cells (as many as walls - points + 1). Every quantity is that
// of the thin-walled theory on the mid-line of the walls (Timoshenko and Gere, Megson), except the second moments of
// area, which take each wall as the rectangle of its length and thickness (so a single wall is a flat bar).
struct ThinWallProperties
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();

    double area = UNDEFINED;
    std::array<double, 2> centroid{UNDEFINED, UNDEFINED}; // (y, z)
    double i1 = UNDEFINED;                                // the integral of (y - yc)^2 about the centroid
    double i2 = UNDEFINED;                                // of (z - zc)^2
    double i12 = UNDEFINED;                               // of (y - yc)(z - zc)
    double iMax = UNDEFINED;                              // the principal second moments
    double iMin = UNDEFINED;
    double principalAngle = UNDEFINED; // degrees from the y axis towards z to the axis of iMax (of the y' integral)
    double j = UNDEFINED;              // Saint-Venant torsional constant
    double cw = UNDEFINED;             // warping constant about the shear centre
    std::array<double, 2> shearCentre{UNDEFINED, UNDEFINED}; // (y, z)
    std::size_t cells = 0;
};

// The section of the walls `segments` (pairs of point indexes) between `points` ((y, z) each), of thickness
// `thickness` and modulus ratio `ratio` (empty: 1; the modulus of a wall over the reference modulus: every property is
// weighted by it, those of torsion as by the shear modulus, the walls taken to share their Poisson ratio).
//   area, centroid, second moments: the sums over the walls (each a rectangle on its mid-line);
//   j: open walls (in no cell) n L t^3 / 3; closed cells by Bredt and Batho: the loop flows of a unit rate of twist
//   from
//      the compatibility of every cell, j = the sum of 2 A q over the cells, plus the open walls;
//   shear centre and cw: the warping function of the mid-line (dw/ds = q / (n t) - r, r the distance of the pole to the
//      tangent, q the flow of the cells under a unit twist), the shear centre the pole whose warping function has no
//      product with y nor z (Trefftz; the shear centre of the thin-walled theory, open or closed), cw the integral of
//      its square, its mean removed.
// std::invalid_argument for a point index out of range, a wall of zero length or thickness not above zero, a graph that
// is not connected.
[[nodiscard]] ThinWallProperties thinWallSection(std::span<const std::array<double, 2>> points,
                                                 std::span<const std::array<std::int64_t, 2>> segments,
                                                 std::span<const double> thickness, std::span<const double> ratio);

// The second derivatives of the properties of thinWallSection with respect to its inputs: the y and
// z of each point, then the thickness of each wall (2 P + S inputs). `fields[f]` is the symmetric matrix (inputs x
// inputs, row by row) of the field f, in the order area, centroid y, centroid z, i1, i2, i12, iMax, iMin,
// principalAngle, j, cw, shear centre y, shear centre z. Exact to rounding: forward over forward through the same
// operations as the values, by blocks of inputs. Not finite where a property is not twice differentiable (iMax and
// iMin of equal principal moments). The same exceptions as thinWallSection.
struct ThinWallHessian
{
    static constexpr std::size_t FIELDS = 13;
    std::size_t inputs = 0;
    std::array<std::vector<double>, FIELDS> fields;
};

[[nodiscard]] ThinWallHessian thinWallSectionHessian(std::span<const std::array<double, 2>> points,
                                                     std::span<const std::array<std::int64_t, 2>> segments,
                                                     std::span<const double> thickness, std::span<const double> ratio);

// The shear flows of an idealised section: booms (concentrated areas carrying the direct
// stress) at (y, z), and walls between two booms that carry only shear, of thickness t (Megson's idealisation; a
// boom of zero area is a joint). The walls form one or more closed cells, or none (an open section: a tree of the
// booms). The geometry is factorised once; each load case (vy along y and vz along z through `reference`, the torque
// about the axis x through `reference`, positive by the right-hand rule about x = y x z) gives the flow of every wall,
// positive from its first boom to its second, and the rate of twist times the shear modulus (G theta'). Closed: the
// open-section flows from the equilibrium of each boom, the closing flows from the compatibility of twist of each cell,
// and the moment about the reference point. Open: the flows from the equilibrium of each boom, zero at the free end of
// each branch, which pass through the shear centre; the moment of the loads about the shear centre is carried by the
// Saint-Venant torsion of the walls alone (G theta' = T / J, J = sum L t^3 / 3), with no flow along the walls.
class ShearFlowSection
{
public:
    // `cells`: the loops of booms around each cell, in order (a wall between each pair of consecutive booms and between
    // the last and the first); empty: the independent loops of the graph of the walls (none for an open section).
    // std::invalid_argument for walls that do not connect every boom, a boom index out of range, booms whose
    // direct-stress areas give no second moment, or a loop that is not a closed chain of walls.
    ShearFlowSection(std::span<const std::array<double, 2>> booms, std::span<const double> areas,
                     std::span<const std::array<std::int64_t, 2>> walls, std::span<const double> thickness,
                     const std::vector<std::vector<std::int64_t>>& cells, std::array<double, 2> reference);

    [[nodiscard]] std::size_t wallCount() const noexcept { return m_walls; }
    [[nodiscard]] std::size_t cellCount() const noexcept { return m_cells; }
    [[nodiscard]] std::array<double, 2> centroid() const noexcept { return m_centroid; }
    // The point a shear force passes through to give no twist (y, z).
    [[nodiscard]] std::array<double, 2> shearCentre() const noexcept { return m_shearCentre; }

    // The flows of the walls (`flows`, wallCount values) and G theta' for one load case.
    void solve(double vy, double vz, double torque, double* flows, double& twist) const;

private:
    // The flows and the twist of the unit loads of an open section (no cell); `moments` Iyy, Izz, Iyz of the booms.
    void solveOpen(std::span<const std::array<double, 2>> booms, std::span<const double> areas,
                   std::span<const std::array<std::int64_t, 2>> walls, std::span<const double> thickness,
                   const std::vector<double>& length, std::array<double, 2> reference,
                   const std::array<double, 3>& moments);

    std::size_t m_walls = 0;
    std::size_t m_cells = 0;
    std::array<double, 2> m_centroid{};
    std::array<double, 2> m_shearCentre{};
    // the solutions of unit vy, vz and torque: flows then G theta', wallCount + 1 values each
    std::array<std::vector<double>, 3> m_unit;
};

} // namespace oor::analysis
