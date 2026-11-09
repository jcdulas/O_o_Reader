#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// Lamination parameters (a companion package and R22, a companion package RW1 and RW2): the material invariants of a ply
// (Tsai and Pagano, 1968), the lamination parameters of a stack and the moments of each angle of a finite set, the
// A, B, D matrices of lamination parameters, the feasibility rows of a finite set of angles with the coupling, and the
// fields of the PSHELL and MAT2 that give an A, B, D. First and second derivatives in closed form.
//
// Conventions: z from the mid-plane of the stack, z' = 2 z / h in [-1, 1], f(theta) = (cos 2 theta, sin 2 theta,
// cos 4 theta, sin 4 theta), angles in degrees (from the x axis of the laminate to the fibres, as Laminate), the plies
// bottom first:
//
//   xi_A = 1/2 int f dz'    xi_B = int f z' dz'    xi_D = 3/2 int f z'^2 dz'
//   A = h (G0 + sum Gi xi_A_i)    B = h^2 / 4 sum Gi xi_B_i    D = h^3 / 12 (G0 + sum Gi xi_D_i)
//
// The moments of an angle of a set are those of the plies at that angle: v = 1/2 int dz', b = int z' dz',
// w = 3/2 int z'^2 dz' (sum v = 1, sum b = 0, sum w = 1).

namespace oor::analysis
{

// The plane stress stiffness of a ply in its axes and the five invariants U1 ... U5.
struct MaterialInvariants
{
    double q11 = 0.0;
    double q22 = 0.0;
    double q12 = 0.0;
    double q66 = 0.0;
    std::array<double, 5> u{};
};

// The invariants of a ply of moduli E1, E2, NU12, G12 (NU21 = NU12 E2 / E1).
[[nodiscard]] MaterialInvariants materialInvariants(double e1, double e2, double nu12, double g12) noexcept;

// The matrices G0 ... G4 of the invariants (3 x 3 each, row by row: 45 values), so that the turned stiffness of a ply
// at theta is G0 + G1 cos 2 theta + G2 sin 2 theta + G3 cos 4 theta + G4 sin 4 theta.
[[nodiscard]] std::array<double, 45> invariantMatrices(const std::array<double, 5>& u) noexcept;

// The lamination parameters of a stack of layers (bottom first) and their derivatives with respect to the angle
// (degrees) and the thickness of each ply: layer k is ply plyOf[k] of `plyCount` (the two layers of a ply of a
// symmetric laminate are one ply: its angle and thickness move both). Component c of 12: xi_A (c = 0 ... 3), xi_B
// (4 ... 7), xi_D (8 ... 11). Derivative arrays: entry c * plyCount + p; second derivative arrays: entry
// (c * plyCount + p) * plyCount + q.
struct StackLaminationParameters
{
    std::array<double, 12> xi{};
    double thickness = 0.0;
    std::vector<double> dAngle;
    std::vector<double> dThickness;
    std::vector<double> hAngleAngle;
    std::vector<double> hAngleThickness; // (c, angle of ply p, thickness of ply q)
    std::vector<double> hThicknessThickness;
};

// Throws std::invalid_argument for a negative thickness or a stack of no thickness.
[[nodiscard]] StackLaminationParameters stackLaminationParameters(std::span<const double> angles,
                                                                  std::span<const double> thickness,
                                                                  std::span<const std::uint32_t> plyOf,
                                                                  std::size_t plyCount, bool gradient, bool hessian);

// The lamination parameters (xi, 12 a row, and the thickness) of the rows of a table of plies (a companion package RW3):
// the layers of a guide, their angles once, each row keeping some of them (`kept`, rows x layers, a factor on the
// thickness of each layer: 1 kept, 0 dropped). The trigonometry is that of a guide layer once; the values those of
// stackLaminationParameters of each row's layers, a dropped layer of no thickness, to the bit. Throws
// std::invalid_argument as it does (naming the row).
void rowLaminationParameters(std::span<const double> angles, std::span<const double> thickness,
                             std::span<const double> kept, std::size_t rows, std::span<double> xi,
                             std::span<double> totals);

// The index in `angleSet` of an angle (modulo 180 degrees, within 1e-6 degrees), or -1.
[[nodiscard]] std::ptrdiff_t angleIndex(std::span<const double> angleSet, double angle) noexcept;

// The A, B, D matrices of lamination parameters (`components` of each xi: 3, the fourth zero, or 4) and their
// derivatives with respect to the variables (h, xi_A, xi_B when `withB`, xi_D): V = 1 + (2 or 3) components.
// Derivative arrays: entry (matrix entry e of 9) * V + v; second: (e * V + v) * V + w.
struct LaminationAbd
{
    std::array<double, 9> a{};
    std::array<double, 9> b{};
    std::array<double, 9> d{};
    std::vector<double> da, db, dd;
    std::vector<double> ha, hb, hd;
};

[[nodiscard]] LaminationAbd abdFromLaminationParameters(const std::array<double, 45>& gamma, double h,
                                                        const double* xiA, const double* xiB, const double* xiD,
                                                        std::size_t components, bool gradient, bool hessian);

// The rows of the feasibility of the moments of a finite set of K angles (2^(K-1) - 1 classes {S, complement}, S the
// sets holding the first angle in increasing order of their bit masks, a lower and an upper row each):
//
//   lower = w_S - v_S^3 - 3 b_S^2 / (4 v_S)                upper = 1 - (1 - v_S)^3 - 3 b_S^2 / (4 (1 - v_S)) - w_S
//
// with their derivatives with respect to (v_S, b_S, w_S) (3 an row) and the second derivatives (9 a row). Where v_S
// (1 - v_S for the upper row) is not positive the row is its limit, w_S - v_S^3 (1 - (1 - v_S)^3 - w_S) when b_S is
// zero and minus infinity otherwise, and its derivatives NaN.
[[nodiscard]] std::vector<std::uint32_t> feasibilityClasses(std::size_t angleCount);

void momentFeasibility(std::span<const double> v, std::span<const double> b, std::span<const double> w,
                       std::span<const std::uint32_t> classes, double* rows, double* slopes, double* curvatures);

// The linear map from the lamination parameters to the moments of the angles of a set: the least-squares inverse P
// (K x 5, row by row) of the matrix M (5 x K) of (1, cos 2 theta, sin 2 theta, cos 4 theta, sin 4 theta) at the angles,
// so that v = P (1, xi_A), b = P (0, xi_B), w = P (1, xi_D) when xi lies in what the angles span. Throws
// std::invalid_argument when M has not the rank K (more than five angles, or angles equal modulo 180 degrees).
[[nodiscard]] std::vector<double> angleMomentMap(std::span<const double> angles);

// The transverse shear stiffness (H_x, H_y) of a stack of layers (bottom first) from the equilibrium of cylindrical
// bending in each direction, as a companion package's PCOMP takes it (its docs/developer/shells.md section 8): E the Q-bar 11
// (x) or 22 (y) of each layer, the neutral axis zn = B/A and D* = D - B^2/A of E, the shear per unit force
// tau(z) = -S(z) / D*, S(z) = int_bottom^z E (zeta - zn), and 1/H = int tau^2 / G dz, G the layer's turned transverse
// shear modulus (`shear` 2 x 2 row by row: G_xz on the first diagonal entry, G_yz on the second). One material gives
// 5/6 G h. A layer of infinite G in a direction makes the stack rigid in it (H infinite), as in a companion package.
[[nodiscard]] std::array<double, 2> equilibriumTransverseShear(std::span<const std::array<double, 9>> stiffness,
                                                               std::span<const std::array<double, 4>> shear,
                                                               std::span<const double> thickness);

// The largest number of angles of a set the feasibility takes (2^(K-1) - 1 classes).
inline constexpr std::size_t MAX_FEASIBILITY_ANGLES = 16;

} // namespace oor::analysis
