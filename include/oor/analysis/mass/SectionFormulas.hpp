#pragma once

#include "oor/analysis/mass/SectionShearSolver.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// The section properties of a bar or beam at one station, in the element coordinate system of the bar: y is the
// vertical axis of the format reference figures of PBARL (figure 9-106 and following), z the horizontal one, and the
// section lies in the plane perpendicular to the element x axis. NaN means "not defined" (a blank value on the card, a
// section whose dimensions are impossible, a property that has no such value).
//
//   area     the cross-section area
//   i1       area moment of inertia in plane 1 (the x-y plane), about the neutral axis: the integral of y squared
//   i2       area moment of inertia in plane 2 (the x-z plane), about the neutral axis: the integral of z squared
//   i12      area product of inertia, the integral of y times z
//   j        torsional constant
//   k1, k2   shear area factors in planes 1 and 2 (the shear area divided by the area); NaN when the card leaves them
//            blank (no shear flexibility) or when they were not asked for (ShearFactorRule::None).
//            A CBAR ignores them when I12 is not zero.
//   nsm      non-structural mass per unit length at the station (0 for a section that has none)
//   recovery the stress recovery points C, D, E, F as (y, z) pairs, measured from the neutral axis; NaN where not given
//   xxb      the station along the element, 0.0 at end A and 1.0 at end B
// and for a section of the library, in the coordinates of its drawing (SectionShapes.cpp: each type
// drawn with y up and z to the right as the format reference figures draw it, about an origin of its own):
//   drawingCentroid     the centroid (y, z)
//   drawingBounds       the bounding box of the section: y min, y max, z min, z max
//   drawingShearCentre  the shear centre (y, z): the pole of the Saint-Venant warping function that has no product
//                       with y nor z (SectionShearSolver), the centroid of the doubly symmetric sections (ROD, TUBE,
//                       TUBE2, BAR, HEXA); NaN under ShearFactorRule::None
//   cw                  the warping constant about the shear centre (0 for ROD, TUBE, TUBE2); NaN under None
struct SectionProperties
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();

    double xxb = 0.0;
    double area = UNDEFINED;
    double i1 = UNDEFINED;
    double i2 = UNDEFINED;
    double i12 = UNDEFINED;
    double j = UNDEFINED;
    double k1 = UNDEFINED;
    double k2 = UNDEFINED;
    double nsm = 0.0;
    std::array<std::array<double, 2>, 4> recovery = {
        {{UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}}};
    std::array<double, 2> drawingCentroid = {UNDEFINED, UNDEFINED};
    std::array<double, 4> drawingBounds = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
    std::array<double, 2> drawingShearCentre = {UNDEFINED, UNDEFINED};
    double cw = UNDEFINED;

    [[nodiscard]] bool hasArea() const noexcept { return area == area; }
};

// The section of a bar or beam property: its card type, the library section type of a PBARL or PBEAML, and one set of
// properties per station, in the order of increasing X/XB (a PBAR, PBARL and PBCOMP have one).
struct BeamSection
{
    std::string propertyType; // the card type: "BarProp", "BeamShapeProp", ...
    std::string sectionType;  // "I", "TUBE", ... for PBARL and PBEAML; empty for the others
    base::EntityId materialId = 0;
    std::vector<SectionProperties> stations;
    // Where the centre of gravity of the non-structural mass is, as (y, z) in the element system, at the two ends: M1A,
    // M2A and M1B, M2B of a PBEAM, M1 and M2 of a PBCOMP, MY and MZ of the ends of a PBEAM3 (zero for the others).
    std::array<double, 2> nsmCenterA = {0.0, 0.0};
    std::array<double, 2> nsmCenterB = {0.0, 0.0};

    // The average of `area` over the length of the element, the section varying linearly between the stations
    // (the rule of the PBEAM: the trapezoid over X/XB). NaN when a station has no area.
    [[nodiscard]] double meanArea() const;
    // The same for the non-structural mass per unit length.
    [[nodiscard]] double meanNsm() const;
};

// How the shear factors (and, for the sections without a closed form, the torsional constant) of the PBARL and PBEAML
// library are computed:
//   Solver       the solver's finite element formulation of the cross-section, the default of the solver:
//                ROD 6/7, TUBE 6 (1 + m^2)^2 / (7 (1 + m^2)^2 + 20 m^2) with m the ratio of the
//                radii, BAR 5/6, the other sections by SectionShearSolver;
//   BeamLibrary  the beam library equations the solver uses under MDLPRM,TWBRBML,1 (and other implementations and
//                earlier versions before the
//                finite element formulation): the net area of the walls along the shear over the area, the corner
//                squares where two walls meet counting for neither direction; ROD 0.9, TUBE 0.5, BAR 5/6; the
//                torsional constant of the thin-walled formulas;
//   None         not computed (NaN): the mass and the geometry need neither, and the finite element solve costs time.
enum class ShearFactorRule : std::uint8_t
{
    None,
    Solver,
    BeamLibrary
};

// The first moment of the distribution `value(station) * density` along the element, normalised: the mass per unit
// length integrated over X/XB in [0, 1] is `integral`; `centroid` is the X/XB where the integral's centre is.
struct LinearIntegral
{
    double integral = 0.0;
    double centroid = 0.5;
};

// The values at the stations of a property (area times density plus nsm, say), integrated over the length with the
// linear interpolation the PBEAM defines; the stations outside [0, 1] are not extrapolated (the first station's value
// holds before the first, the last's after the last).
[[nodiscard]] LinearIntegral integrateAlongBeam(std::span<const double> positions, std::span<const double> values);

class SectionCache;

// The derivatives of the properties of a section of the library with respect to its dimensions DIM1 ... DIMn:
// entry i of each array the derivative with respect to DIMi, `count` of them. The
// properties of the finite element formulation (K1, K2, J under the solver's rule, the shear centre and the warping
// constant) are derived on the meshes of the plan (SectionShearSolver), exactly; the closed forms by a five-point
// central difference over a relative step of 1e-3 (they are rational in the dimensions: about 1e-12 of error). NaN
// where the property is NaN; at a kink of the geometry (DBOX's central web as thick as the walls of both cells) the
// finite element derivatives along the dimensions that move it are NaN and the closed forms give the mean of the two
// sides.
struct SectionPropertiesGradient
{
    using Row = std::array<double, MAX_SECTION_DIRECTIONS>;
    std::size_t count = 0;
    Row area{};
    Row i1{};
    Row i2{};
    Row i12{};
    Row j{};
    Row k1{};
    Row k2{};
    std::array<std::array<Row, 2>, 4> recovery{};
    std::array<Row, 2> drawingCentroid{};
    std::array<Row, 4> drawingBounds{};
    std::array<Row, 2> drawingShearCentre{};
    Row cw{};
};

// The second derivatives of the properties of a section of the library with respect to two of its dimensions:
// entry i * MAX_SECTION_DIRECTIONS + j of each matrix for DIMi and DIMj, symmetric, `count` of
// them. The closed forms (the area, the moments and the product of inertia, the centroid, the bounds and the recovery
// points, and the torsional constant and the shear factors of the closed forms of the rule) through their formulas
// (forward over forward: exact to rounding); the properties of the finite element formulation on the meshes of the
// plan, exactly (SectionStiffnessHessian). NaN where the property is NaN, and for the finite element ones along a
// dimension whose meshes cannot be made; at a kink of the geometry, the side the formulas take.
struct SectionPropertiesHessian
{
    using Matrix = std::array<double, MAX_SECTION_DIRECTIONS * MAX_SECTION_DIRECTIONS>;
    std::size_t count = 0;
    Matrix area{};
    Matrix i1{};
    Matrix i2{};
    Matrix i12{};
    Matrix j{};
    Matrix k1{};
    Matrix k2{};
    std::array<std::array<Matrix, 2>, 4> recovery{};
    std::array<Matrix, 2> drawingCentroid{};
    std::array<Matrix, 4> drawingBounds{};
    std::array<Matrix, 2> drawingShearCentre{};
    Matrix cw{};
};

// The outline of a section of the library: closed rings of (y, z) points in the coordinates of its
// drawing (as SectionProperties::drawingCentroid), the outer outline counter-clockwise in the plane with z to the right
// and y up, the holes (`holes` 1) clockwise. Polygons with the area of the section exactly, except ROD and TUBE, whose
// circles are regular polygons of CIRCLE_POINTS corners (their area short by about 0.16 %).
struct SectionOutline
{
    static constexpr std::size_t CIRCLE_POINTS = 64;
    std::vector<std::vector<std::array<double, 2>>> rings;
    std::vector<std::uint8_t> holes;
};

// The section formulas of the library of cross-sections (PBARL and PBEAML, the standard GROUP), and the sections of the
// bar and beam property cards as they are given.
class SectionFormulas
{
public:
    // The property of one section of the library from its dimensions (DIM1 ... DIMn of a station). The formulas are
    // exact for the area, the moments and the product of inertia of the figure the format reference draws (the figure
    // is cut into rectangles, a circle or a polygon, and integrated); the torsional constant and the shear factors
    // follow `rule`. A type that is not in the library, a wrong number of dimensions or an impossible section (a wall
    // thicker than the section) gives all NaN.
    [[nodiscard]] static SectionProperties standard(std::string_view type, std::span<const double> dimensions,
                                                    ShearFactorRule rule = ShearFactorRule::Solver);
    // The same with the meshes of the finite element formulation planned for the dimensions `reference` (empty: the
    // section's own), so that the properties are smooth functions of the dimensions around the reference (a companion
    // package P10.3; a section whose meshes cannot be made from the reference plan is computed on its own), and with
    // `gradient`, their derivatives with respect to the dimensions (at most MAX_SECTION_DIRECTIONS).
    [[nodiscard]] static SectionProperties standard(std::string_view type, std::span<const double> dimensions,
                                                    ShearFactorRule rule, std::span<const double> reference,
                                                    SectionPropertiesGradient* gradient);
    // The same with, when `hessian` is given, the second derivatives too (a companion package; they need the first
    // ones, computed whether `gradient` is given or not).
    [[nodiscard]] static SectionProperties standard(std::string_view type, std::span<const double> dimensions,
                                                    ShearFactorRule rule, std::span<const double> reference,
                                                    SectionPropertiesGradient* gradient,
                                                    SectionPropertiesHessian* hessian);

    // The outline of a section of the library from its dimensions; no ring for an impossible section or a type that
    // is not in the library.
    [[nodiscard]] static SectionOutline outline(std::string_view type, std::span<const double> dimensions);

    // The rule of a model: BeamLibrary when it has MDLPRM,TWBRBML,1 (format reference MDLPRM: 0, the default, is the
    // finite element formulation), else Solver.
    [[nodiscard]] static ShearFactorRule ruleOf(const model::Model& model);

    // True when the property card type is one whose section is known (PBAR, PBARL, PBEAM, PBEAML, PBEAM3, PBCOMP, PROD,
    // PTUBE, PBEND).
    [[nodiscard]] static bool supports(std::uint16_t propertyType) noexcept;

    // The section of the property in `location` (a live row of a property table), or nullopt for a property that has
    // none (PSHELL, PSOLID, ...). The shear factors of a library section follow `rule`; with a `cache`, a library
    // section already computed is taken from it.
    [[nodiscard]] static std::optional<BeamSection> of(const model::Model& model, model::CardLocation location,
                                                       ShearFactorRule rule, SectionCache* cache = nullptr);
    // The same with the rule of the model (ruleOf).
    [[nodiscard]] static std::optional<BeamSection> of(const model::Model& model, model::CardLocation location);
    // The section of the property with this ID, or nullopt (the rule of the model).
    [[nodiscard]] static std::optional<BeamSection> of(const model::Model& model, base::EntityId pid);
};

} // namespace oor::analysis
