#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <string_view>
#include <vector>

// What the multi-module cards of the first part of the family share (MDBULK, MDLABEL, MDBNDRY, MDEXCLD, MDCONCT,
// MDFSET, MDSSET, MDDMIG, MDBKSYS, MDBOLT, MDBUSH): the module that is a number or the word ALL, the label of a module,
// and the reference to an ID that lives in a module. Each helper moves one kind of value; no helper describes the
// fields of a card. The names carry "Boundary" where a shorter one would be taken by the other groups of the
// multi-module family.
//
// A module is a `BEGIN MODULE=n` section of the deck, a part of the deck that no card defines, so a module number is a
// plain integer: it names no card and is never visited. Module 0 is the main Bulk Data, and a blank module field means
// module 0 (the format reference says so for MIDA of MDBNDRY and MDEXCLD, MODC of MDBUSH and
// MODID of MDCONCT; the other module fields give no default and are read the same way, as MDBCNCT and MDBCTB1 do). The
// IDs that a card lists in a module are the IDs of that module: they are looked up in the main Bulk Data for module 0
// and in the part of the module otherwise (`visitBoundaryId`, `visitBoundaryRuns`, ReferenceVisitor::visitInPart).
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// A module number or the word ALL
// ---------------------------------------------------------------------------------------------------------------------

// The MODID of an MDBULK and the MIDB of an MDBNDRY or MDEXCLD: a module number, or ALL. ALL is a flag, not a number.
struct ModuleOrAll
{
    bool all = false;
    base::EntityId number = 0; // the module, when `all` is false

    [[nodiscard]] static ModuleOrAll everything() noexcept { return ModuleOrAll{true, 0}; }
    [[nodiscard]] static ModuleOrAll module(base::EntityId moduleNumber) noexcept
    {
        return ModuleOrAll{false, moduleNumber};
    }
    // The number the ID index of an MDBULK keeps: 0 for ALL (no module has the number 0 there: MODID is positive).
    [[nodiscard]] base::EntityId idOf() const noexcept { return all ? 0 : number; }
    [[nodiscard]] static ModuleOrAll fromId(base::EntityId id) noexcept { return id == 0 ? everything() : module(id); }

    [[nodiscard]] bool operator==(const ModuleOrAll&) const noexcept = default;
};

// Reads the field: the word ALL in any case, or an integer; a blank field gives `fallback`.
[[nodiscard]] ModuleOrAll readModuleOrAll(FieldReader& reader, int field, const char* name, ModuleOrAll fallback);

// The same for a field that the card requires: a blank field is an error on `name`.
[[nodiscard]] ModuleOrAll readRequiredModuleOrAll(FieldReader& reader, int field, const char* name);

// Writes it. With `blankIsAll` the word ALL is a blank where defaults are left out (MIDB is ALL by default), otherwise
// it is always written.
void writeModuleOrAll(FieldWriter& writer, int field, const ModuleOrAll& value, bool blankIsAll);

// The problem with a module that is a number or ALL, if there is one: a number is positive, or zero where `zeroAllowed`
// (the main Bulk Data).
[[nodiscard]] std::optional<CardProblem> checkModuleOrAll(const ModuleOrAll& value, const char* field,
                                                          bool zeroAllowed);

// The problem with the number of a module, if there is one: it is zero (the main Bulk Data) or positive.
[[nodiscard]] std::optional<CardProblem> checkBoundaryModule(base::EntityId module, const char* field);

// ---------------------------------------------------------------------------------------------------------------------
// The label of a module
// ---------------------------------------------------------------------------------------------------------------------

// The longest label of an MDLABEL: seven fields of eight characters (fields 2 to 8).

// The problem with a label that is written over several fields, if there is one: not empty, at most 56 characters, not
// beginning or ending with a blank (the lexer trims the fields), and no '$', comma, tab or line break (the line would
// not read back as it was written).
[[nodiscard]] std::optional<CardProblem> boundaryLabelProblem(std::string_view label, const char* field);

// ---------------------------------------------------------------------------------------------------------------------
// References to what a module holds
// ---------------------------------------------------------------------------------------------------------------------

// Visits an ID that the card places in `module` as a reference to `target` there (the main Bulk Data for module 0, the
// part of the module otherwise), and stores what the visitor returns. A blank ID (0) is left alone.
inline void visitBoundaryId(ReferenceVisitor& visitor, std::string_view field, ReferenceTarget target,
                            base::EntityId module, base::EntityId& id)
{
    if (id <= 0)
        return;
    id = module == 0 ? visitor.visit(field, target, id)
                     : visitor.visitInPart(field, target, id, "ModuleSection", module);
}

// Visits a list of IDs and THRU ranges of `module`, in place: a single ID with `single`, the two ends of a range with
// `range` (the IDs between them keep their numbers, as for the nodes of a THRU list), each in its module.
void visitBoundaryRuns(ReferenceVisitor& visitor, std::string_view field, base::EntityId module,
                       std::vector<IdRun>& runs, ReferenceTarget single, ReferenceTarget range);

} // namespace oor::model
