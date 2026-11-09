#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// The shift function of a MATTVE card (the time-temperature shift of a thermo-rheologically simple material).
enum class ViscoElasticMatTemperatureShift : std::uint8_t
{
    Wlf,   // WLF (the default): Williams-Landell-Ferry, the constants A1 and A2
    Power, // POWER: a power series of the temperature, the coefficients C0 to C10
    Nara,  // NARA: the Narayanaswamy model, the weights W and the relaxation times T of a Prony series
    User   // USER: the shift function of a user subroutine
};

// One MATTVE card: the temperature dependent viscoelastic properties of the material of the MATVE with the same ID
// (SOL 400).
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATTVE  MID  FUNCTION  RT  ENER  FRACT  TDIF  TREF  NP                       fields 1 to 8
//           A1   A2                                                              9 and 10 (WLF, POWER and NARA)
//           C0   C1   C2 ...                                                     POWER: NP coefficients from field 17
//           W0   W1   W2 ...                                                     NARA: NP weights, on a new line
//           T0   T1   T2 ...                                                     NARA: NP relaxation times, on a new
//           line IFICT                                                                NARA: the initial fictive
//           temperature
// A series starts on a line of its own and goes on eight values to a line. ENER, FRACT, TDIF and TREF are used by NARA
// only (a real with no default, so they are required there); NP is the number of terms of the POWER series (1 to 11)
// and of the NARA series. A1 and A2 are the WLF constants, 0.0 for the other functions (the format reference asks for
// the line to be there because a blank line is dropped); a card of the USER function may stop after the first line.
// IFICT is 1 (the initial temperature), 2 (the reference temperature of the structural relaxation) or 3 (0.0, the
// default when the field is absent).
struct ViscoElasticMatTemperatureCard
{
    static constexpr int MAX_POWER_TERMS = 11;

    base::EntityId id = 0; // the MATVE that the card extends
    ViscoElasticMatTemperatureShift shift = ViscoElasticMatTemperatureShift::Wlf;
    double shiftReferenceTemperature = 0.0;
    std::optional<double> activationEnergy;
    std::optional<double> structuralFraction;
    std::optional<double> temperatureDifference;
    std::optional<double> referenceTemperature;
    int termCount = 0;
    double wlfConstant1 = 0.0;
    double wlfConstant2 = 0.0;
    std::vector<double> powerCoefficients;   // POWER: C0 to C(NP-1)
    std::vector<double> naraWeights;         // NARA: W0 to W(NP-1)
    std::vector<double> naraRelaxationTimes; // NARA: T0 to T(NP-1)
    std::optional<int> fictiveTemperatureOption;
};

} // namespace oor::model
