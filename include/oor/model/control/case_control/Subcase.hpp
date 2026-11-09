#pragma once

#include "oor/model/control/case_control/CaseEntry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class SubcaseKind : std::uint8_t
{
    Global,              // the commands above the first SUBCASE (subcase 0)
    Subcase,             // SUBCASE n
    SubcaseCombination,  // SUBCOM n: a combination of subcases (its SUBSEQ gives the coefficients)
    SymmetrySubcase,     // SYM n: a symmetry subcase
    SymmetryCombination, // SYMCOM n: a combination of symmetry subcases (its SYMSEQ gives the coefficients)
    RepeatSubcase,       // REPCASE n: a repeated output request subcase
    Step,                // STEP n: inside a subcase (SOL 400)
    Substep,             // SUBSTEP n: inside a step or a subcase
    // AUXCASE (SOL 200): the top of the Case Control of an auxiliary model, a top-level section with no number. The
    // SUBCASEs after it, up to the next AUXCASE, belong to that auxiliary model (CaseControl::auxiliaryCaseOf).
    AuxiliaryCase
};

// The name of a kind of section in the code and the queries: "global", "subcase", "subcaseCombination", ...
[[nodiscard]] std::string_view subcaseKindName(SubcaseKind kind) noexcept;
// The kind of a name of subcaseKindName; nullopt for another name.
[[nodiscard]] std::optional<SubcaseKind> subcaseKindOf(std::string_view name) noexcept;
// The neutral name of the command that opens a section of this kind ("SubcaseStart" for SUBCASE); empty for the
// global section, which no command opens.
[[nodiscard]] std::string_view subcaseCommandName(SubcaseKind kind) noexcept;
[[nodiscard]] std::string subcaseLabel(SubcaseKind kind);

// One section of the Case Control: the global commands, a subcase, a step or a substep, with its own commands in file
// order. Steps and substeps nest in `children`.
struct Subcase
{
    std::uint32_t key = 0; // unique in the CaseControl, never reused
    SubcaseKind kind = SubcaseKind::Global;
    std::int64_t id = 0; // the number after SUBCASE, STEP, ...; 0 for the global section and an AUXCASE
    // The line that opens it (SUBCASE 1); unused for the global section.
    CaseEntry header;
    std::vector<CaseEntry> entries;
    std::vector<Subcase> children;
};

} // namespace oor::model
