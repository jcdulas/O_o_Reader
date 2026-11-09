#pragma once

#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One MDBCATP card: the parameters that detect contact pairs between bodies that lie in different modules when the
// contact is generated automatically (Case Control BCONTACT=AUTO, SOLs 101 and 400), as (name, value) pairs. The format
// reference files it among the node entries; it is a contact card, the counterpart of BCAUTOP for several modules, and
// every parameter of BCAUTOP that concerns single bodies is left out.
//
// Card layout: MDBCATP  (blank)  (blank)  Param1  Value1  Param2  Value2  Param3  Value3 / Param4  Value4 ...
// The first pair is in field 3, as in BCAUTOP, and the pairs go on in the continuation lines (the second line of the
// format reference drawing shows Parami and Valuei two cells to the right: blank pairs are skipped, so both drawings
// read the same, and the card writes the pairs one after the other from field 9). Fields 1 and 2 are blank: the format
// lists no ID.
//
// The names the format lists are CTYPE (TOUCH, PGLUE, SGLUE or GGLUE), DISTANCE, BEAMCNT, EDGECNT, FTYPE, FRIC, IGNTHK,
// INISTF and SELFCONT (the format writes SelfCont), and it adds that every parameter of BCPARA, BCONPRP and BCONPRG may
// be given. a common deck uses DELSLD, which the format does not list: it is read as a word (YES or NO) so that such a
// deck reads. The card keeps what the deck wrote, as BCAUTOP does; only one MDBCATP is allowed in a deck.
struct ModuleContactDetectParamsCard
{
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model
