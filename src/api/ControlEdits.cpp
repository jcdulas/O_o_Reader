#include "oor/api/ControlEdits.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/model/control/ControlRecords.hpp"
#include "oor/model/control/case_control/CaseValueParser.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace oor::api
{
namespace
{

void rootControls(const Deck& deck)
{
    if (deck.model()->isPart())
        throw base::UsageError("analysis controls belong to the root model");
}

std::vector<deck::ModelItem> itemsOf(std::span<const ControlRecord> records)
{
    std::vector<deck::ModelItem> items;
    items.reserve(records.size());
    for (const auto& record : records)
    {
        if (record.name.empty() || record.name.find_first_of("\r\n") != std::string::npos)
            throw base::UsageError("a control record needs a name with no line break");
        deck::ModelItem item;
        item.values.push_back(record.name);
        for (const auto& field : record.fields)
        {
            if (field.find_first_of("\r\n") != std::string::npos)
                throw base::UsageError("a control field cannot contain a line break");
            item.values.push_back(field);
        }
        item.comments = record.comment;
        item.line = static_cast<std::uint32_t>(items.size() + 1);
        items.push_back(std::move(item));
    }
    return items;
}

void requireValid(const base::DiagnosticSink& sink)
{
    if (const auto error = sink.firstError())
        throw base::UsageError(error->message);
}

void checkEntry(const model::CaseEntry& entry)
{
    if (entry.kind != model::EntryKind::Command && entry.kind != model::EntryKind::Output)
        return;
    const auto* definition = model::CommandCatalogue::instance().findByName(entry.name);
    if (const auto problem = model::CaseValueParser::check(*definition, entry.value))
        throw base::UsageError(problem->message);
    if (entry.kind == model::EntryKind::Output && entry.value.kind == model::CaseValueKind::Keyword &&
        entry.value.text != "All" && entry.value.text != "None")
        throw base::UsageError("an output selects All, None or a positive set ID");
}

std::vector<model::CaseEntry> parseCommands(std::span<const ControlRecord> records)
{
    for (const auto& record : records)
    {
        const auto* definition = model::CommandCatalogue::instance().findByName(record.name);
        if (!definition || definition->shape == model::CommandShape::SubcaseHeader ||
            definition->shape == model::CommandShape::BlockHeader || definition->isBlockCommand)
            throw base::UsageError(record.name +
                                   ": use the section API for section headers; plot blocks are not commands");
    }
    base::DiagnosticSink sink(base::ReadMode::Strict);
    auto parsed = model::ControlRecords::readCase(itemsOf(records), {"(API)"}, sink);
    requireValid(sink);
    for (const auto& entry : parsed.global().entries)
        checkEntry(entry);
    return std::move(parsed.global().entries);
}

model::CaseEntry& put(model::CaseControl& control, model::Subcase& section, const model::CaseEntry& entry)
{
    model::CaseEntry* result = nullptr;
    if (entry.kind == model::EntryKind::Set)
        result = &control.setSet(section, entry.number, entry.set);
    else if (entry.kind == model::EntryKind::Param)
        result = &control.setParam(section, entry.variant, entry.value);
    else
    {
        const auto& definition = *model::CommandCatalogue::instance().findByName(entry.name);
        if (entry.kind == model::EntryKind::Output)
            result = &control.setOutput(section, definition, entry.options, entry.value);
        else
            result = &control.setCommand(section, definition, entry.variant, entry.options, entry.value);
    }
    result->leadingComments = entry.leadingComments;
    return *result;
}

void checkNumber(const model::CaseControl& control, model::SubcaseKind kind, std::int64_t id, std::uint32_t parentKey,
                 std::uint32_t except = 0)
{
    if (kind == model::SubcaseKind::AuxiliaryCase)
    {
        if (parentKey != 0 || id != 0)
            throw base::UsageError("an auxiliary case is top-level and has no number");
        return;
    }
    if (id <= 0)
        throw base::UsageError("a case section number must be positive");
    const auto* parent = parentKey ? control.findSubcase(parentKey) : nullptr;
    if (parentKey && !parent)
        throw base::UsageError("the parent section no longer exists");
    const bool child = kind == model::SubcaseKind::Step || kind == model::SubcaseKind::Substep;
    if (child != (parent != nullptr) ||
        (parent && ((kind == model::SubcaseKind::Step && parent->kind != model::SubcaseKind::Subcase) ||
                    (kind == model::SubcaseKind::Substep && parent->kind != model::SubcaseKind::Subcase &&
                     parent->kind != model::SubcaseKind::Step))))
        throw base::UsageError("a step belongs to a subcase; a substep belongs to a subcase or a step");
    const auto& siblings = parent ? parent->children : control.segments();
    // Separate auxiliary models may reuse subcase numbers. Top-level additions belong to the last auxiliary model.
    std::size_t first = 0;
    std::size_t last = siblings.size();
    if (!parent)
    {
        std::size_t position = siblings.size();
        if (except)
            for (std::size_t i = 0; i < siblings.size(); ++i)
                if (siblings[i].key == except)
                    position = i;
        for (std::size_t i = 0; i < siblings.size(); ++i)
            if (siblings[i].kind == model::SubcaseKind::AuxiliaryCase)
            {
                if (i <= position)
                    first = i;
                else
                {
                    last = i;
                    break;
                }
            }
    }
    for (std::size_t i = first; i < last; ++i)
        if (siblings[i].key != except && siblings[i].kind == kind && siblings[i].id == id)
            throw base::UsageError("that section number is already used in this scope");
}

// Full replacement uses one pass per sibling list, with separate number spaces after auxiliary headers.
void checkSections(std::span<const model::Subcase> sections, std::optional<model::SubcaseKind> parent = {})
{
    using Kind = model::SubcaseKind;
    std::set<std::pair<Kind, std::int64_t>> numbers;
    for (const auto& section : sections)
    {
        if (section.kind == Kind::AuxiliaryCase)
            numbers.clear();
        const bool child = section.kind == Kind::Step || section.kind == Kind::Substep;
        if (child != parent.has_value() ||
            (parent && ((section.kind == Kind::Step && *parent != Kind::Subcase) ||
                        (section.kind == Kind::Substep && *parent != Kind::Subcase && *parent != Kind::Step))))
            throw base::UsageError("invalid case section nesting");
        if (section.kind != Kind::Global && section.kind != Kind::AuxiliaryCase)
        {
            if (section.id <= 0 || !numbers.emplace(section.kind, section.id).second)
                throw base::UsageError("case section numbers must be positive and unique in their scope");
        }
        for (const auto& entry : section.entries)
            checkEntry(entry);
        if (!section.children.empty())
            checkSections(section.children, section.kind);
    }
}

class RecordCollector final : public model::RecordOutput
{
public:
    std::vector<ControlRecord> records;
    std::string pending;
    void comments(const std::string& text) override { pending += (pending.empty() ? "" : "\n") + text; }
    void record(const std::vector<std::string>& values) override
    {
        records.push_back({values.front(), {values.begin() + 1, values.end()}, std::exchange(pending, {})});
    }
    void includeBegin(std::uint32_t) override {}
    void includeEnd() override {}
    void dropped(const std::string&) override {}
};

} // namespace

void setExecutive(const Deck& deck, std::span<const ControlRecord> records)
{
    rootControls(deck);
    base::DiagnosticSink sink(base::ReadMode::Strict);
    model::FileManagement files;
    auto executive = model::ControlRecords::readExecutive(itemsOf(records), {"(API)"}, sink, files);
    requireValid(sink);
    model::ExecutiveControl check;
    for (const auto& entry : executive.entries())
    {
        if (entry.kind == model::ControlKind::TimeLimit)
            check.setTimeLimit(entry.words);
        else if (entry.kind == model::ControlKind::JobIdentifier)
            check.setJobIdentifier(entry.words);
        else if (entry.kind == model::ControlKind::SolutionSequence)
            check.setSolution(entry.words.front());
        else if (entry.kind == model::ControlKind::DiagnosticSelection)
            for (const auto& word : entry.words)
                if (!base::NumberParser::parseInteger(word))
                    throw base::UsageError("DiagnosticSelection takes integers");
    }
    auto& control = deck.model()->control();
    control.executive.replaceWith(executive);
    control.fileManagement.replaceWith(files);
}

std::vector<ControlRecord> executiveRecords(const Deck& deck)
{
    rootControls(deck);
    const auto& control = std::as_const(*deck.model()).control();
    RecordCollector collector;
    model::ControlRecords::writeFileAssignments(control.fileManagement, collector);
    model::ControlRecords::writeExecutive(control.executive, collector);
    return std::move(collector.records);
}

void setSolution(const Deck& deck, const std::string& solution)
{
    rootControls(deck);
    // Validate on a small temporary section before obtaining mutable control data.
    model::ExecutiveControl check;
    check.setSolution(solution);
    deck.model()->control().executive.setSolution(solution);
}

void setCaseControl(const Deck& deck, std::span<const ControlRecord> records)
{
    rootControls(deck);
    base::DiagnosticSink sink(base::ReadMode::Strict);
    const auto control = model::ControlRecords::readCase(itemsOf(records), {"(API)"}, sink);
    requireValid(sink);
    checkSections(control.segments());
    deck.model()->control().caseControl.replaceWith(control);
}

std::vector<ControlRecord> caseRecords(const Deck& deck)
{
    rootControls(deck);
    RecordCollector collector;
    model::ControlRecords::writeCase(std::as_const(*deck.model()).control().caseControl, collector);
    return std::move(collector.records);
}

const model::Subcase& caseSection(const Deck& deck, std::uint32_t key)
{
    rootControls(deck);
    const auto* section = std::as_const(*deck.model()).control().caseControl.findSubcase(key);
    if (!section)
        throw base::UsageError("the case section no longer exists");
    return *section;
}

std::uint32_t addCaseSection(const Deck& deck, const std::string& kindName, std::int64_t id, std::uint32_t parentKey,
                             std::span<const ControlRecord> commands)
{
    rootControls(deck);
    const auto kind = model::subcaseKindOf(kindName);
    if (!kind || *kind == model::SubcaseKind::Global)
        throw base::UsageError("unknown or uncreatable case section kind: " + kindName);
    const auto entries = parseCommands(commands);
    checkNumber(std::as_const(*deck.model()).control().caseControl, *kind, id, parentKey);
    auto& control = deck.model()->control().caseControl;
    model::Subcase& section = parentKey                                    ? control.addChild(parentKey, *kind, id)
                              : *kind == model::SubcaseKind::AuxiliaryCase ? control.addAuxiliaryCase()
                                                                           : control.addSubcase(*kind, id);
    for (const auto& entry : entries)
        put(control, section, entry);
    return section.key;
}

void renumberCaseSection(const Deck& deck, std::uint32_t key, std::int64_t id)
{
    rootControls(deck);
    const auto& current = std::as_const(*deck.model()).control().caseControl;
    const auto& section = caseSection(deck, key);
    if (section.kind == model::SubcaseKind::Global || section.kind == model::SubcaseKind::AuxiliaryCase)
        throw base::UsageError("this section has no number");
    const auto* parent = current.parentOf(key);
    checkNumber(current, section.kind, id, parent ? parent->key : 0, key);
    auto& control = deck.model()->control().caseControl;
    control.setNumber(*control.findSubcase(key), id);
}

bool removeCaseSection(const Deck& deck, std::uint32_t key)
{
    rootControls(deck);
    return deck.model()->control().caseControl.removeSubcase(key);
}

std::vector<std::uint32_t> setCaseCommands(const Deck& deck, std::uint32_t key, std::span<const ControlRecord> commands)
{
    rootControls(deck);
    (void)caseSection(deck, key);
    const auto entries = parseCommands(commands);
    auto& control = deck.model()->control().caseControl;
    auto& section = *control.findSubcase(key);
    std::vector<std::uint32_t> result;
    result.reserve(entries.size());
    for (const auto& entry : entries)
        result.push_back(put(control, section, entry).key);
    return result;
}

bool removeCaseEntry(const Deck& deck, std::uint32_t key)
{
    rootControls(deck);
    const auto* found = std::as_const(*deck.model()).control().caseControl.findEntry(key);
    if (!found || found->kind == model::EntryKind::Header || found->kind == model::EntryKind::IncludeBegin ||
        found->kind == model::EntryKind::IncludeEnd || found->kind == model::EntryKind::OutputBlock)
        return false;
    return deck.model()->control().caseControl.removeEntry(key);
}

} // namespace oor::api
