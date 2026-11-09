#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The module that a group (or the card) of an IMPGEOM names when it names none: the whole model.
inline constexpr std::int32_t IMPERFECTION_WHOLE_MODEL = -1;

// One group of an IMPGEOM: one imperfection that is added to the geometry, taken from an imperfection input file (a
// result file of SOL 101, 103, 105 or 400, or a text file of displacements or coordinates). The result is selected by
// SUBCASE, STEP and MODEINC; the grid points that take it are those of a set; SCALE multiplies the variation; UNIT is
// the number of the file of the ASSIGN statement; MODID names a module.
//
// Card layout of the line of a group: (blank)  SUBCASEi  STEPi  MODEINCi  SETIDi  SCALEi  (blank)  UNITi  MODIDi
// SETID, SCALE, UNIT and MODID are resolved here: a field the deck leaves blank takes the value of the card's own
// SETID, SCALE, UNIT or MODID (which default to 0, 0.0, 0 and -1), so the group does not change meaning when the card's
// defaults are edited, and the writer always writes them.
struct ImperfectionGeometryGroup
{
    std::int32_t subcase = 0;       // the subcase of the result, 0 or above
    std::int32_t step = 0;          // the step of the result, 0 or above
    std::int32_t modeIncrement = 0; // the mode (buckling, modal) or the increment (static), 0 or above
    base::EntityId nodeSetId = 0;   // a SET1 or SET3 of grid points, 0 for every grid point
    double scaleFactor = 0.0;       // the scaleFactor factor of the variation
    std::int32_t fileUnit = 0;      // the file unit, 0 or above (it must be given in the end)
    std::int32_t moduleId = IMPERFECTION_WHOLE_MODEL; // the module, -1 or above

    [[nodiscard]] bool operator==(const ImperfectionGeometryGroup&) const noexcept = default;
};

// One IMPGEOM card (SOL 400): the definition of a geometric imperfection, an ID that an IMPCASE (and the Case Control
// IMPERFECT) names. The imperfection is the sum of the variations that its groups select, applied to the coordinates of
// the grid points of the residual structure.
//
// Card layout: IMPGEOM  IMPFID  (blank)  (blank)  SETID  SCALE  (blank)  UNIT  MODID          fields 1 to 8
//                      .  SUBCASE1  STEP1  MODEINC1  SETID1  SCALE1  (blank)  UNIT1  MODID1  one line for each group
//                      .  SUBCASE2  ...
// In this library's numbering (field 1 is IMPFID): SETID is field 4, SCALE 5, UNIT 7 and MODID 8 (fields 2, 3 and 6 are
// not used and must be blank), and the groups are the continuation lines, eight fields each from field 9 (SUBCASEi
// first). A group is a whole line; a line with nothing on it is a group of defaults.
//
// IMPFID is the card's own number: IdSpace::PerCardType, category ImperfectionGeometry, unique and what the
// IMPFIDi of an IMPCASE and the Case Control IMPERFECT name. The card has at least one group, on the continuation line.
// The SETIDs name a SET1 or a SET3 of grid points (ReferenceTarget::Set; a zero is blank and names nothing); the card's
// own SETID is visited too, so that a renumbering of the sets moves the default with the groups that took it. The
// module numbers are plain integers: a module is a BEGIN MODULE section that the library keeps as text. UNIT is the
// number of an ASSIGN statement and no card names it.
//
// Two rules of the format have to be read together: SETIDi, SCALEi, UNITi and MODIDi default to 0, 0.0, 0 and -1, and
// SETID, SCALE, UNIT and MODID also serve as their defaults. The card reads them as one rule: a blank field of a group
// is the value of the card's field, which is itself 0, 0.0, 0 or -1 when the card leaves it blank.
struct ImperfectionGeometryCard
{
    base::EntityId id = 0;
    base::EntityId nodeSetId = 0;                     // the default of SETIDi, 0 or above
    double scaleFactor = 0.0;                         // the default of SCALEi
    std::int32_t fileUnit = 0;                        // the default of UNITi, 0 or above
    std::int32_t moduleId = IMPERFECTION_WHOLE_MODEL; // the default of MODIDi, -1 or above
    std::vector<ImperfectionGeometryGroup> groups;    // at least one

    [[nodiscard]] bool operator==(const ImperfectionGeometryCard&) const noexcept = default;
};

} // namespace oor::model
