#pragma once

#include "oor/model/cards/properties/PropertyExtensionWords.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

// What the two nonlinear extensions of the shells and plane elements (PSHLN1 for a PSHELL, PCOMP or PCOMPG; PSHLN2 for
// a PLPLANE) have in common: the words of the format reference tables of element behaviours and integration schemes,
// and the keyword line "Ck BEHk INTk BEHkH INTkH" of the elements with k corner and midside grids. This is the helper
// of one kind of value, the line of a keyword. Nothing here describes the fields of a card: each card lays out its own
// first line, says which keyword lines it has and, for each, which pairs of words the table allows. The analysis word
// (ANAL) and the layout of a line of eight fields are the ones of PropertyExtensionWords.hpp, which the nonlinear
// extensions of the other properties use.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The words
// ---------------------------------------------------------------------------------------------------------------------

// The BEHi and BEHiH codes of the two cards: the thick (DCT) and thin (DCTN) doubly-curved shell and the
// three-dimensional membrane (MB) of a PSHLN1, the plane stress, plane strain, composite, axisymmetric and
// incompressible elements of a PSHLN2. The enumerators follow SHELL_BEHAVIOR_WORDS.
enum class ShellBehavior : std::uint8_t
{
    ThickShell,                 // DCT
    ThinShell,                  // DCTN
    Membrane,                   // MB
    PlaneStress,                // PSTRS
    PlaneStrain,                // PLSTRN
    PlaneStrainComposite,       // COMPS
    AxisymmetricSolid,          // AXSOLID
    AxisymmetricComposite,      // AXCOMP
    IncompressiblePlaneStrain,  // IPS
    IncompressibleAxisymmetric, // IAX
};

inline constexpr std::array<std::string_view, 10> SHELL_BEHAVIOR_WORDS{
    "ThickShell",          "ThinShell",       "Membrane",
    "PlaneStress",         "PlaneStrain",     "PlaneStrainComposite",
    "AxisymSolid",         "AxisymComposite", "IncompressiblePlaneStrain",
    "IncompressibleAxisym"};

// The INTi and INTiH codes of the two cards. The enumerators follow SHELL_INTEGRATION_WORDS.
enum class ShellIntegration : std::uint8_t
{
    Linear,                  // L
    LinearReducedHourglass,  // LRIH: reduced integration with hourglass control (assumed strain)
    LinearTwist,             // LT: linear with twist
    LinearDiscreteKirchhoff, // LDK
    Quadratic,               // Q
    QuadraticReduced,        // QRI
    QuadraticTwist,          // QT: quadratic with twist
};

inline constexpr std::array<std::string_view, 7> SHELL_INTEGRATION_WORDS{
    "Linear",    "LinearReducedHourglass", "LinearTwist",   "LinearDiscreteKirchhoff",
    "Quadratic", "QuadraticReduced",       "QuadraticTwist"};

// ---------------------------------------------------------------------------------------------------------------------
// The keyword line
// ---------------------------------------------------------------------------------------------------------------------

// A behaviour and the integration scheme that goes with it: a row of the tables of behaviours and integration
// schemes of PSHLN1 and PSHLN2.
struct ShellWords
{
    ShellBehavior behavior = ShellBehavior::ThickShell;
    ShellIntegration integration = ShellIntegration::Linear;

    [[nodiscard]] constexpr bool operator==(const ShellWords&) const noexcept = default;
};

// The four words of a keyword line: the structural behaviour and integration, and the heat ones.
struct ShellLine
{
    ShellWords structural;
    ShellWords heat;

    [[nodiscard]] constexpr bool operator==(const ShellLine&) const noexcept = default;
};

// What a card allows on the line of one keyword: the names of the fields (for the diagnostics) and the rows of the
// format reference tables for the elements of this keyword (the structural table and the heat table), in the order of
// the format reference. The first row of each table is the default of the line, the one the format reference gives as
// the default of its four fields, and the one a line that the card leaves out gets. A word is allowed on the line when
// a row of its table has it. The tables are static constants written next to the card.
struct ShellLineRules
{
    const char* keyword = "";             // "C8"
    const char* behaviorName = "";        // "behavior8"
    const char* integrationName = "";     // "integration8"
    const char* heatBehaviorName = "";    // "heatBehavior8"
    const char* heatIntegrationName = ""; // "heatIntegration8"
    std::span<const ShellWords> structural{};
    std::span<const ShellWords> heat{};
};

// The line of a keyword the card does not give: the first row of each table.
[[nodiscard]] constexpr ShellLine defaultLine(const ShellLineRules& rules) noexcept
{
    return ShellLine{rules.structural.front(), rules.heat.front()};
}

template <std::size_t COUNT>
[[nodiscard]] constexpr std::array<ShellLine, COUNT> defaultLines(const std::array<ShellLineRules, COUNT>& rules)
{
    std::array<ShellLine, COUNT> lines{};
    for (std::size_t index = 0; index < COUNT; ++index)
        lines[index] = defaultLine(rules[index]);
    return lines;
}

// The words a deck (or an `add_*` call) gives to a line; a word that is not there is blank.
struct ShellLineGiven
{
    std::optional<ShellBehavior> behavior;
    std::optional<ShellIntegration> integration;
    std::optional<ShellBehavior> heatBehavior;
    std::optional<ShellIntegration> heatIntegration;
};

// The line that blank words resolve to. A blank behaviour is the default behaviour of the line; a blank integration is
// the first integration the table lists for the behaviour in force (DCT on the eight-node quadrilateral takes QRI, MB
// takes Q: the table pairs a behaviour with its schemes, and the format reference gives one default per field only), or
// the default of the line when the table has no row for that behaviour (the card then refuses the behaviour). The
// reader and the Python `add_*` methods use this one function, so that they cannot differ.
[[nodiscard]] ShellLine resolveShellLine(const ShellLineRules& rules, const ShellLineGiven& given) noexcept;

// The position of the keyword `text` (without regard to the case) among the rules, if there is one.
[[nodiscard]] std::optional<std::size_t> findShellLineRules(std::span<const ShellLineRules> rules,
                                                            std::string_view text) noexcept;

// A card whose continuation lines from `firstField` on are all keyword lines (PSHLN1 and PSHLN2): the lines may come in
// any order and may be missing (the line keeps its default), blank lines are padding, a keyword may not be given twice.
// A word that the line does not allow is an error that names its field. `lines` has one entry for each of `rules`, in
// the same order.
void readShellLines(FieldReader& reader, int firstField, std::span<const ShellLineRules> rules,
                    std::span<ShellLine> lines);

// Writes the lines that need writing, one after the other from `firstField`, in the order of the rules; returns the
// next free field. A line that is the default is left out unless the writer keeps defaults; a line that is written has
// its four words, because the default integration depends on the behaviour and so is never left blank.
int writeShellLines(FieldWriter& writer, int firstField, std::span<const ShellLineRules> rules,
                    std::span<const ShellLine> lines);

// The first word that a line does not allow, if there is one. A word is allowed when some row of the table of its line
// has it; whether the two words of a line make a row is left to the solver.
[[nodiscard]] std::optional<CardProblem> checkShellLines(std::span<const ShellLineRules> rules,
                                                         std::span<const ShellLine> lines);

} // namespace oor::model
