#pragma once

#include <optional>
#include <string>

namespace oor::model
{

// CREEP: the creep model of the job. VALC1 (0 Maxwell, 1 explicit viscoplastic by the user subroutine CRPLAW) and VALC2
// (0, or 1 reserved) are the explicit creep; VALC3 (0 off, 1 on) and VALC4 (0 elastic tangent, 1 secant tangent, 2
// radial return) the implicit creep.
struct NonlinearMatOptionsCreep
{
    int explicitModel = 0;
    int explicitOption = 0;
    int implicitCreep = 0;
    int implicitTangent = 0;
};

// HEMICUBE: the view factor calculation method. VALUE is 0 for the finite difference, contour or Gaussian integration
// and 1 for the pixel based modified hemicube method; NPIXEL (500) the number of pixels; CUTOFF (0.0) the fraction of
// the maximum view factor under which view factors are ignored; FRACTION (0.01) the fraction of the maximum view
// factors that is treated implicitly; FACCNT (0 or 1) activates the explicit treatment of the reflection matrix; FACTOL
// (absent when blank) is the tolerance of its iteration.
struct NonlinearMatOptionsHemicube
{
    static constexpr int DEFAULT_PIXEL_COUNT = 500;
    static constexpr double DEFAULT_IMPLICIT_FRACTION = 1.0e-2;

    int method = 0;
    int pixelCount = DEFAULT_PIXEL_COUNT;
    double viewFactorCutoff = 0.0;
    double implicitFraction = DEFAULT_IMPLICIT_FRACTION;
    int explicitReflection = 0;
    std::optional<double> reflectionTolerance;
};

// SPROPMAP: the automatic inclusion of the nonlinear property extensions. PROPMAP is -1 (no mapping) to 3; PROPBEH is
// the property behaviour (1D, 3D, PLST, PSTR or AXI; blank is the default of the element); DIRECT (1 to 3) the layer
// direction; THICKOP (1.0) the out of plane thickness of 2D solids; IPRINT (0 to 3) the print and punch flag of the
// generated properties.
struct NonlinearMatOptionsPropertyMap
{
    int mapping = 0;
    std::string behaviour;
    int layerDirection = 1;
    double outOfPlaneThickness = 1.0;
    int printFlag = 0;
};

// DEACTEL: the default rules and the diagnostics of element deactivation. VALD1 (-1 to 2) is absent when blank (the
// default depends on the analysis, -1 for a single physics and 0 for a coupled one); VALD2 (0 to 3) is the diagnostic
// output.
struct NonlinearMatOptionsDeactivation
{
    std::optional<int> rule;
    int output = 0;
};

// ENTHALP: the scheme of the capacitance matrix (VALCLU, 0 to 3) and of the enthalpy vector (VALEN1, 0 to 2) of
// elements with nonlinear property extensions in transient heat transfer.
struct NonlinearMatOptionsEnthalpy
{
    int capacitanceScheme = 0;
    int enthalpyScheme = 0;
};

// INLAM: the composite options. VCOORD selects the coordinate system of the interlaminar stress output (blank, BOTT or
// TOP), CPROJ (OFF, the default, or ON) the projection of the X axis of CORDM onto the layer plane, and ISTRAIN (0 or
// 1) the strain output.
struct NonlinearMatOptionsInterlaminar
{
    std::string frame;
    std::string axisProjection = "Off";
    int strainOutput = 0;
};

// One NLMOPTS card: the nonlinear material, composite and control options of SOL 400.
//
// Card layout: every option is a line of the card that starts with its keyword in the first field of the line, followed
// by its values; the options may come in any order, each at most once, and a missing option has its defaults:
//   NLMOPTS  "CREEP"    VALC1  VALC2  VALC3  VALC4                               fields 1 to 5 (the first line)
//            "ASSM"     VALA                                                     ASSUMED, OFF or blank
//            "TSHEAR"   VALS                                                     TSHEAR or blank
//            "LRGSTRN"  VALLE                                                    -1, 0, 1 or 2
//            "HEMICUBE" VALUE  NPIXEL  (blank)  CUTOFF  FRACTION  FACCNT  FACTOL
//            "TEMPP"    VALTD                                                    CONS, LINE or QUAD
//            "TEMGO"    VMAPTG                                                   NO or YES
//            "SPROPMAP" PROPMAP  PROPBEH  DIRECT  THICKOP  IPRINT
//            "SPCRMPT"  VRAMP                                                    0 or 1
//            "DEACTEL"  VALD1  VALD2
//            "ENTHALP"  VALCLU  VALEN1
//            "MAPTOL"   VMPTOL                                                   greater than 0.0, 0.2 when blank
//            "INLAM"    VCOORD  CPROJ  ISTRAIN
// An option that has only a word keeps it in a string (blank when the field is); a missing option is absent. The card
// has no ID. It is written with the options in the order above.
struct NonlinearMatOptionsCard
{
    std::optional<NonlinearMatOptionsCreep> creep;
    std::optional<std::string> assumedStrain;
    std::optional<std::string> transverseShear;
    std::optional<int> largeStrain;
    std::optional<NonlinearMatOptionsHemicube> hemicube;
    std::optional<std::string> temperatureDistribution;
    std::optional<std::string> gridMapping;
    std::optional<NonlinearMatOptionsPropertyMap> propertyMap;
    std::optional<int> spcRamp;
    std::optional<NonlinearMatOptionsDeactivation> deactivation;
    std::optional<NonlinearMatOptionsEnthalpy> enthalpy;
    std::optional<double> mapTolerance;
    std::optional<NonlinearMatOptionsInterlaminar> interlaminar;
};

} // namespace oor::model
