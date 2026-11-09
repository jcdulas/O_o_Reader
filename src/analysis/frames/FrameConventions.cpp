#include "oor/analysis/frames/FrameConventions.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace oor::analysis
{

using results::StoredFrame;

namespace
{

// The evidence behind each family: how its validated rows were checked.
constexpr std::string_view NODAL_EVIDENCE =
    "analytic test (a known vector turned by a known system, by hand); solver runs: the same model run with the basic "
    "system as displacement system and with two cylindrical displacement systems, the displacements, single-point "
    "constraint forces and applied loads of the runs in a cylindrical system turned to the basic system equal those of "
    "the basic run to single precision";
constexpr std::string_view GPF_EVIDENCE =
    "analytic test; solver runs: the grid point forces of the same model run with the basic system as displacement "
    "system and with a local cylindrical displacement system, turned to the basic system, equal those of the basic "
    "run to single precision";
constexpr std::string_view SHELL_STRESS_EVIDENCE =
    "analytic test (a known plane stress turned by a known angle, by hand); solver runs: the printed results of a "
    "plate run give the stresses in the element system and the table equals its printed rows; the trace and von "
    "Mises value of every shell table are unchanged by the frame change";
constexpr std::string_view SHELL_FORCE_EVIDENCE =
    "analytic test (membrane and bending tensors and the shear vector turned by a known angle, by hand)";
constexpr std::string_view PLY_EVIDENCE =
    "analytic test (a ply stress turned by the ply angle and the material angle, by hand); solver runs: the sum of "
    "the normal stresses of every ply table is unchanged by the frame change";
constexpr std::string_view SOLID_EVIDENCE =
    "analytic test (a known stress tensor turned by a known system, by hand); solver runs: the system word of every "
    "solid entry equals the material system of its property, and the trace and the von Mises stress of every solid "
    "table are "
    "unchanged by the frame change";
constexpr std::string_view DIRECTION_EVIDENCE = "analytic test (the component along a known direction, by hand)";

constexpr std::array<FrameTargetKind, 9> TARGETS = {
    FrameTargetKind::Basic,       FrameTargetKind::NodeOutput, FrameTargetKind::Element,
    FrameTargetKind::Material,    FrameTargetKind::Ply,        FrameTargetKind::Coordinate,
    FrameTargetKind::Cylindrical, FrameTargetKind::Spherical,  FrameTargetKind::Direction};

std::string_view evidenceOf(FrameFamily family, FrameTargetKind target) noexcept
{
    if (target == FrameTargetKind::Direction)
        return DIRECTION_EVIDENCE;
    switch (family)
    {
    case FrameFamily::NodalVector:
        return NODAL_EVIDENCE;
    case FrameFamily::NodeForceBalance:
        return GPF_EVIDENCE;
    case FrameFamily::ShellStress:
    case FrameFamily::ShellStrain:
        return SHELL_STRESS_EVIDENCE;
    case FrameFamily::ShellForce:
        return SHELL_FORCE_EVIDENCE;
    case FrameFamily::PlyStress:
    case FrameFamily::PlyStrain:
        return PLY_EVIDENCE;
    case FrameFamily::SolidStress:
    case FrameFamily::SolidStrain:
        return SOLID_EVIDENCE;
    default:
        return {};
    }
}

bool isNodal(FrameFamily family) noexcept
{
    return family == FrameFamily::NodalVector || family == FrameFamily::NodeForceBalance;
}

bool isShellTensor(FrameFamily family) noexcept
{
    return family == FrameFamily::ShellStress || family == FrameFamily::ShellStrain;
}

bool isPly(FrameFamily family) noexcept
{
    return family == FrameFamily::PlyStress || family == FrameFamily::PlyStrain;
}

bool isSolid(FrameFamily family) noexcept
{
    return family == FrameFamily::SolidStress || family == FrameFamily::SolidStrain;
}

// Why a pair of a family that has a frame change is refused; empty when it is offered.
std::string refusal(FrameFamily family, StoredFrame stored, FrameTargetKind target)
{
    const bool nodal = isNodal(family);
    const bool inPlane = target == FrameTargetKind::Element || target == FrameTargetKind::Material;
    switch (target)
    {
    case FrameTargetKind::NodeOutput:
        if (!nodal)
            return "the displacement system belongs to a grid point: the values of an element have no such system";
        break;
    case FrameTargetKind::Element:
    case FrameTargetKind::Material:
        if (nodal)
            return "a value at a grid point has no element or material system";
        if (isSolid(family))
            return "the material system of a solid is the material system of its property and its element system a "
                   "rule that no observed file shows a solid result stored in: only the basic system and the systems "
                   "of the model are offered";
        break;
    case FrameTargetKind::Ply:
        if (!isPly(family))
            return nodal ? "a value at a grid point has no ply system"
                         : "the ply system needs a laminate: it is the system of the ply tables (`ShellQuad4Composite`, "
                           "...)";
        break;
    default:
        break;
    }
    if (family == FrameFamily::ShellForce && !inPlane)
        return "the membrane, bending and transverse shear resultants are forces per unit length of an edge of the "
               "element: "
               "only their in-plane turn between the element and the material systems is defined, not a turn out of "
               "the "
               "plane of the element";
    if (isShellTensor(family) || isPly(family) || isSolid(family) || family == FrameFamily::ShellForce)
    {
        if (stored == StoredFrame::Unknown || stored == StoredFrame::Mixed)
            return "the frame the values are stored in is not known";
    }
    return {};
}

std::vector<ConventionRow> buildRows()
{
    std::vector<ConventionRow> rows;
    for (std::uint8_t index = 0; index <= static_cast<std::uint8_t>(FrameFamily::SolidStrain); ++index)
    {
        const auto family = static_cast<FrameFamily>(index);
        for (const StoredFrame stored : FrameConventionTable::storedFramesOf(family))
        {
            for (const FrameTargetKind target : TARGETS)
            {
                ConventionRow row{family, stored, target, ConventionStatus::Validated, {}, {}};
                row.reason = refusal(family, stored, target);
                if (target == FrameTargetKind::Direction && family == FrameFamily::ShellForce)
                    row.reason =
                        "the resultants are per unit length of an edge: they have no component along a direction "
                        "of space";
                if (row.reason.empty())
                    row.evidence = std::string(evidenceOf(family, target));
                else
                    row.status = ConventionStatus::Refused;
                rows.push_back(std::move(row));
            }
        }
    }
    const auto refused = [&](FrameFamily family, std::string reason)
    {
        rows.push_back(ConventionRow{
            family, StoredFrame::Unknown, FrameTargetKind::Any, ConventionStatus::Refused, std::move(reason), {}});
    };
    refused(
        FrameFamily::OneDimensionalStress,
        "the stresses of rods, bars and beams are values along the axis of the element and at its recovery points: a "
        "scalar for each point, with no components to turn");
    refused(FrameFamily::OneDimensionalForce,
            "the forces and moments of rods, bars and beams are in the element system with the signs of plane 1 and "
            "plane 2 of the element, and no observed file stores them in two frames: the turn is not validated");
    refused(FrameFamily::ConnectorForce,
            "the force of a spring, damper, bush, gap, Weld or fastener is in its element system with the sign of the "
            "force "
            "acting on the element, and no observed file stores one in two frames: the turn is not validated");
    refused(FrameFamily::Scalar, "a scalar has no components: a frame change has nothing to turn");
    refused(
        FrameFamily::Nonlinear,
        "the non-linear element tables are written in the element system of the deformed or the original element and "
        "no observed file settles which: the frame is not known");
    refused(FrameFamily::NodePointStress,
            "the grid point stresses are in the system the output request names (element, basic or a system), which "
            "the table does not record");
    refused(
        FrameFamily::RandomOrModal,
        "random analysis results (PSD, RMS, ...) and modal contributions are not components of a response: no frame "
        "change is defined for them");
    refused(FrameFamily::Other, "no frame change is defined for this table");
    return rows;
}

bool endsWith(std::string_view text, std::string_view suffix) noexcept
{
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

bool oneOf(std::string_view name, std::initializer_list<std::string_view> names) noexcept
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

} // namespace

std::string_view frameFamilyName(FrameFamily family) noexcept
{
    switch (family)
    {
    case FrameFamily::NodalVector:
        return "nodalVector";
    case FrameFamily::NodeForceBalance:
        return "nodePointForce";
    case FrameFamily::ShellStress:
        return "shellStress";
    case FrameFamily::ShellStrain:
        return "shellStrain";
    case FrameFamily::ShellForce:
        return "shellForce";
    case FrameFamily::PlyStress:
        return "plyStress";
    case FrameFamily::PlyStrain:
        return "plyStrain";
    case FrameFamily::SolidStress:
        return "solidStress";
    case FrameFamily::SolidStrain:
        return "solidStrain";
    case FrameFamily::OneDimensionalStress:
        return "oneDimensionalStress";
    case FrameFamily::OneDimensionalForce:
        return "oneDimensionalForce";
    case FrameFamily::ConnectorForce:
        return "connectorForce";
    case FrameFamily::Scalar:
        return "scalar";
    case FrameFamily::Nonlinear:
        return "nonlinear";
    case FrameFamily::NodePointStress:
        return "nodePointStress";
    case FrameFamily::RandomOrModal:
        return "randomOrModal";
    case FrameFamily::Other:
        return "other";
    }
    return "other";
}

std::string_view frameFamilyDescription(FrameFamily family) noexcept
{
    switch (family)
    {
    case FrameFamily::NodalVector:
        return "displacements, velocities, accelerations, eigenvectors, single-point and multi-point constraint "
               "forces, applied loads: a vector of three translations (or forces) and a vector of three rotations (or "
               "moments) at each grid point";
    case FrameFamily::NodeForceBalance:
        return "grid point force balance: the force and moment an element, a load or a constraint puts on a grid point";
    case FrameFamily::ShellStress:
        return "stresses oxx oyy txy of triangles and quadrilaterals at a fibre";
    case FrameFamily::ShellStrain:
        return "strains exx eyy exy of triangles and quadrilaterals at a fibre (engineering shear)";
    case FrameFamily::ShellForce:
        return "membrane, bending and transverse shear resultants of triangles and quadrilaterals";
    case FrameFamily::PlyStress:
        return "stresses o11 o22 t12 t1z t2z of a ply of a laminate";
    case FrameFamily::PlyStrain:
        return "strains e11 e22 e12 e1z e2z of a ply of a laminate (engineering shear)";
    case FrameFamily::SolidStress:
        return "the six stresses of tetrahedra, wedges, hexahedra and pyramids";
    case FrameFamily::SolidStrain:
        return "the six strains of tetrahedra, wedges, hexahedra and pyramids (engineering shear)";
    case FrameFamily::OneDimensionalStress:
        return "stresses of rods, tubes, bars, beams and bends";
    case FrameFamily::OneDimensionalForce:
        return "forces and moments of rods, tubes, bars, beams and bends";
    case FrameFamily::ConnectorForce:
        return "forces of springs, dampers, bushes, gaps, welds and fasteners";
    case FrameFamily::Scalar:
        return "scalar results: temperatures, heat loads, strain energy, failure indices, eigenvalues";
    case FrameFamily::Nonlinear:
        return "non-linear element tables";
    case FrameFamily::NodePointStress:
        return "grid point stresses and strains";
    case FrameFamily::RandomOrModal:
        return "random analysis results and modal contributions";
    case FrameFamily::Other:
        return "every other table";
    }
    return {};
}

std::string_view frameTargetName(FrameTargetKind target) noexcept
{
    switch (target)
    {
    case FrameTargetKind::Basic:
        return "basic";
    case FrameTargetKind::NodeOutput:
        return "nodeOutput";
    case FrameTargetKind::Element:
        return "element";
    case FrameTargetKind::Material:
        return "material";
    case FrameTargetKind::Ply:
        return "ply";
    case FrameTargetKind::Coordinate:
        return "coordinateSystem";
    case FrameTargetKind::Cylindrical:
        return "cylindrical";
    case FrameTargetKind::Spherical:
        return "spherical";
    case FrameTargetKind::Direction:
        return "direction";
    case FrameTargetKind::Any:
        return "any";
    }
    return "any";
}

std::string_view conventionStatusName(ConventionStatus status) noexcept
{
    return status == ConventionStatus::Validated ? "validated" : "refused";
}

std::vector<StoredFrame> FrameConventionTable::storedFramesOf(FrameFamily family)
{
    switch (family)
    {
    case FrameFamily::NodalVector:
        return {StoredFrame::NodeOutputSystem, StoredFrame::Basic};
    case FrameFamily::NodeForceBalance:
        return {StoredFrame::NodeOutputSystem};
    case FrameFamily::ShellStress:
    case FrameFamily::ShellStrain:
    case FrameFamily::ShellForce:
        return {StoredFrame::Element, StoredFrame::Material};
    case FrameFamily::PlyStress:
    case FrameFamily::PlyStrain:
        return {StoredFrame::Ply};
    case FrameFamily::SolidStress:
    case FrameFamily::SolidStrain:
        return {StoredFrame::Basic, StoredFrame::Coordinate};
    default:
        return {};
    }
}

bool FrameConventionTable::hasFrameChange(FrameFamily family) noexcept
{
    return static_cast<std::uint8_t>(family) <= static_cast<std::uint8_t>(FrameFamily::SolidStrain);
}

const std::vector<ConventionRow>& FrameConventionTable::rows()
{
    static const std::vector<ConventionRow> ROWS = buildRows();
    return ROWS;
}

ConventionRow FrameConventionTable::find(FrameFamily family, StoredFrame stored, FrameTargetKind target)
{
    const std::vector<StoredFrame> offered = storedFramesOf(family);
    if (!hasFrameChange(family))
    {
        for (const ConventionRow& row : rows())
            if (row.family == family)
                return row;
    }
    else if (std::find(offered.begin(), offered.end(), stored) != offered.end())
    {
        for (const ConventionRow& row : rows())
            if (row.family == family && row.stored == stored && row.target == target)
                return row;
    }
    ConventionRow row{family, stored, target, ConventionStatus::Refused, {}, {}};
    if (stored == StoredFrame::Unknown || stored == StoredFrame::Mixed)
        row.reason = "the frame the values are stored in is not known";
    else
        row.reason = std::string(frameFamilyName(family)) + " values are never stored in the " +
                     std::string(results::storedFrameName(stored)) + " frame";
    return row;
}

FrameFamily FrameConventionTable::familyOf(const results::ResultTableMeta& meta)
{
    using results::ResultKind;
    if (meta.variant != results::ResultVariant::Plain)
        return FrameFamily::RandomOrModal;
    switch (meta.kind)
    {
    case ResultKind::Displacements:
    case ResultKind::Velocities:
    case ResultKind::Accelerations:
    case ResultKind::Eigenvectors:
    case ResultKind::SinglePointConstrForces:
    case ResultKind::MultiPointConstrForces:
    case ResultKind::AppliedLoads:
    case ResultKind::NonlinearLoads:
        return FrameFamily::NodalVector;
    case ResultKind::NodePointForces:
        return FrameFamily::NodeForceBalance;
    case ResultKind::NodePointStresses:
    case ResultKind::NodePointStrains:
        return FrameFamily::NodePointStress;
    case ResultKind::Temperatures:
    case ResultKind::ThermalLoads:
    case ResultKind::TemperatureRates:
    case ResultKind::StrainEnergy:
    case ResultKind::KineticEnergy:
    case ResultKind::FailureIndices:
    case ResultKind::Eigenvalues:
    case ResultKind::ConstraintHeat:
    case ResultKind::HeatFlows:
        return FrameFamily::Scalar;
    case ResultKind::Stress:
    case ResultKind::Strain:
    case ResultKind::ElementForces:
    {
        const std::string_view name = meta.elementName;
        const bool stress = meta.kind == ResultKind::Stress;
        if (endsWith(name, "Nonlinear") || name == "ShellQuad4Hyperelastic")
            return FrameFamily::Nonlinear;
        if (oneOf(name, {"ShellQuad4", "ShellQuad8", "ShellQuad4Drilling", "ShellTria3", "ShellTria6", "ShellTria3Drilling"}))
            return meta.kind == ResultKind::ElementForces
                       ? FrameFamily::ShellForce
                       : (stress ? FrameFamily::ShellStress : FrameFamily::ShellStrain);
        if (endsWith(name, "Composite"))
            return meta.kind == ResultKind::ElementForces ? FrameFamily::Other
                                                          : (stress ? FrameFamily::PlyStress : FrameFamily::PlyStrain);
        if (oneOf(name, {"SolidTetra", "SolidPenta", "SolidHexa", "SolidPyramid"}))
            return meta.kind == ResultKind::ElementForces
                       ? FrameFamily::Other
                       : (stress ? FrameFamily::SolidStress : FrameFamily::SolidStrain);
        if (oneOf(name, {"Rod2", "Rod2InlineSection", "Tube2", "Bar2", "Bar2LoadStations", "Beam2",
                         "CurvedBeam2"}))
            return meta.kind == ResultKind::ElementForces ? FrameFamily::OneDimensionalForce
                                                          : FrameFamily::OneDimensionalStress;
        if (oneOf(name, {"SpringDofProperty", "SpringDofInline", "SpringScalarProperty", "SpringScalarInline", "DamperDofProperty", "DamperDofInline",
                         "DamperScalarProperty", "DamperScalarInline", "ViscousDamper2", "Gap2", "Bushing", "Bushing1D",
                         "Weld", "Fastener"}))
            return FrameFamily::ConnectorForce;
        return FrameFamily::Other;
    }
    default:
        return FrameFamily::Other;
    }
}

} // namespace oor::analysis
