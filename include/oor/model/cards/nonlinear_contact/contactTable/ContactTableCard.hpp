#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One SECNDRY / PRIMARY set of a BCTABLE: a touching (secondary) body, the parameters of the pair, and the bodies it
// touches (primary).
struct ContactTableGroup
{
    base::EntityId secondaryBodyId = 0; // IDSEC1: the touching body; 0 when blank (SOL 700: every element of the model)
    std::vector<ContactParameter> params;      // the named fields of the set (ERROR, FRIC, IGLUE, ...)
    std::vector<base::EntityId> primaryBodies; // IDPRIM1, IDPRIM2, ...: the touched bodies

    [[nodiscard]] bool operator==(const ContactTableGroup&) const = default;
};

// One BCTABLE card: the contact table of a load case, for general contact in SOLs 101 and 400 and in SOL 700, and (with
// the options of the glued contact) for the permanently glued contact of the other solutions. The three format
// reference entries (the general table, its SOL 700 form and the glued option) have the same first line and the same
// lines for the fields they share, so one layout reads them all.
//
// Card layout: BCTABLE  ID  IDSCND  IDPRIM  NGROUP  COPTS  COPTM
//   then for each set of touching and touched bodies, on continuation lines (keywords in field 1 or, for the options,
//   field 2):
//     SECNDRY  IDSEC1  ERROR  FNTOL  FRIC  CINTERF  IGLUE
//              ISEARCH  ICOORD  JGLUE  (blank)  DQNEAR
//     (blank)  FBSH  FRLIM  BIAS  SLIDE  HARDS  COPTS1  COPTM1
//     (blank)  BKGL  BGSN  BGST  BGM  BGN
//     (blank)  SEGS  PENALT  AUGDIST  TPENALT  STKSLP
//     (blank)  HHHB  HCT  HCV  HNC  BNC  EMISS  HBL
//              (blank)  HNL  BNL  HGLUE
//     PRIMARY  IDPRIM1  IDPRIM2 ...  IDPRIM7 / IDPRIM8 ...
//   The SOL 700 form has no keyword lines after SECNDRY: its lines follow it in order, FRIC and IGLUE and JGLUE in the
//   places above and then FK EXP METHOD ADAPT THICK THICKOF PENV / FACT TSTART TEND MAXPAR / IADJ / IGNORE / AUTO /
//   SIDE WEIGHT MONDIS MONDISV INITMON DAMPING.
// IDSCND and IDPRIM are the short form of one touching and one touched body (no continuation line then). A deck may
// also say SLAVE and MASTER (or MASTERS) for SECNDRY and PRIMARY (older names): they are read, and written as SECNDRY
// and PRIMARY.
struct ContactTableCard
{
    base::EntityId id = 0;              // referenced by BCONTACT; 0 is the load case 0
    base::EntityId secondaryBodyId = 0; // the touching body of the short form; 0 when blank
    base::EntityId primaryBodyId = 0;   // the touched body of the short form; 0 when blank
    std::int32_t setCount = 1; // the number of sets (the format reference default is 1); 0: no continuation line
    std::int32_t secondaryContactOption = 0; // how the touching surfaces may contact (applies to every set)
    std::int32_t primaryContactOption = 0;   // the same for the touched surfaces
    std::vector<ContactTableGroup> groups;
};

} // namespace oor::model
