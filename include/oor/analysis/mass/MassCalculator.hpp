#pragma once

#include "oor/analysis/mass/MassProperties.hpp"
#include "oor/analysis/mass/NonStructuralMass.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace oor::analysis
{

// Where the mass of an element with offsets is: the shells with a ZOFFS (CQUAD4, CTRIA3, CQUADR, CTRIAR, CQUAD8,
// CTRIA6, ...) and the bars and beams with WA, WB (CBAR, CBEAM; CBEAM3 with WC). The amount of mass is the same
// either way (a bar's or a beam's that of its length between the offset ends); only where it is changes.
//
//   Model       as the MDLPRM OFFDEF of the model says (MassCalculator::offsetMassOf)
//   GridPoints  at the grid points, as the solver's default ELMOFF: a shell's mass on
//               the plane of its grid points, a bar's or a beam's between its grid points (the point at a fraction of
//               the length between the offset ends goes to the same fraction between the grid points; the
//               non-structural mass off the axis keeps its place in the element axes)
//   Offsets     on the offsets, as the solver's LROFF and ELMZ: a shell's mass ZOFFS from its grid points along its
//   normal,
//               a bar's or a beam's between its offset ends
enum class OffsetMass : std::uint8_t
{
    Model,
    GridPoints,
    Offsets
};

// What the mass of the elements depends on besides the cards.
//
//   weightFactor   multiplies every mass and inertia (PARAM,WTMASS; 1.0 gives the masses of the deck's unit system)
//   nonStructural  the non-structural mass of an NSM set (NonStructuralMass::resolve), added on top of the NSM fields
//                  of the properties; none when null
//   offsets        where the mass of the elements with offsets is (OffsetMass; the model's MDLPRM OFFDEF by default)
struct MassOptions
{
    double weightFactor = 1.0;
    const NonStructuralMass* nonStructural = nullptr;
    OffsetMass offsets = OffsetMass::Model;
};

// The inertia of an element about its own centre, for the elements that have some (concentrated masses, a beam whose
// non-structural mass is off its axis): the element at this index of the arrays.
struct OwnInertia
{
    std::uint32_t index = 0;
    base::Mat3 tensor{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
};

// The mass of many elements of one card type, in the order of the rows asked for. NaN is a mass that could not be
// computed (a curved bend, an element between nodes that are not there, a section that is impossible); an element that
// has no mass (a spring, a rigid element) has 0.
struct ElementMasses
{
    std::vector<double> mass;              // everything: the structural mass, the NSM of the property and the NSM cards
    std::vector<double> structural;        // the part that is the mass of the material
    std::vector<double> nonStructural;     // the NSM of the property, of the element and of the NSM set
    std::vector<base::Vec3> center;        // where the mass is (NaN for an element with no place)
    std::vector<base::EntityId> property; // the property, 0 for an element that has none
    std::vector<base::EntityId> material; // the material of the structural mass, 0 for none or for several (laminates)
    std::vector<OwnInertia> inertia;
};

// The rows of a card type: what a total, a selection or a grouping is made of.
struct RowSet
{
    std::uint16_t type = 0;
    std::vector<std::uint32_t> rows;
};

// The total of a set of elements.
//
//   sums       the mass, the first moment and the inertia
//   elements   the number of elements counted
//   excluded   the elements whose mass is not computed (NaN), by card name: they are not in the sums
struct MassTotals
{
    MassSums sums;
    std::size_t elements = 0;
    std::map<std::string, std::size_t> excluded;
};

enum class MassGrouping : std::uint8_t
{
    Property,    // by property ID (0: the elements without one)
    Material,    // by material ID of the structural mass (0: no material, and the non-structural mass)
    ElementType, // by card name
};

// The masses grouped by a key: an ID for the properties and the materials, a card name for the element types.
struct MassGroups
{
    std::map<std::int64_t, MassSums> byId;
    std::map<std::string, MassSums> byName;
};

// The mass of every mass-bearing element of a model, in the basic system.
//
//   shells         PSHELL, PSHEAR: area times (density times thickness + NSM); PCOMP, PCOMPG: area times (the plies'
//   density
//                  times thickness + NSM); PLPLANE and the axisymmetric elements: density times volume
//   solids         PSOLID, PLSOLID: density times the isoparametric volume
//   rods and bars  (density times area + NSM) times length, the section varying linearly between the stations of a beam
//   CONROD         the same with the section on the element
//   CONM1, CONM2   the concentrated mass with its centre and inertia; CMASS1, CMASS2 on grid points; CBUSH and CFAST
//   with
//                  the mass of their property
//   all the rest   no mass: springs, dampers, gaps, rigid elements, plot elements, CMASS3 and CMASS4 (scalar points)
// The NSM sets add non-structural mass; the weight factor scales everything.
class MassCalculator
{
public:
    // The mass of the elements in `rows` of the table of `typeIndex`, which must be live rows. A card type that has no
    // mass gives zeros, a CCONEAX (whose geometry is not computed) gives NaN.
    [[nodiscard]] static ElementMasses compute(const model::Model& model, std::uint16_t typeIndex,
                                               std::span<const std::uint32_t> rows, const MassOptions& options = {},
                                               base::ThreadPool* pool = nullptr);

    // The mass of one element (the arrays have one entry).
    [[nodiscard]] static ElementMasses of(const model::Model& model, model::CardLocation location,
                                          const MassOptions& options = {});

    // Every live element of the model (the card types of the category Element and the rigid elements are not included:
    // they have no mass), by card type.
    [[nodiscard]] static std::vector<RowSet> allElements(const model::Model& model);

    // The total of the elements of `sets`.
    [[nodiscard]] static MassTotals total(const model::Model& model, const std::vector<RowSet>& sets,
                                          const MassOptions& options = {}, base::ThreadPool* pool = nullptr);

    // The same, in groups.
    [[nodiscard]] static MassGroups group(const model::Model& model, const std::vector<RowSet>& sets,
                                          MassGrouping grouping, const MassOptions& options = {},
                                          base::ThreadPool* pool = nullptr);

    // The factor of PARAM,WTMASS in the model (1.0 when the parameter is not there).
    [[nodiscard]] static double weightFactorOf(const model::Model& model);

    // Where the model's MDLPRM OFFDEF puts the mass of the elements with offsets (the last MDLPRM
    // that gives it): GridPoints for ELMOFF, the default when no card gives it, and for NOMASS and NDMTZ (LROFF "but
    // the no mass effects are included", and its variant with the element z direction); Offsets for LROFF, NODIF,
    // NOTHRM, NODT and ELMZ (LROFF and its variants that keep the mass effects; the offset of a shell is taken along
    // its element normal under each of them). Throws MassError for another value.
    [[nodiscard]] static OffsetMass offsetMassOf(const model::Model& model);
};

} // namespace oor::analysis
