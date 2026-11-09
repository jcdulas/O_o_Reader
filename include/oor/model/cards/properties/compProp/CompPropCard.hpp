#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

enum class FailureTheory : std::uint8_t
{
    None,
    Hill,
    Hoffman,
    TsaiWu,
    MaxStrain,
    HashinFailure, // HFAIL
    HashinTape,    // HTAPE
    HashinFabric   // HFABR
};

enum class LaminateOption : std::uint8_t
{
    None,  // blank: all plies are given, no symmetry
    Sym,   // symmetric: only the plies on one side of the mid-plane are given
    Mem,   // membrane only
    Bend,  // bending only
    Smear, // smeared membrane
    SmCore // smeared with a core
};

// One ply of a laminate.
struct PlyCard
{
    base::EntityId matId = 0;
    double thickness = 0.0;
    double thetaInDegrees = 0.0;
    bool stressOutput = false; // SOUT = YES
};

// One PCOMP card: the property of a layered composite shell.
//
// Card layout:
//   PCOMP  PID   Z0     NSM    SB      FT     TREF   GE     LAM
//          MID1  T1     THETA1 SOUT1   MID2   T2     THETA2 SOUT2
//          MID3  T3 ...
//
// Z0 is stored resolved (blank gives minus half the laminate thickness). With LAM = SYM only half of the plies are
// given: `plies` holds exactly what the card holds, the mirrored half is not stored.
struct CompPropCard
{
    base::EntityId id = 0;
    double bottomOffset = 0.0;
    // False when Z0 was blank: it is then -T/2 of the laminate and follows its thickness (a design, an edit of the
    // plies) and is written blank. True for a value given, the default of a card made in code.
    bool bottomOffsetGiven = true;
    double nonStructMass = 0.0;
    double bondShearAllowable = 0.0; // SB
    FailureTheory failureTheory = FailureTheory::None;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    LaminateOption option = LaminateOption::None;
    std::vector<PlyCard> plies;

    // Total thickness: the sum of the ply thicknesses, twice that for a symmetric laminate.
    [[nodiscard]] double totalThickness() const noexcept
    {
        double sum = 0.0;
        for (const PlyCard& ply : plies)
            sum += ply.thickness;
        return option == LaminateOption::Sym ? 2.0 * sum : sum;
    }
};

} // namespace oor::model
