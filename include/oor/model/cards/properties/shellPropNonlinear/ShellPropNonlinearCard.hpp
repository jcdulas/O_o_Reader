#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/ShellExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The rows of the tables of the format reference for each keyword line, from the elements
// the keyword names: C3 the triangle with three grids, C4 the quadrilateral with four, C6 the triangle with six, C8 the
// quadrilateral with eight. A row pairs a behaviour with an integration scheme; the first row of each list is the
// default of the line (the underlined one of the format reference, which is also the default of its BEHi and INTi
// fields). The structural table has the thick shell DCT (L and LRIH on the four-node quadrilateral, QRI on the
// eight-node one), the thin shell DCTN (LDK on the triangle and the quadrilateral) and the membrane MB (L on the three-
// and four-node elements, Q on the six- and eight-node ones); the heat table has DCT with L or Q, and MB. The format
// reference lists no DCT row for a triangle with three grids.
inline constexpr std::array<ShellWords, 2> SHELL_PROP_NONLINEAR_C3_STRUCTURAL{
    ShellWords{ShellBehavior::ThinShell, ShellIntegration::LinearDiscreteKirchhoff},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 4> SHELL_PROP_NONLINEAR_C4_STRUCTURAL{
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::Linear},
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::LinearReducedHourglass},
    ShellWords{ShellBehavior::ThinShell, ShellIntegration::LinearDiscreteKirchhoff},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 1> SHELL_PROP_NONLINEAR_C6_STRUCTURAL{
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Quadratic}};
inline constexpr std::array<ShellWords, 2> SHELL_PROP_NONLINEAR_C8_STRUCTURAL{
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::QuadraticReduced},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Quadratic}};

inline constexpr std::array<ShellWords, 2> SHELL_PROP_NONLINEAR_C3_HEAT{
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::Linear},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 2> SHELL_PROP_NONLINEAR_C4_HEAT{
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::Linear},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 1> SHELL_PROP_NONLINEAR_C6_HEAT{
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Quadratic}};
inline constexpr std::array<ShellWords, 2> SHELL_PROP_NONLINEAR_C8_HEAT{
    ShellWords{ShellBehavior::ThickShell, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::Membrane, ShellIntegration::Quadratic}};

// The keyword lines of the card, in the order of `ShellPropNonlinearCard::lines`.
inline constexpr std::size_t SHELL_PROP_NONLINEAR_C3 = 0;
inline constexpr std::size_t SHELL_PROP_NONLINEAR_C4 = 1;
inline constexpr std::size_t SHELL_PROP_NONLINEAR_C6 = 2;
inline constexpr std::size_t SHELL_PROP_NONLINEAR_C8 = 3;

inline constexpr std::array<ShellLineRules, 4> SHELL_PROP_NONLINEAR_LINES{
    ShellLineRules{.keyword = "ThreeNodeElems",
                   .behaviorName = "behavior3",
                   .integrationName = "integration3",
                   .heatBehaviorName = "heatBehavior3",
                   .heatIntegrationName = "heatIntegration3",
                   .structural = SHELL_PROP_NONLINEAR_C3_STRUCTURAL,
                   .heat = SHELL_PROP_NONLINEAR_C3_HEAT},
    ShellLineRules{.keyword = "FourNodeElems",
                   .behaviorName = "behavior4",
                   .integrationName = "integration4",
                   .heatBehaviorName = "heatBehavior4",
                   .heatIntegrationName = "heatIntegration4",
                   .structural = SHELL_PROP_NONLINEAR_C4_STRUCTURAL,
                   .heat = SHELL_PROP_NONLINEAR_C4_HEAT},
    ShellLineRules{.keyword = "SixNodeElems",
                   .behaviorName = "behavior6",
                   .integrationName = "integration6",
                   .heatBehaviorName = "heatBehavior6",
                   .heatIntegrationName = "heatIntegration6",
                   .structural = SHELL_PROP_NONLINEAR_C6_STRUCTURAL,
                   .heat = SHELL_PROP_NONLINEAR_C6_HEAT},
    ShellLineRules{.keyword = "EightNodeElems",
                   .behaviorName = "behavior8",
                   .integrationName = "integration8",
                   .heatBehaviorName = "heatBehavior8",
                   .heatIntegrationName = "heatIntegration8",
                   .structural = SHELL_PROP_NONLINEAR_C8_STRUCTURAL,
                   .heat = SHELL_PROP_NONLINEAR_C8_HEAT}};

// One PSHLN1 card (SOL 400): the nonlinear extension of the PSHELL, PCOMP or PCOMPG with the same PID, for the shell
// elements (CQUAD4, CQUADR, CQUAD8, CTRIA3, CTRIAR, CTRIA6) that name it. It also gives the implicit heat transfer
// shell elements their formulation (ANAL = IH or ISH): the structural and the heat use are one card. It is honoured in
// SOL 400 only.
//
// Card layout:
//   PSHLN1  PID    MID1    MID2     (blank)  ANAL
//           "C3"   BEH3    INT3     BEH3H    INT3H          one line for each keyword,
//           "C4"   BEH4    INT4     BEH4H    INT4H          fields 9 to 13 of its line
//           "C6"   BEH6    INT6     BEH6H    INT6H
//           "C8"   BEH8    INT8     BEH8H    INT8H
//
// The ID is the one of the property it extends, so it is in the space of its own card type (one PSHLN1 for each
// property) and is a reference to a property (a rule between cards says it is a PSHELL, PCOMP or PCOMPG). MID1 and MID2
// are the membrane and bending materials: a blank or 0 keeps the ones of the PSHELL, a value above 0 replaces them, and
// both are ignored for a PCOMP or PCOMPG. They name a structural material, or a MAT4 or MAT5 for the
// heat analysis (the Material target falls back to those). ANAL is IS, IH or ISH, the default.
//
// The keyword lines may come in any order or not at all; a line that is not in the card gets the
// defaults of its keyword, so `lines` always holds the four lines resolved and a line is written only when a word of it
// is not the default. A blank behaviour is the default of the line; a blank integration is the first scheme the table
// lists for the behaviour (see resolveShellLine), so a deck that gives DCTN alone gets LDK and one that gives MB alone
// gets L or Q.
struct ShellPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId1 = 0; // 0: the MID1 of the PSHELL
    base::EntityId matId2 = 0; // 0: the MID2 of the PSHELL
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<ShellLine, 4> lines = defaultLines(SHELL_PROP_NONLINEAR_LINES);
};

} // namespace oor::model
