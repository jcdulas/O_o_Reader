#pragma once

#include "oor/model/control/case_control/CaseSet.hpp"
#include "oor/model/control/case_control/CaseValue.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class EntryKind : std::uint8_t
{
    Command,      // a valued command or a flag: SPC = 1, TITLE = a plate, SEPLOT 0,1,2, a plot command
    Output,       // an output request: DISPLACEMENT(PLOT,SORT1,REAL) = ALL
    Set,          // SET 10 = 1 THRU 5, 9
    Param,        // PARAM,POST,-1
    Header,       // SUBCASE 1, STEP 2: the line that opens a subcase or a step
    OutputBlock,  // OUTPUT(PLOT), OUTPUT(POST), OUTPUT(XYPLOT): the line that opens a block of plot commands
    Raw,          // opaque command text, omitted by the model writer
    Comment,      // comment lines with no command after them (the end of a section)
    IncludeBegin, // an INCLUDE statement: the statements of the included file follow
    IncludeEnd    // the end of the included file
};

inline constexpr std::string_view SET_COMMAND = "SetDefinition";          // SET n = members
inline constexpr std::string_view PARAMETER_COMMAND = "SubcaseParameter"; // PARAM,NAME,value
inline constexpr std::string_view OUTPUT_BLOCK_COMMAND = "OutputBlock";   // OUTPUT(PLOT), OUTPUT(POST), OUTPUT(XYPLOT)
inline constexpr std::string_view SETS_BLOCK_COMMAND = "SetsBlock";       // SETS DEFINITION
// The neutral names of the commands that select the constraint sets (SPC, MPC) and the static loads (LOAD).
inline constexpr std::string_view SINGLE_POINT_CONSTR_SELECTION = "SinglePointConstrSelection";
inline constexpr std::string_view MULTI_POINT_CONSTR_SELECTION = "MultiPointConstrSelection";
inline constexpr std::string_view LOAD_SELECTION = "StaticLoadSelection";

// One typed statement of the Case Control section. The writer emits a canonical model record from its members.
// Attached comments remain with the statement; `original` does not control serialization.
struct CaseEntry
{
    EntryKind kind = EntryKind::Command;
    // The key of the entry in its CaseControl: unique, never reused, what Python handles hold.
    std::uint32_t key = 0;

    std::string name;
    // The option variant that is part of the command's identity, by its neutral name: "InitialTemperature" for
    // TEMPERATURE(INITIAL) = 1. A Param has the name of the parameter here (its neutral name: "PostprocessingFile").
    // An OutputBlock has the kind of its block, as the deck writes it: PLOT, POST or XYPLOT.
    std::string variant;
    std::vector<std::string> options;
    // Command and Param: the value. Output: the selection (Keyword ALL or NONE, or Integer set ID, or None).
    CaseValue value;
    // Set and Header: the number after the command (SET 10, SUBCASE 3).
    std::int64_t number = 0;
    // Set: the members.
    CaseSet set;

    // Comment text before the statement, joined with newlines without leading # characters.
    std::string leadingComments;
    // Optional comment text emitted with the record, without a leading #.
    std::string trailingComment;
    // Optional source text retained in memory; model output is generated from the typed members.
    std::string original;
    // Raw, Comment, IncludeBegin: the text (lines joined with newlines).
    std::string text;
    // IncludeBegin: the index of the file that is opened (Model::files).
    std::uint32_t includedFileIndex = 0;

    [[nodiscard]] bool isCommandLike() const noexcept
    {
        return kind == EntryKind::Command || kind == EntryKind::Output;
    }
};

} // namespace oor::model
