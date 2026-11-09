#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace oor::analysis
{

// A rectangle of a cross-section: y0 <= y <= y1 (vertical) and z0 <= z <= z1 (horizontal), in the coordinates of the
// format reference figures of the beam library.
struct SectionRectangle
{
    double y0 = 0.0;
    double y1 = 0.0;
    double z0 = 0.0;
    double z1 = 0.0;
};

// The shear factors and the torsional constant of a cross-section; NaN when they cannot be computed.
struct SectionStiffness
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();

    double k1 = UNDEFINED; // shear along the element y axis (plane 1)
    double k2 = UNDEFINED; // shear along the element z axis (plane 2)
    double j = UNDEFINED;
    // The shear centre (y, z) in the coordinates of the section, and the warping constant about it.
    double shearY = UNDEFINED;
    double shearZ = UNDEFINED;
    double cw = UNDEFINED;
};

// The plan of the two meshes of a section made of rectangles: the cells of each band (the bands between consecutive
// distinct edges, along y then along z; the coarse mesh and the fine one) and the break of each edge of each piece.
// Made from the section's own edges, or from a reference section: the meshes of a plan have its
// topology whatever the dimensions, their nodes following the edges of the pieces (two edges that met in the reference
// may move apart: the nodes of each piece follow their own), so that the properties are smooth functions of the
// dimensions (rational) as long as the meshes can be made: no cell turned over, no node on two edges that moved apart.
struct SectionMeshPlan
{
    std::vector<std::size_t> coarseY;
    std::vector<std::size_t> coarseZ;
    std::vector<std::size_t> fineY;
    std::vector<std::size_t> fineZ;
    // The graded bands (a section whose walls are of very different thicknesses): for each band, the lines of its
    // coarse cells strictly inside it as fractions of the band, increasing (empty for cells of one length, and both
    // empty for a plan with no graded band); the fine mesh halves each coarse cell of a graded band.
    std::vector<std::vector<double>> cutsY;
    std::vector<std::vector<double>> cutsZ;
    // the breaks of each edge of each piece: y0, y1 (along y), z0, z1 (along z)
    std::vector<std::array<std::uint32_t, 4>> edges;
    [[nodiscard]] bool empty() const noexcept { return coarseY.empty(); }
};

// The derivatives of a SectionStiffness along `count` directions of its geometry (a direction is the derivative of
// every edge, or of the inset, width and height of the hexagon, with respect to one dimension): those of the discrete
// solution on the meshes of the plan, exact to rounding (forward-mode derivatives of the element integrals, the
// derivatives of the solutions from the factorised matrix). NaN for a direction along which the meshes of the plan
// cannot be made (a wall end that would leave the middle of another wall).
inline constexpr std::size_t MAX_SECTION_DIRECTIONS = 12;
struct SectionStiffnessGradient
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();
    std::size_t count = 0;
    std::array<double, MAX_SECTION_DIRECTIONS> k1{};
    std::array<double, MAX_SECTION_DIRECTIONS> k2{};
    std::array<double, MAX_SECTION_DIRECTIONS> j{};
    std::array<double, MAX_SECTION_DIRECTIONS> shearY{};
    std::array<double, MAX_SECTION_DIRECTIONS> shearZ{};
    std::array<double, MAX_SECTION_DIRECTIONS> cw{};
};

// The second derivatives of a SectionStiffness along pairs of the directions of its geometry, entry
// i * MAX_SECTION_DIRECTIONS + j for the directions i and j, symmetric: those of the discrete solution on the meshes of
// the plan, exact to rounding (the nodes are affine along the directions; the solutions and the adjoints are
// differentiated through the factorised matrix, five solves a direction, then the derivatives of the element integrals
// forward over forward). NaN along a direction whose meshes cannot be made.
struct SectionStiffnessHessian
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();
    using Matrix = std::array<double, MAX_SECTION_DIRECTIONS * MAX_SECTION_DIRECTIONS>;
    std::size_t count = 0;
    Matrix k1{};
    Matrix k2{};
    Matrix j{};
    Matrix shearY{};
    Matrix shearZ{};
    Matrix cw{};
};

// The shear factors and the torsional constant of a section of the PBARL / PBEAML library as the solver computes them
// with its finite element formulation of the cross-section (the sections of the standard library are computed as
// arbitrary sections).
//
// The section is meshed with bilinear quadrilaterals and three Laplace problems with a zero normal derivative on the
// whole boundary are solved on it:
//   - shear in plane 1: -lap g = y - yc; K1 = I1^2 / (A integral of (y - yc) g dA), I1 the integral of (y - yc)^2;
//   - shear in plane 2: the same with z - zc and I2, which gives K2;
//   - torsion: lap w = 0 with dw/dn = (z - zc) ny - (y - yc) nz, the Saint-Venant warping function;
//     J = Ip - integral of ((z - zc) dw/dy - (y - yc) dw/dz) dA, Ip = I1 + I2;
//   - the shear centre (Trefftz): the pole (a, b) about which the warping function w + a (z - zc) - b (y - yc) has no
//     product with y - yc nor z - zc (the pole of the thin-walled theory; no Poisson effect), and Cw the integral of
//     the square of that warping function, its mean removed.
// K is the energy-consistent shear factor of the Saint-Venant flexure solution with no Poisson effect: no material
// enters it (the solver's records equal these values whatever the Poisson ratio). The mesh is aligned with every edge
// of the rectangles, CELLS_PER_THICKNESS cells across the thinnest one and cells at most twice as long along the walls;
// the problem is solved on that mesh and on the mesh twice as fine, and the two results are extrapolated (Richardson,
// the error of the energies falling as h^(4/3) where the section has a re-entrant corner, at which the solutions are
// singular, as h^2 elsewhere). The error left is below 6e-4 against values converged on meshes refined at the corners
// (2026-10-22: at most 3.1e-4 of J, 1.4e-4 of K1 and K2, 2.2e-4 of Cw over I, T, T2, Z, L, CHAN, CHAN1, BOX, BOX1, HAT
// and CROSS, 4.4e-4 of the Cw of a rectangle; the extrapolation as h^2 everywhere of before left up to 2.5e-3 of J,
// 1.5e-3 of K, 1.9e-3 of Cw on the sections with re-entrant corners, at the same cost). A section whose
// coarse mesh would have more than 2 500 cells (walls of very different thicknesses: a thin web cut every wall to its
// own cells; walls many times longer than thick) is meshed with graded cells instead (a companion package D-577):
// across a wall the cells of the uniform mesh of that wall alone, along it cells as short only at the corners of the
// section and growing away from them by a quarter of the distance, a thick wall not cut below an eighth of its own
// cells; the cost then follows the number of walls, not how thin they are (an I whose web is 19 to 1 000 times thinner
// than its flanges: 800 to 1 000 cells, against 15 000), within the same error (1.1e-4 of J, 2e-4 of K on such an
// I, 4.4e-4 of K on one whose flanges are 100 times thinner than its web; 4.6e-4 of K2 on one whose flanges are 20
// times thinner), the two graded meshes extrapolated as h^2 (their cells are short at the corners). The sections of the
// validation decks and the reference models keep their uniform meshes.
class SectionShearSolver
{
public:
    // A section made of rectangles that touch along their edges and do not overlap.
    [[nodiscard]] static SectionStiffness ofRectangles(std::span<const SectionRectangle> pieces);
    // The plan of the meshes of a section made of rectangles (the cells of each band, from its own edges).
    [[nodiscard]] static SectionMeshPlan planOf(std::span<const SectionRectangle> pieces);
    // The same on the meshes of `plan` (empty, or not of the same bands as the pieces: their own plan), and with
    // `gradient`, the derivatives along `directions` (each the derivative of every piece, in the order of the pieces).
    [[nodiscard]] static SectionStiffness ofRectangles(std::span<const SectionRectangle> pieces,
                                                       const SectionMeshPlan& plan,
                                                       std::span<const std::vector<SectionRectangle>> directions,
                                                       SectionStiffnessGradient* gradient);
    // The same with, when `hessian` is given (and `gradient`), the second derivatives along pairs of the directions
    // (each affine in the dimensions it follows).
    [[nodiscard]] static SectionStiffness ofRectangles(std::span<const SectionRectangle> pieces,
                                                       const SectionMeshPlan& plan,
                                                       std::span<const std::vector<SectionRectangle>> directions,
                                                       SectionStiffnessGradient* gradient,
                                                       SectionStiffnessHessian* hessian);
    // The HEXA section: a hexagon `width` wide and `height` high whose flat top and bottom are inset by `inset` at each
    // end, centred on the origin. Its mesh does not depend on the dimensions.
    [[nodiscard]] static SectionStiffness ofHexagon(double inset, double width, double height);
    // The same with the derivatives along `directions` (each the derivatives of inset, width and height).
    [[nodiscard]] static SectionStiffness ofHexagon(double inset, double width, double height,
                                                    std::span<const std::array<double, 3>> directions,
                                                    SectionStiffnessGradient* gradient);
    // The same with the second derivatives (when `hessian` and `gradient` are given).
    [[nodiscard]] static SectionStiffness ofHexagon(double inset, double width, double height,
                                                    std::span<const std::array<double, 3>> directions,
                                                    SectionStiffnessGradient* gradient,
                                                    SectionStiffnessHessian* hessian);
};

} // namespace oor::analysis
