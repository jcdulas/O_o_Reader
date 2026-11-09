#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MDBUSH card: a generalized spring-and-damper element whose end points may lie in different modules; the rules
// of the CBUSH apply to it.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank):
//   MDBUSH  EID  PID   GA    GB    X1   X2   X3   CID
//           S    OCID  S1    S2    S3   MODA MODB MODC
//   MDBUSH  EID  PID   GA    GB    GO             CID
// This is the CBUSH with the three module fields in the last places of the second line, and it is read as the CBUSH is:
// field 5 is an integer (GO) or a real (X1), both can be blank (then the element axis is the line from GA to GB), X2
// and X3 count only when X1 is there, GB blank is a grounded end, a blank PID is the EID (stored resolved), CID blank
// is kept apart from CID 0 (the basic system), S is 0.5 and OCID -1 when blank. EID is above zero and below 100 000
// 000.
//
// MODA is the module that holds GA, MODB the one that holds GB, and MODC the one that holds GO, CID and OCID (0 when
// blank: the main Bulk Data; the format reference gives MODA and MODB no default, and a blank field is read as 0 as in
// the other cards of the family). GA and GB may be the same number when their modules differ, which is what the card is
// for. Looked up in this model: GA when MODA is 0, GB when MODB is 0, GO, CID and OCID when MODC is 0, and the property
// PID, which the format ties to a PBUSH with no module, so it is a property of the main Bulk Data (the
// element itself must be in module 0). The module numbers are plain integers.
struct ModuleBushingCard
{
    static constexpr base::EntityId NO_FRAME_ID = -1;        // CID blank: the element system comes from GO or X
    static constexpr base::EntityId NO_OFFSET_FRAME_ID = -1; // OCID blank or -1: the spring-damper is on the line GA-GB
    static constexpr double DEFAULT_OFFSET_FRACTION = 0.5;
    static constexpr base::EntityId MAX_ID = 99999999; // the format reference: EID below 100 000 000

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;

    base::EntityId orientationNode = 0;           // the grid point that gives the orientation vector; 0: none
    std::array<double, 3> orientationVector = {}; // X1 X2 X3: the orientation vector; all zero: none

    base::EntityId frameId = NO_FRAME_ID; // NO_FRAME_ID: blank; 0 is the basic system
    double offsetFraction =
        DEFAULT_OFFSET_FRACTION; // location of the spring-damper along GA-GB, used when OCID is NO_OFFSET_FRAME_ID
    base::EntityId offsetFrameId = NO_OFFSET_FRAME_ID;
    std::array<double, 3> offsets = {}; // S1 S2 S3: the spring-damper offset in the system OCID

    base::EntityId moduleIdA = 0;           // the module of GA
    base::EntityId moduleIdB = 0;           // the module of GB
    base::EntityId orientationModuleId = 0; // the module of GO, CID and OCID

    [[nodiscard]] bool hasVector() const noexcept
    {
        return orientationVector[0] != 0.0 || orientationVector[1] != 0.0 || orientationVector[2] != 0.0;
    }
    [[nodiscard]] bool operator==(const ModuleBushingCard&) const noexcept = default;
};

} // namespace oor::model
