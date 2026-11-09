#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The letters of the response type of a TPADEFN response: D displacement, V velocity, A acceleration and, for the grid
// points of a fluid, P pressure. A type is any combination of them (DA, for
// one).
inline constexpr std::string_view TRANSFER_PATH_RESPONSE_LETTERS = "DVAP";

// The response type a blank RESPTYPi means: D, the displacement.
inline constexpr std::string_view TRANSFER_PATH_DEFAULT_RESPONSE = "D";

// One response of a TPADEFN: what is recorded (RESPTYPi) at the grid points of a set (OUTSETi).
struct TransferPathSetupResponse
{
    std::string type{
        TRANSFER_PATH_DEFAULT_RESPONSE}; // upper case letters of TRANSFER_PATH_RESPONSE_LETTERS, resolved when blank
    base::EntityId outputSet = 0;        // a SET3 of the response grid points, required

    [[nodiscard]] bool operator==(const TransferPathSetupResponse&) const noexcept = default;
};

// One TPADEFN card: the set-up of a transfer path analysis (TPA). The analysis looks at the paths through the grid
// points of the interface (GRIDSET, a SET3) and at the responses at the grid points of the response sets (OUTSETi); the
// Case Control TPASET selects the card by its SETID. The format reference says the card is required to run a
// single-step TPA.
//
// Card layout: TPADEFN  SETID  GRIDSET  UNIT
//                      .  RESPTYP1  OUTSET1  RESPTYP2  OUTSET2  -etc.-
// In this library's numbering (field 1 is SETID): GRIDSET is field 2, UNIT field 3 (fields 4 to 8 are not used and must
// be blank), and the responses are pairs from field 9, four to a line: RESPTYPi in the first field of the pair, OUTSETi
// in the second. A pair with both fields blank is padding. UNIT is the number of a Fortran unit for internal use, above
// zero, 24 when blank: its default is static, so it is written blank. A blank RESPTYPi is D, stored resolved and
// written blank (a static default too); an OUTSETi is required, so there is at least one response.
//
// The ID is the card's own number (the format calls it SUBID in the description and SETID in the layout):
// IdSpace::PerCardType, category TransferPathSet, which the Case Control TPASET names. GRIDSET and the OUTSETs name a
// SET3 (ReferenceTarget::Set). The UNIT is a number of the solver, not an ID of any card.
struct TransferPathSetupCard
{
    static constexpr std::int32_t DEFAULT_UNIT = 24;

    base::EntityId id = 0;
    base::EntityId nodeSetId = 0;
    std::int32_t unit = DEFAULT_UNIT;
    std::vector<TransferPathSetupResponse> responses; // at least one

    [[nodiscard]] bool operator==(const TransferPathSetupCard&) const noexcept = default;
};

} // namespace oor::model
