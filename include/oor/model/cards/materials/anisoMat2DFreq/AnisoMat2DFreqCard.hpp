#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MAT2F card: the frequency dependence of the properties of a MAT2 (a shell element anisotropic material). Each
// field holds the ID of a TABLEDi (0 when blank: the property does not depend on the frequency) that replaces the
// property of the MAT2 with the same MID.
//
// Card layout (the fields sit where the properties sit on the MAT2):
//   MAT2F  MID    G11   G12    G13    G22    G23    G33                          fields 1 to 7
//          (blank line)
//          (blank) (blank) (blank) (blank) GE                                      the second line: GE is field 13
//          (blank) GE11  GE12   GE13   GE22   GE23   GE33                         the third line: fields 18 to 23
// The properties are kept in `stiffnessTableIds` as (G11, G12, G13, G22, G23, G33) and in
// `structuralDampingTermTableIds` as (GE11, GE12, GE13, GE22, GE23, GE33); `structuralDampingTableId` is the GE of the
// element. A table of GE or of a GEij must be blank when the property of the MAT2 is 0.0.
struct AnisoMat2DFreqCard
{
    static constexpr std::size_t TERMS = 6;

    base::EntityId id = 0; // the MAT2 that the card extends
    std::array<base::EntityId, TERMS> stiffnessTableIds{};
    base::EntityId structuralDampingTableId = 0;
    std::array<base::EntityId, TERMS> structuralDampingTermTableIds{};
};

} // namespace oor::model
