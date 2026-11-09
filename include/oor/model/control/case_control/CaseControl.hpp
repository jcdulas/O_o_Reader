#pragma once

#include "oor/model/control/case_control/CommandDefinition.hpp"
#include "oor/model/control/case_control/Subcase.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The Case Control section of a model, typed: the global commands, the subcases and steps in file order, each with its
// own commands.
//
// Inheritance. The commands above the first SUBCASE are the global section, `segments()[0]`. A subcase sees its own
// commands first, then (for a step or substep) those of the subcase or step it is in, then the global ones: the
// effective value of a command is the nearest one that defines it (`effective*` functions); the value defined in the
// subcase itself is its `own*` value. A command replaces the inherited one as a whole: a subcase that has any
// DISPLACEMENT request hides every DISPLACEMENT request of the global section. REPCASE inherits like a SUBCASE.
//
// Auxiliary models. AUXCASE (format reference, SOL 200) opens the Case Control of an auxiliary model after the primary
// one: the commands after it, up to its first SUBCASE, are the top of that model's Case Control (a top-level section of
// kind Auxcase), and the SUBCASEs that follow, up to the next AUXCASE, are its subcases. Nothing of the primary
// sections is inherited there: an auxiliary subcase sees its own commands, then its AUXCASE section, and stops
// (`lineage`).
//
// Writing. The writer emits canonical model records from each entry's typed members, retaining source order and
// attached comments.
//
// Identity. Subcases and entries have a key, assigned when they are created and never reused; Python handles hold
// keys, so they follow their object when others are added or removed. Pointers into the section are valid only until
// the next change of its structure.
class CaseControl
{
public:
    CaseControl();

    // --- structure ---------------------------------------------------------------------------------------------------
    // The top-level sections in file order; the first is always the global section. Steps and substeps are the
    // `children` of a subcase.
    [[nodiscard]] const std::vector<Subcase>& segments() const noexcept { return m_segments; }
    [[nodiscard]] std::vector<Subcase>& segments() noexcept { return m_segments; }
    [[nodiscard]] const Subcase& global() const noexcept { return m_segments.front(); }
    [[nodiscard]] Subcase& global() noexcept { return m_segments.front(); }
    // Nothing but an empty global section: the model has no Case Control to write.
    [[nodiscard]] bool isEmpty() const noexcept;
    // A new key, for a section or an entry.
    [[nodiscard]] std::uint32_t newKey() noexcept { return m_nextKey++; }
    // Becomes a copy of `other`, with new keys for every section and entry: a handle to what this section held is stale
    // afterwards instead of standing for another object.
    void replaceWith(const CaseControl& other);

    // --- lookup ------------------------------------------------------------------------------------------------------
    // The last top-level section is found first (the subcase just added), another one by bisection on the keys, which
    // grow in file order (a section is read, added at the end or rekeyed in order), in a time logarithmic in the
    // subcases; a step, or a key the bisection misses, by walking every section.
    [[nodiscard]] Subcase* findSubcase(std::uint32_t key) noexcept;
    [[nodiscard]] const Subcase* findSubcase(std::uint32_t key) const noexcept;
    // The section that holds the section with this key as a child (a step's subcase); null for a top-level section.
    [[nodiscard]] const Subcase* parentOf(std::uint32_t key) const noexcept;
    // The section with this key, then the ones it inherits from, nearest first, ending with the global section (with
    // the AUXCASE section for a section of an auxiliary model). It walks the sections to find this one: a loop over the
    // subcases asks CaseLineages, made once, instead (as for the effective* functions, which call it).
    [[nodiscard]] std::vector<const Subcase*> lineage(std::uint32_t key) const;
    // The AUXCASE section that the section with this key belongs to (itself for an AUXCASE section, the nearest AUXCASE
    // before it for a subcase or a step of one); null for a section of the primary Case Control.
    [[nodiscard]] const Subcase* auxiliaryCaseOf(std::uint32_t key) const noexcept;

    struct EntryLocation
    {
        Subcase* subcase = nullptr;
        std::size_t index = 0;
    };
    // The section and position of the entry with this key (the header of a section included).
    [[nodiscard]] std::optional<EntryLocation> findEntry(std::uint32_t entryKey) noexcept;
    [[nodiscard]] const CaseEntry* findEntry(std::uint32_t entryKey) const noexcept;
    // The section that holds the entry with this key (the header of a section included); null when there is none.
    [[nodiscard]] const Subcase* ownerOfEntry(std::uint32_t entryKey) const noexcept;

    // The last own command with this name and variant (a command repeated in a subcase: the last one counts).
    [[nodiscard]] static const CaseEntry* ownCommand(const Subcase& subcase, std::string_view name,
                                                     std::string_view variant = {});
    [[nodiscard]] const CaseEntry* effectiveCommand(std::uint32_t subcaseKey, std::string_view name,
                                                    std::string_view variant = {}) const;
    // The own output requests of this name, in file order.
    [[nodiscard]] static std::vector<const CaseEntry*> ownOutputs(const Subcase& subcase, std::string_view name);
    // The requests of this name of the nearest section that has any.
    [[nodiscard]] std::vector<const CaseEntry*> effectiveOutputs(std::uint32_t subcaseKey, std::string_view name) const;
    [[nodiscard]] static const CaseEntry* ownSet(const Subcase& subcase, std::int64_t id);
    [[nodiscard]] const CaseEntry* effectiveSet(std::uint32_t subcaseKey, std::int64_t id) const;
    // The same along a lineage (`lineage`, or CaseLineages::of): the first section of it that has one gives it.
    [[nodiscard]] static const CaseEntry* commandIn(std::span<const Subcase* const> lineage, std::string_view name,
                                                    std::string_view variant = {});
    [[nodiscard]] static std::vector<const CaseEntry*> outputsIn(std::span<const Subcase* const> lineage,
                                                                 std::string_view name);
    [[nodiscard]] static const CaseEntry* setIn(std::span<const Subcase* const> lineage, std::int64_t id);
    [[nodiscard]] static const CaseEntry* ownParam(const Subcase& subcase, std::string_view name);
    [[nodiscard]] static bool sameOptions(const std::vector<std::string>& left, const std::vector<std::string>& right);

    // One command or parameter whose effective value differs between two sections (`differences`).
    struct CommandDifference
    {
        std::string name;    // the command ("SinglePointConstrSelection"), or PARAMETER_COMMAND for a parameter
        std::string variant; // the variant (TEMPERATURE(LOAD): "TemperatureLoadPart"), or the name of a parameter
        std::optional<std::string> first; // the effective value in the first section as text; nothing when absent
        std::optional<std::string> second;
    };
    // The commands and parameters whose effective values differ between two sections (each with what it inherits), by
    // name and variant: what tells two subcases apart beyond their loads (a companion package: a superposition of unit
    // subcases needs the same constraints, temperatures and parameters). The output requests, the sets and the commands
    // named in `ignored` (neutral names: "StaticLoadSelection", "TitleText") are left out. A value is the text of the
    // command: its value, after its options in parentheses when it has any.
    [[nodiscard]] std::vector<CommandDifference> differences(std::uint32_t firstKey, std::uint32_t secondKey,
                                                             const std::vector<std::string>& ignored) const;

    // --- editing -----------------------------------------------------------------------------------------------------
    // A new subcase after the last one. The OUTPUT(PLOT) or OUTPUT(XYPLOT) block at the end of the last subcase (they
    // must follow every subcase) moves to the new one.
    Subcase& addSubcase(SubcaseKind kind, std::int64_t id);
    // What addSubcase changes, kept to undo it: the number of top-level sections, a copy of the last one (whose
    // OUTPUT(PLOT) or OUTPUT(XYPLOT) block moves to the new subcase) and the key counter. It costs what the last
    // section holds, not what the Case Control holds, so that adding a subcase that may be refused (the Python
    // add_subcase and its commands) takes a time independent of the subcases already there.
    struct AdditionPoint
    {
        std::size_t segmentCount = 0;
        Subcase last;
        std::uint32_t nextKey = 0;
    };
    [[nodiscard]] AdditionPoint additionPoint() const;
    // The top-level sections as they were at `point`: those added since are removed, the last one then gets its copy
    // back, and the key counter its value. What was edited since in the other sections stays.
    void restore(AdditionPoint point);
    // A new AUXCASE section after the last section: the subcases added after it belong to its auxiliary model.
    Subcase& addAuxiliaryCase();
    // A new step (or substep) at the end of the subcase or step `parentKey`.
    Subcase& addChild(std::uint32_t parentKey, SubcaseKind kind, std::int64_t id);
    // Removes a subcase or a step. Returns false (and changes nothing) when it holds INCLUDE markers.
    bool removeSubcase(std::uint32_t key);
    // Changes the number of a subcase, step or substep.
    void setNumber(Subcase& subcase, std::int64_t id);

    // Sets the value of a command in a subcase: the last own entry of that name and variant takes it (and stays
    // where it is), or a new entry goes at the end. Nothing changes when the entry already has this value and these
    // options (its original text stays).
    CaseEntry& setCommand(Subcase& subcase, const CommandDefinition& definition, std::string_view variant,
                          std::vector<std::string> options, CaseValue value);
    // An output request: the own request with the same name and the same options takes the selection, or a new one
    // goes at the end.
    CaseEntry& setOutput(Subcase& subcase, const CommandDefinition& definition, std::vector<std::string> options,
                         CaseValue selection);
    CaseEntry& setSet(Subcase& subcase, std::int64_t id, CaseSet members);
    CaseEntry& setParam(Subcase& subcase, std::string_view name, CaseValue value);
    // A statement the library does not know, kept as text, at the end.

    // --- output blocks
    // -------------------------------------------------------------------------------------------------- OUTPUT(PLOT),
    // OUTPUT(POST) and OUTPUT(XYPLOT) are lines in the entries of the subcase they were written in (the last one for
    // PLOT and XYPLOT, which must follow every subcase). The block is that line and the plot commands, SETs and
    // statements the library does not know that follow it, up to the first other command or the next block. The entries
    // of the block after `markerIndex`: [first, last).
    struct Extent
    {
        std::size_t first = 0;
        std::size_t last = 0;
    };
    [[nodiscard]] static Extent blockExtent(const Subcase& subcase, std::size_t markerIndex);
    // A new block at the end of the section last in the file (a subcase, or its last step); `kind` is Plot,
    // Postprocessing or CurvePlot. A command added to a subcase that has a Plot or CurvePlot block goes before the
    // block.
    CaseEntry& addBlock(std::string kind);
    // A plot command by name and argument text, at the end of the block whose line is `markerKey`.
    CaseEntry& addBlockCommand(std::uint32_t markerKey, const CommandDefinition& definition, std::string arguments);
    // A statement kept as text at the end of the block whose line is `markerKey`.
    // Removes the block line and the entries of the block.
    bool removeBlock(std::uint32_t markerKey);
    // The keys of the block lines, in file order.
    [[nodiscard]] std::vector<std::uint32_t> blockKeys() const;

    // Remove entries; each returns whether anything was removed.
    bool removeCommand(Subcase& subcase, std::string_view name, std::string_view variant = {});
    bool removeOutputs(Subcase& subcase, std::string_view name, const std::vector<std::string>* options = nullptr);
    bool removeSet(Subcase& subcase, std::int64_t id);
    bool removeParam(Subcase& subcase, std::string_view name);
    bool removeEntry(std::uint32_t entryKey);

    // --- references --------------------------------------------------------------------------------------------------
    // Called when a new section is entered: its kind and number.
    using ScopeCallback = std::function<void(SubcaseKind kind, std::int64_t number)>;
    // Shows every command that holds a Bulk Data set ID (SPC, LOAD, METHOD, ...) to the visitor, which gives back the
    // ID the command should hold; a changed ID is stored and the entry's original text dropped. Renumbering, reference
    // checks and resolution are visitors, as for the cards.
    void visitReferences(ReferenceVisitor& visitor, const ScopeCallback& onScope = {});

private:
    [[nodiscard]] CaseEntry& append(Subcase& subcase, CaseEntry entry);

    std::vector<Subcase> m_segments;
    std::uint32_t m_nextKey = 1;
};

} // namespace oor::model
