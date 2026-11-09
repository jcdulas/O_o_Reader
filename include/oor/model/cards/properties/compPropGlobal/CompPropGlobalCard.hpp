#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/compProp/CompPropCard.hpp"

#include <vector>

namespace oor::model
{

// One ply of a PCOMPG laminate: a PCOMP ply with the global (external) ID the user gives it.
struct PlygCard
{
    base::EntityId globalId = 0; // GPLYID, unique in the card
    base::EntityId matId = 0;
    double thickness = 0.0;
    double thetaInDegrees = 0.0;
    bool stressOutput = false; // SOUT = YES
};

// One PCOMPG card: the property of a layered composite shell whose plies have global ply IDs.
//
// Card layout (one ply per line, with the same ply fields as PCOMP plus the global ID in front):
//   PCOMPG  PID       Z0   NSM     SB     FT     TREF  GE  LAM
//           GPLYID1   MID1 T1      THETA1 SOUT1  GEFLG
//           GPLYID2   MID2 T2      THETA2 SOUT2
//
// Z0 is stored resolved (blank gives minus half the laminate thickness). A blank MID or T repeats the one of the ply
// before. GEFLG, the ply structural damping flag (0, -1 or -2), is written on the first ply only. Every ply is given
// (there is no LAM = SYM here: the format reference lists blank, MEM, BEND, SMEAR and SMCORE).
struct CompPropGlobalCard
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
    int structuralDampingFlag = 0;
    std::vector<PlygCard> plies;

    [[nodiscard]] double totalThickness() const noexcept
    {
        double sum = 0.0;
        for (const PlygCard& ply : plies)
            sum += ply.thickness;
        return sum;
    }
};

} // namespace oor::model
