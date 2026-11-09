#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MAT9F card: the frequency dependence of the properties of a MAT9 (a solid element anisotropic material). Each
// field holds the ID of a TABLEDi (0 when blank: the property does not depend on the frequency) that replaces the
// property of the MAT9 with the same MID.
//
// Card layout (the fields sit where the properties sit on the MAT9, numbered across the lines, eight to a line):
//   MAT9F  MID    G11   G12   G13   G14   G15   G16   G22                       fields 1 to 8
//          G23    G24   G25   G26   G33   G34   G35   G36                       9 to 16
//          G44    G45   G46   G55   G56   G66                                    17 to 22
//          (blank) (blank) (blank) (blank) (blank) (blank) GE                    the fourth line: GE is field 31
//          GE11   GE12  GE13  GE14  GE15  GE16  GE22  GE23                       33 to 40
//          GE24   GE25  GE26  GE33  GE34  GE35  GE36  GE44                       41 to 48
//          GE45   GE46  GE55  GE56  GE66                                         49 to 53
// The Gij are kept in `stiffnessTableIds` in the order G11 G12 G13 G14 G15 G16 G22 G23 G24 G25 G26 G33 G34 G35 G36 G44
// G45 G46 G55 G56 G66 (the upper triangle of the matrix by rows, fields 2 to 22) and the GEij in
// `structuralDampingTermTableIds` in the same order (fields 33 to 53); `structuralDampingTableId` is the GE of the
// element. A table of GE or of a GEij must be blank when the property of the MAT9 is 0.0.
struct AnisoMat3DFreqCard
{
    static constexpr std::size_t TERMS = 21;

    base::EntityId id = 0; // the MAT9 that the card extends
    std::array<base::EntityId, TERMS> stiffnessTableIds{};
    base::EntityId structuralDampingTableId = 0;
    std::array<base::EntityId, TERMS> structuralDampingTermTableIds{};
};

} // namespace oor::model
