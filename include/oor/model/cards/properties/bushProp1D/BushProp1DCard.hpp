#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// How a nonlinear relationship of a PBUSH1D is given: by TABLEDi entries (TABLE, the default) or by DEQATN equations.
enum class BushProp1DDefinition : std::uint8_t
{
    Table,
    Equation
};

// The SHOCKA continuation of a PBUSH1D: F(u, v) = CV S(u) sign(v) |v|^EXPV, a shock absorber.
//
// Card layout (two lines):
//   "SHOCKA"  TYPE   CVT    CVC    EXPVT   EXPVC   IDTS
//                    IDETS  IDECS  IDETSD  IDECSD
//
// CVC, EXPVT and EXPVC have defaults that depend on other fields (CVT, 1.0 and EXPVT): stored resolved. With TYPE =
// TABLE the scale factor S is the TABLEDi IDTS; with TYPE = EQUAT it is the DEQATN IDETS (tension) and IDECS
// (compression, IDETS when blank), and IDETSD and IDECSD (IDETSD when blank) are the equations of its derivative.
struct BushProp1DShock
{
    bool present = false;
    BushProp1DDefinition type = BushProp1DDefinition::Table;
    double dampingTension = 0.0;
    double dampingCompression = 0.0;
    double velocityExponentTension = 1.0;
    double velocityExponentCompression = 1.0;
    base::EntityId scaleTableId = 0;
    base::EntityId scaleTensionEquationId = 0;
    base::EntityId scaleCompressionEquationId = 0;
    base::EntityId scaleTensionDerivativeEquationId = 0;
    base::EntityId scaleCompressionDerivativeEquationId = 0;
};

// The SPRING (force against displacement) or DAMPER (force against velocity) continuation of a PBUSH1D.
//
// Card layout (one line):
//   "SPRING"  TYPE  IDT  IDC  IDTDU  IDCDU      "DAMPER"  TYPE  IDT  IDC  IDTDV  IDCDV
//
// With TYPE = TABLE IDT is the TABLEDi of tension and compression; with TYPE = EQUAT IDT and IDC are the DEQATN
// equations of tension and compression and IDTD and IDCD those of the derivative (IDTDU/IDCDU of a spring, IDTDV/IDCDV
// of a damper). IDC defaults to IDT and IDCD to IDTD: stored resolved.
struct BushProp1DCurve
{
    bool present = false;
    BushProp1DDefinition type = BushProp1DDefinition::Table;
    base::EntityId tensionCurveId = 0;
    base::EntityId compressionCurveId = 0;
    base::EntityId tensionDerivativeId = 0;
    base::EntityId compressionDerivativeId = 0;
};

// The GENER continuation of a PBUSH1D: a general force against displacement and velocity, given by equations only.
//
// Card layout (one line):
//   "GENER"  TYPE  IDT  IDC  IDTDU  IDCDU  IDTDV  IDCDV
//
// IDC, IDCDU and IDCDV default to IDT, IDTDU and IDTDV: stored resolved. TYPE is blank or EQUAT, always an equation.
struct BushProp1DGeneral
{
    bool present = false;
    base::EntityId tensionCurveId = 0;
    base::EntityId compressionCurveId = 0;
    base::EntityId tensionDisplacementDerivativeId = 0;
    base::EntityId compressionDisplacementDerivativeId = 0;
    base::EntityId tensionVelocityDerivativeId = 0;
    base::EntityId compressionVelocityDerivativeId = 0;
};

// One PBUSH1D card: the property of a rod type spring-and-damper (CBUSH1D).
//
// Card layout:
//   PBUSH1D  PID   K    C    M    (blank)  SA   SE
// and the optional continuations SHOCKA, SPRING, DAMPER and GENER, each starting a line, in any order, each at most
// once. The card is written with them in that order.
struct BushProp1DCard
{
    base::EntityId id = 0;
    double stiffness = 0.0;    // stiffness
    double damping = 0.0;      // viscous damping
    double mass = 0.0;         // mass of the element
    double stressFactor = 0.0; // stress recovery coefficient
    double strainFactor = 0.0; // strain recovery coefficient
    BushProp1DShock shock;
    BushProp1DCurve spring;
    BushProp1DCurve damper;
    BushProp1DGeneral general;
};

} // namespace oor::model
