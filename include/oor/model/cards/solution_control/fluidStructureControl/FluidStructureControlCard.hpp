#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// The solution strategy of the coupling between the external code and the solver.
enum class FsiCouplingType : std::uint8_t
{
    Explicit, // the default
    Implicit,
};

// The words of TYPE, in the order of the enumeration.
inline constexpr std::array<std::string_view, 2> FLUID_STRUCTURE_COUPLING_WORDS{"ExplicitSolver", "ImplicitSolver"};

// The interface that the service implements: DV is the basic interface, U the extended one.
enum class FsiServiceInterface : std::uint8_t
{
    Dv, // the default
    U,
};

// The words of ANALYSIS, in the order of the enumeration.
inline constexpr std::array<std::string_view, 2> FLUID_STRUCTURE_INTERFACE_WORDS{"DesignVariables", "Displacement"};

// One FSICTRL card (SOL 400): the analysis type of an external fluid-structure simulation, for the service SERV_ID that
// the FMS statement CONNECT SERVICE defines. No other entry of the format reference has this name.
//
// Card layout: FSICTRL  SERV_ID  TYPE  FREQ  ANALYSIS                                fields 1 to 4
// SERV_ID is required, at most eight characters, and is kept as the deck wrote it; the WETLOAD cards of the
// service carry the same name. TYPE is EXPLICIT when blank, FREQ (a positive integer, the update frequency per time
// step, which the format ties to the IMPLICIT strategy) is 1 and ANALYSIS is DV; all three defaults are static, so a
// card leaves the field blank when it holds the default.
//
// The card has no ID (IdSpace::None, category Other): the service name identifies it, `model.fsictrl["fsisrv"]` finds
// it without regard to the case. The format does not say how many FSICTRL cards a service may have; the library does
// not limit them.
struct FluidStructureControlCard
{
    static constexpr FsiCouplingType DEFAULT_TYPE = FsiCouplingType::Explicit;
    static constexpr std::int32_t DEFAULT_FREQUENCY = 1;
    static constexpr FsiServiceInterface DEFAULT_ANALYSIS = FsiServiceInterface::Dv;

    std::string serviceName;
    FsiCouplingType type = DEFAULT_TYPE;
    std::int32_t frequency = DEFAULT_FREQUENCY;
    FsiServiceInterface analysis = DEFAULT_ANALYSIS;
};

} // namespace oor::model
