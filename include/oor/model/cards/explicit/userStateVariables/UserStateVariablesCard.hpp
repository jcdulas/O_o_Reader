#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// The user name of one state variable: the number i of the variable (the card writes it as the default name SVi, which
// stands in the first field of its pair) and the name the user gives it (SVi_NAME).
struct UserStateVariablesName
{
    std::int32_t index = 0;
    std::string name;

    [[nodiscard]] bool operator==(const UserStateVariablesName&) const noexcept = default;
};

// One UDSESV card (the format says SOL 400 only): the number and the names of the user state variables that material
// user subroutines (UMAT and UCOHES) keep. It is a global entry: it applies to the whole model, and a MATDIGI needs it.
// No other entry of the format reference has this name.
//
// Card layout: UDSESV  (blank)  NSTATS                                       fields 1 and 2 (field 1 is blank)
//             +       SV2  SV2_NAME  SV3  SV3_NAME  SV4  SV4_NAME ...        fields 9 on, two to a pair, any number
// NSTATS is the number of state variables, the temperature included, from 1 to 100. The temperature is
// always the first one (its name should not be redefined); the format lets the pairs start at SV1 and the card
// does not refuse it. Each pair is the default name SVi, which gives the number i, and the name the user wants for that
// variable (at most eight characters); a variable with no pair keeps the name SVi, and a pair whose name is blank gives
// it the name SVi, which the card stores as it is read. The names are the keywords of the output selection of NLOUT. A
// number appears once and cannot exceed NSTATS. The names are kept in capitals. The format also says the entry is
// global; the card does not check that a deck has one only.
struct UserStateVariablesCard
{
    static constexpr std::int32_t MAX_STATE_VARIABLES = 100;

    std::int32_t variableCount = 1;
    std::vector<UserStateVariablesName> names;
};

} // namespace oor::model
