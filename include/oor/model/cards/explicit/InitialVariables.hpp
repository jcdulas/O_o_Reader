#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The one value that TICEL and TICVAL share: the list of (variable name, value) pairs that gives the initial state of
// the elements of a SOL 700 model (density, specific internal energy, velocity, ...). The two cards say where the list
// starts, what comes before it and what the names refer to; this header only reads, writes, checks and visits the list.
namespace oor::model
{

// What the values of the list are. TICEL always gives numbers. A TICVAL of METHOD = RADIAL gives, instead of a number,
// the ID of the TABLED1 that holds the radial profile of the variable; the exceptions are the
// coordinates of the centre of the sphere and the decay coefficient of the radial velocity, which stay numbers.
enum class InitialValueMode : std::uint8_t
{
    Numbers,
    RadialProfiles
};

// The names that stay numbers in a radial list: X-CENTER, Y-CENTER, Z-CENTER (the centre of the sphere, default 0, 0,
// 0) and DECAY (the decay coefficient of the radial velocity: the format of TICVAL does not mention it, and
// a coefficient is not a profile). R-VEL, SIE and DENSITY name a table there; so does any other variable, since the
// format says that the values of a radial list are TABLED1 IDs.
[[nodiscard]] bool isRadialConstant(std::string_view name) noexcept;

// True when the value of the variable `name` is the ID of a TABLED1 in a list of this mode.
[[nodiscard]] bool holdsTable(InitialValueMode mode, std::string_view name) noexcept;

// One variable of the list. The name is kept in capitals. The value is a number, or (when `holdsTable` says so) the ID
// of a TABLED1 kept as a whole number: the single member means that a list never has to be converted when its mode
// changes, and the check says whether the values suit the mode.
struct InitialVariable
{
    std::string name;
    double value = 0.0;

    [[nodiscard]] bool operator==(const InitialVariable&) const noexcept = default;
};

// Reads the pairs from `firstField` on (the name in an odd position of the pair, the value in the next field). A pair
// with no name and no value is padding: it ends the list, and a pair after it is an error, as the format reference says
// that a blank field terminates the list. The names are not matched against the variables of the format: the format
// reference lists the ones of the Eulerian regions, and a card with another name is kept as written.
[[nodiscard]] std::vector<InitialVariable> readInitialVariables(FieldReader& reader, int firstField,
                                                                InitialValueMode mode);

// Writes the pairs from `firstField` on.
void writeInitialVariables(FieldWriter& writer, int firstField, InitialValueMode mode,
                           const std::vector<InitialVariable>& variables);

// The first problem of the list: no pair, a name that does not read back, a value that is not a finite number, the ID
// of a table that is not a positive whole number.
[[nodiscard]] std::optional<CardProblem> checkInitialVariables(const std::vector<InitialVariable>& variables,
                                                               InitialValueMode mode);

// Visits the TABLED1 of every value that is a table.
void visitInitialTables(std::vector<InitialVariable>& variables, InitialValueMode mode, ReferenceVisitor& visitor);

} // namespace oor::model
