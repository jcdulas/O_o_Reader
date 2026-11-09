#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MATTUSR card: the table variation of the properties of a MATUSR (a user defined generic material of SOL 400).
//
// Card layout:
//   MATTUSR  MID    (blank) (blank) T(RHO) T(A1)  T(A2)  T(A3)                    fields 1 to 7
//            T(GE)  T(ST)   T(SC)   T(SS)                                          9 to 12
// T(RHO) and T(A1) to T(A3) are the IDs of a TABLEM1 or of a TABL3Di (a table of the temperature and other variables),
// 0 when blank; T(GE), T(ST), T(SC) and T(SS) are TABLEMk IDs. When T(A1) is given and T(A2) and T(A3) are blank, the
// three expansion tables are T(A1).
struct UserMatTableCard
{
    base::EntityId id = 0; // the MATUSR that the card extends
    base::EntityId densityTableId = 0;
    std::array<base::EntityId, 3> thermalExpansionTableIds{}; // T(A1), T(A2), T(A3)
    base::EntityId structuralDampingTableId = 0;
    base::EntityId tensionAllowableTableId = 0;
    base::EntityId compressionAllowableTableId = 0;
    base::EntityId shearAllowableTableId = 0;
};

} // namespace oor::model
