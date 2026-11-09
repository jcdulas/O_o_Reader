#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The flutter analysis method (METHOD): the K method, the K method restricted for efficiency, the PK method, the PK
// method with no looping, and the sweep forms of the last two. The enumerators follow the order of the words.
enum class AeroFlutterMethod : std::uint8_t
{
    K,
    Ke,
    Pk,
    Pknl,
    Pks,
    Pknls
};

inline constexpr std::array<std::string_view, 6> FLUTTER_METHOD_WORDS{
    "KMethod", "KMethodEfficient", "PkMethod", "PkMethodNoLoop", "PkMethodSweep", "PkMethodNoLoopSweep"};

// The interpolation of the aerodynamic matrices (IMETH): linear (the default), surface (a surface across Mach numbers
// and reduced frequencies) or termwise cubic.
enum class AeroFlutterInterpolation : std::uint8_t
{
    Linear,
    Surface,
    TermwiseCubic
};

inline constexpr std::array<std::string_view, 3> FLUTTER_INTERPOLATION_WORDS{"Linear", "Surface", "TermwiseCubic"};

// The methods whose field 7 is the largest frequency of the sweep (OMAX, a real); for the others it is the number of
// eigenvalues kept (NVALUE, an integer).
[[nodiscard]] constexpr bool isSweepMethod(AeroFlutterMethod method) noexcept
{
    return method == AeroFlutterMethod::Pks || method == AeroFlutterMethod::Pknls;
}

// One FLUTTER card: the data of a flutter analysis, selected by the Case Control command FMETHOD = SID.
//
// Card layout: FLUTTER  SID  METHOD  DENS  MACH  RFREQ  IMETH  NVALUE/OMAX  EPS
// DENS, MACH and RFREQ are the IDs of the FLFACT cards that list the density ratios, the Mach numbers and the reduced
// frequencies. For the PK methods the third list holds velocities, and the format reference calls the field VEL; the
// field keeps the name RFREQ. The densities are RHOREF of the AERO card times the factor, and all combinations of the
// three lists are run except for PKNL and PKNLS, which pair the i-th numbers of the three lists (the lists
// must then have the same length, a rule between cards).
//
// METHOD has no default and is required. IMETH is L when blank. Field 7 holds an integer or a real by METHOD: NVALUE
// for K, KE, PK and PKNL, OMAX for the sweeps PKS and PKNLS. Both are positive when given and their blank has the
// default the analysis computes (the number of modal degrees of freedom; the highest natural frequency). The card keeps
// them as two members and writes the one the method uses in field 7; the other must be empty (a PKS card with an NVALUE
// would read back as an OMAX). NVALUE is 0 when blank. EPS is the convergence parameter of the PK methods, and the
// sweeps use it to cut the reduced frequency range in INT(1 / EPS) equal intervals; it is 1.0E-3 when
// blank.
//
// The format says the surface interpolation is for the K and KE methods and the termwise cubic one for the PK
// methods: a K or KE card with TCUB is refused. The S word on a PK card means linear interpolation there and is
// allowed.
struct AeroFlutterCard
{
    base::EntityId id = 0;
    AeroFlutterMethod method = AeroFlutterMethod::K;
    base::EntityId densityFactorsId = 0;
    base::EntityId machFactorsId = 0;
    base::EntityId reducedFreqFactorsId = 0;
    AeroFlutterInterpolation interpolationMethod = AeroFlutterInterpolation::Linear;
    std::int32_t eigenvalueCount = 0;   // 0 when blank; only for K, KE, PK and PKNL
    std::optional<double> maxFrequency; // only for PKS and PKNLS
    double epsilon = 1.0E-3;
};

} // namespace oor::model
