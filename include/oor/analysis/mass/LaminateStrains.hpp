#pragma once

#include "oor/analysis/mass/Laminate.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace oor::analysis
{

// The ABD matrix of a laminate, 6 x 6 row by row: [A B; B D], for the resultants (Nx, Ny, Nxy, Mx, My, Mxy) and the
// mid-plane strains and curvatures (ex0, ey0, gxy0, kx, ky, kxy), engineering shears.
using AbdMatrix = std::array<double, 36>;

// The mid-plane strains and the curvatures of a laminate under its resultants: [e0; k] = [A B; B D]^-1 [N; M]
// (classical lamination theory: M is the integral of sigma z over the thickness, and the strain at the height z is
// e0 + z k; the bending moments of the solver's shell forces are the opposite, -M). The shears are engineering strains.
struct MidPlaneStrains
{
    std::array<double, 3> strain = {};    // ex0, ey0, gxy0
    std::array<double, 3> curvature = {}; // kx, ky, kxy
};

// The ABD matrix of a laminate stiffness.
[[nodiscard]] AbdMatrix abdOf(const LaminateStiffness& stiffness) noexcept;

// The inverse of an ABD matrix, as the strains need it. A matrix that cannot be inverted whole (a membrane-only
// property: B and D zero, or a bending-only one) is inverted by its blocks when B is zero: the part whose block is
// singular is NaN. All NaN when nothing can be inverted, or when the matrix holds a NaN (a SMEAR laminate).
[[nodiscard]] AbdMatrix invertAbd(const AbdMatrix& abd) noexcept;

// The strains of resultants by an inverse of ABD (invertAbd): [e0; k] = inverse [N; M].
[[nodiscard]] MidPlaneStrains strainsOf(const AbdMatrix& inverse, const std::array<double, 3>& n,
                                        const std::array<double, 3>& m) noexcept;

// The stiffness of a shell property as a laminate, about its reference plane: a PCOMP or a PCOMPG (Laminate::of), or a
// PSHELL from the laws of its materials, as the the solver reference manual writes them, in the moments of the
// lamination theory (+int sigma z dz, as the laminates): A = T G1, B = -T^2 G4 (the solver's plate moments are the
// opposite, so the coupling of the card turns sign: a companion package D-319, D-597), D = (12I/T^3) T^3 / 12 G2, G =
// (TS/T) T G3 (a part whose material is blank, or not positive, is zero). nullopt for another property or a missing
// one. Throws MassError for a material without a plane stress law.
[[nodiscard]] std::optional<LaminateStiffness> shellStiffness(const model::Model& model, model::CardLocation property);

// The normal strain along a direction of the plane at `angleInDegrees` from the x axis, of the strains (ex, ey, gxy),
// gxy the engineering shear: ex c^2 + ey s^2 + gxy s c.
[[nodiscard]] double strainAlong(double ex, double ey, double gxy, double angleInDegrees) noexcept;

// The strains (ex, ey, gxy, engineering shear) in axes turned by `angleInDegrees` about z.
[[nodiscard]] std::array<double, 3> turnStrains(const std::array<double, 3>& strains, double angleInDegrees) noexcept;

// The resultants of a plane tensor (Nx, Ny, Nxy, a stress-like tensor) in axes turned by `angleInDegrees` about z.
[[nodiscard]] std::array<double, 3> turnResultants(const std::array<double, 3>& resultants,
                                                   double angleInDegrees) noexcept;

// One face of a sandwich: the plies below (bottom) or above (top) its core, their thickness and where they are, and
// their A and B matrices about the reference plane of the laminate, in its material axes (3 x 3, row by row; the
// resultants of the face are A e0 + B k).
struct SandwichFace
{
    std::size_t plies = 0;
    double thickness = 0.0;
    double zBottom = 0.0;
    double zTop = 0.0;
    std::array<double, 9> a = {};
    std::array<double, 9> b = {};
};

// A laminate seen as a sandwich: its core ply (numbered from 1 in the stack), the core's material,
// thickness and place, its transverse shear moduli (G1Z, G2Z of a MAT8, G of a MAT1; infinite for a MAT8 whose G1Z or
// G2Z is blank), and the two faces. The distance between the middles of the faces is the lever arm of the bending.
struct SandwichLayup
{
    std::size_t corePly = 0;
    base::EntityId coreMaterial = 0;
    double coreThickness = 0.0;
    double coreZBottom = 0.0;
    double coreZTop = 0.0;
    std::array<double, 2> coreShear = {};
    SandwichFace bottom;
    SandwichFace top;
    [[nodiscard]] double faceDistance() const noexcept
    {
        return 0.5 * (top.zBottom + top.zTop) - 0.5 * (bottom.zBottom + bottom.zTop);
    }
};

// The sandwich of a PCOMP or PCOMPG: the core is the ply `corePly` (from 1, in the stack of the laminate: both halves
// of a SYM laminate), else the thickest ply (the first of the thickest). nullopt for another property, or a ply number
// outside the stack. A face with no ply (a core at the bottom or the top of the stack) has no plies and zero matrices.
// Throws MassError for a ply whose material has no plane stress law.
[[nodiscard]] std::optional<SandwichLayup> sandwichLayup(const model::Model& model, model::CardLocation property,
                                                         std::optional<std::size_t> corePly = std::nullopt);

} // namespace oor::analysis
