#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/control/executive_control/ExecutiveControl.hpp"
#include "oor/model/control/file_management/FileManagement.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oor::model
{

// Where the records of a model file go while it is written (ModelFileWriter): the control sections and the cards hand
// their records, comments and Include markers to it in file order.
class RecordOutput
{
public:
    virtual ~RecordOutput() = default;
    // Comment lines (joined with '\n'), written before the next record.
    virtual void comments(const std::string& text) = 0;
    virtual void record(const std::vector<std::string>& values) = 0;
    // The records of the model's file `fileIndex` (Model::files) follow, up to includeEnd.
    virtual void includeBegin(std::uint32_t fileIndex) = 0;
    virtual void includeEnd() = 0;
    // Something of the model that a model file has no place for (a statement of another program, text kept raw).
    virtual void dropped(const std::string& what) = 0;
};

// The records of the [executive] and [case] sections of a model file (docs/model-format.md), read into the structures
// the deck parsers fill (ExecutiveControl, CaseControl) and written from them.
//
// Executive: a statement is `Name;value;...`: the neutral name of a statement of the catalogue (executiveStatements),
// then its words (the solution, the job names, the time, the diagnostics) and its parameters, each one field written
// `name`, `name=value` or `name(options)=value`.
//
// Case: a command is `Name;value;option;option=value`: the neutral name of the command, its value as one field (a list
// with commas: 1,2,3), then its options by their neutral names. A set is `SetDefinition;id;member;member;...` (THRU
// and EXCEPT are members of their own), a parameter `SubcaseParameter;name;value`. A subcase (SubcaseStart;n, and the
// other section headers) holds the records that follow it, up to the next one.
//
// The words inside values are those the structures hold today (ALL, YES, THRU). Comments become leading comments of
// the statement after them ('$' lines of the deck structure); `Include;path` becomes an INCLUDE marker of the section.
struct ControlRecords
{
    [[nodiscard]] static ExecutiveControl readExecutive(std::span<const deck::ModelItem> items,
                                                        const std::vector<std::string>& fileNames,
                                                        base::DiagnosticSink& sink, FileManagement& files);
    [[nodiscard]] static CaseControl readCase(std::span<const deck::ModelItem> items,
                                              const std::vector<std::string>& fileNames, base::DiagnosticSink& sink);

    static void writeExecutive(const ExecutiveControl& control, RecordOutput& out);
    // The FileAssignment statements of a File Management, as records of the [executive] section; the other statements
    // of the section (the database statements of another program) are dropped.
    static void writeFileAssignments(const FileManagement& files, RecordOutput& out);
    static void writeCase(const CaseControl& control, RecordOutput& out);
};

} // namespace oor::model
