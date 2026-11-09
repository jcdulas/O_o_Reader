#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/InitialVariables.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// METHOD (field 2): how the values of the list are read. A blank METHOD is NORMAL.
enum class InitialConditionValuesMethod : std::uint8_t
{
    Normal, // the values are numbers
    Radial  // the values are the TABLED1 that hold the radial profiles (the map of a 1-D spherical solution)
};
inline constexpr std::array<std::string_view, 2> INITIAL_CONDITION_VALUES_METHOD_WORDS{"Normal", "Radial"};

// One TICVAL card (SOL 700): the initial values of an Eulerian region, a list of variables and the value each starts
// with. A TICREG names it (its second TSID). No other entry of the format reference has this name.
//
// Card layout: TICVAL  TSID  METHOD  NAME1  VALUE1  NAME2  VALUE2  NAME3  VALUE3       fields 1 to 8
//             then, on any number of continuation lines, pairs NAMEi VALUEi           fields 9 on
// (METHOD may be blank, and the pairs go on from the first field of the continuation line). TSID is the number of the
// card, unique among the TICVAL cards. The variables are those of TICEL; a name that is not one
// of them is kept as written, as for a TICEL.
//
// METHOD = RADIAL maps the result of a spherical symmetric 1-D solution onto the 3-D mesh: the
// values are then the IDs of the TABLED1 that give the variable against the distance from the centre. SIE and DENSITY
// need a table, so does the radial velocity R-VEL, and the centre of the sphere may be moved with X-CENTER, Y-CENTER
// and Z-CENTER, which are numbers (InitialVariables.hpp says which names stay numbers). A blank field ends the list, as
// in a TICEL.
struct InitialConditionValuesCard
{
    base::EntityId id = 0; // the number of the card, which a TICREG names
    InitialConditionValuesMethod method = InitialConditionValuesMethod::Normal;
    std::vector<InitialVariable> variables;

    // How the values of the list are read, from the method.
    [[nodiscard]] InitialValueMode valueMode() const noexcept
    {
        return method == InitialConditionValuesMethod::Radial ? InitialValueMode::RadialProfiles
                                                              : InitialValueMode::Numbers;
    }
};

} // namespace oor::model
