#include "oor/model/control/case_control/CaseControl.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"

#include <algorithm>
#include <iterator>
#include <set>
#include <type_traits>

namespace oor::model
{

namespace
{

// Spaces a command is indented by per level of nesting (a subcase's commands, a step's commands, ...).

// The section with this key in a list of sections and their children (List is a vector of Subcase, const or not).
template <typename List>
auto* findIn(List& sections, std::uint32_t key) noexcept
{
    using Section = std::remove_reference_t<decltype(sections.front())>;
    Section* found = nullptr;
    for (auto& section : sections)
    {
        if (section.key == key)
            return &section;
        found = findIn(section.children, key);
        if (found != nullptr)
            return found;
    }
    return found;
}

// The section with this key among the top-level sections: the last one (the subcase just added, which its commands
// and output requests then ask for), else by bisection on their keys (in increasing order when the sections are in
// file order); null when the bisection misses it (a step's key, a stale key, sections out of order).
template <typename List>
auto* bisectTopLevel(List& sections, std::uint32_t key) noexcept
{
    if (!sections.empty() && sections.back().key == key)
        return &sections.back();
    std::size_t low = 0;
    std::size_t high = sections.size();
    while (low < high)
    {
        const std::size_t middle = low + (high - low) / 2;
        if (sections[middle].key < key)
            low = middle + 1;
        else
            high = middle;
    }
    return low < sections.size() && sections[low].key == key ? &sections[low] : nullptr;
}

// The section whose children include the one with this key.
template <typename Section>
Section* findParent(Section& node, std::uint32_t key) noexcept
{
    for (auto& child : node.children)
    {
        if (child.key == key)
            return &node;
        if (Section* found = findParent(child, key))
            return found;
    }
    return nullptr;
}

// Fills `chain` with the sections from `node` down to the one with this key.
bool collectChain(const Subcase& node, std::uint32_t key, std::vector<const Subcase*>& chain)
{
    chain.push_back(&node);
    if (node.key == key)
        return true;
    for (const Subcase& child : node.children)
    {
        if (collectChain(child, key, chain))
            return true;
    }
    chain.pop_back();
    return false;
}

bool holdsIncludes(const Subcase& section)
{
    for (const CaseEntry& entry : section.entries)
    {
        if (entry.kind == EntryKind::IncludeBegin || entry.kind == EntryKind::IncludeEnd)
            return true;
    }
    return std::any_of(section.children.begin(), section.children.end(), holdsIncludes);
}

bool isSetEntry(const CaseEntry& entry, std::int64_t id) noexcept
{
    return entry.kind == EntryKind::Set && entry.number == id;
}

std::vector<std::string> sorted(std::vector<std::string> values)
{
    std::sort(values.begin(), values.end());
    return values;
}

const CaseEntry* findEntryIn(const Subcase& section, std::uint32_t key)
{
    for (const CaseEntry& entry : section.entries)
    {
        if (entry.key == key)
            return &entry;
    }
    for (const Subcase& child : section.children)
    {
        if (const CaseEntry* found = findEntryIn(child, key))
            return found;
    }
    return nullptr;
}

CaseEntry makeHeader(std::string_view name)
{
    CaseEntry header;
    header.kind = EntryKind::Header;
    header.name = std::string(name);
    return header;
}

} // namespace

namespace
{

void rekey(Subcase& section, CaseControl& control)
{
    section.key = control.newKey();
    section.header.key = control.newKey();
    for (CaseEntry& entry : section.entries)
        entry.key = control.newKey();
    for (Subcase& child : section.children)
        rekey(child, control);
}

} // namespace

void CaseControl::replaceWith(const CaseControl& other)
{
    std::vector<Subcase> segments = other.m_segments;
    for (Subcase& segment : segments)
        rekey(segment, *this);
    m_segments = std::move(segments);
}

CaseControl::CaseControl()
{
    Subcase global;
    global.key = newKey();
    global.kind = SubcaseKind::Global;
    m_segments.push_back(std::move(global));
}

bool CaseControl::isEmpty() const noexcept
{
    return m_segments.size() == 1 && m_segments.front().entries.empty() && m_segments.front().children.empty();
}

Subcase* CaseControl::findSubcase(std::uint32_t key) noexcept
{
    if (Subcase* found = bisectTopLevel(m_segments, key))
        return found;
    return findIn(m_segments, key);
}

const Subcase* CaseControl::findSubcase(std::uint32_t key) const noexcept
{
    if (const Subcase* found = bisectTopLevel(m_segments, key))
        return found;
    return findIn(m_segments, key);
}

const Subcase* CaseControl::parentOf(std::uint32_t key) const noexcept
{
    for (const Subcase& segment : m_segments)
    {
        if (const Subcase* found = findParent(segment, key))
            return found;
    }
    return nullptr;
}

std::vector<const Subcase*> CaseControl::lineage(std::uint32_t key) const
{
    std::vector<const Subcase*> chain;
    std::size_t top = 0;
    for (; top < m_segments.size(); ++top)
    {
        if (collectChain(m_segments[top], key, chain))
            break;
    }
    if (chain.empty())
        return chain;
    std::reverse(chain.begin(), chain.end());
    const SubcaseKind topKind = chain.back()->kind;
    if (topKind == SubcaseKind::Global || topKind == SubcaseKind::AuxiliaryCase)
        return chain;
    // format reference AUXCASE: a subcase of an auxiliary model inherits from its AUXCASE section, never from the
    // primary sections.
    for (std::size_t before = top; before-- > 0;)
    {
        if (m_segments[before].kind == SubcaseKind::AuxiliaryCase)
        {
            chain.push_back(&m_segments[before]);
            return chain;
        }
    }
    chain.push_back(&global());
    return chain;
}

const Subcase* CaseControl::auxiliaryCaseOf(std::uint32_t key) const noexcept
{
    const Subcase* auxcase = nullptr;
    for (const Subcase& segment : m_segments)
    {
        if (segment.kind == SubcaseKind::AuxiliaryCase)
            auxcase = &segment;
        if (segment.key == key || findIn(segment.children, key) != nullptr)
            return auxcase;
    }
    return nullptr;
}

std::optional<CaseControl::EntryLocation> CaseControl::findEntry(std::uint32_t entryKey) noexcept
{
    std::vector<Subcase*> pending;
    for (Subcase& segment : m_segments)
        pending.push_back(&segment);
    while (!pending.empty())
    {
        Subcase* section = pending.back();
        pending.pop_back();
        for (std::size_t index = 0; index < section->entries.size(); ++index)
        {
            if (section->entries[index].key == entryKey)
                return EntryLocation{section, index};
        }
        for (Subcase& child : section->children)
            pending.push_back(&child);
    }
    return std::nullopt;
}

const Subcase* CaseControl::ownerOfEntry(std::uint32_t entryKey) const noexcept
{
    std::vector<const Subcase*> pending;
    for (const Subcase& segment : m_segments)
        pending.push_back(&segment);
    while (!pending.empty())
    {
        const Subcase* section = pending.back();
        pending.pop_back();
        for (const CaseEntry& entry : section->entries)
        {
            if (entry.key == entryKey)
                return section;
        }
        for (const Subcase& child : section->children)
            pending.push_back(&child);
    }
    return nullptr;
}

const CaseEntry* CaseControl::findEntry(std::uint32_t entryKey) const noexcept
{
    for (const Subcase& segment : m_segments)
    {
        if (const CaseEntry* found = findEntryIn(segment, entryKey))
            return found;
    }
    return nullptr;
}

const CaseEntry* CaseControl::ownCommand(const Subcase& subcase, std::string_view name, std::string_view variant)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (entry->kind == EntryKind::Command && entry->name == name && entry->variant == variant)
            return &*entry;
    }
    return nullptr;
}

const CaseEntry* CaseControl::effectiveCommand(std::uint32_t subcaseKey, std::string_view name,
                                               std::string_view variant) const
{
    return commandIn(lineage(subcaseKey), name, variant);
}

const CaseEntry* CaseControl::commandIn(std::span<const Subcase* const> lineage, std::string_view name,
                                        std::string_view variant)
{
    for (const Subcase* section : lineage)
    {
        if (const CaseEntry* entry = ownCommand(*section, name, variant))
            return entry;
    }
    return nullptr;
}

std::vector<const CaseEntry*> CaseControl::ownOutputs(const Subcase& subcase, std::string_view name)
{
    std::vector<const CaseEntry*> found;
    for (const CaseEntry& entry : subcase.entries)
    {
        if (entry.kind == EntryKind::Output && entry.name == name)
            found.push_back(&entry);
    }
    return found;
}

std::vector<const CaseEntry*> CaseControl::effectiveOutputs(std::uint32_t subcaseKey, std::string_view name) const
{
    return outputsIn(lineage(subcaseKey), name);
}

std::vector<const CaseEntry*> CaseControl::outputsIn(std::span<const Subcase* const> lineage, std::string_view name)
{
    for (const Subcase* section : lineage)
    {
        std::vector<const CaseEntry*> found = ownOutputs(*section, name);
        if (!found.empty())
            return found;
    }
    return {};
}

const CaseEntry* CaseControl::ownSet(const Subcase& subcase, std::int64_t id)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (isSetEntry(*entry, id))
            return &*entry;
    }
    return nullptr;
}

const CaseEntry* CaseControl::effectiveSet(std::uint32_t subcaseKey, std::int64_t id) const
{
    return setIn(lineage(subcaseKey), id);
}

const CaseEntry* CaseControl::setIn(std::span<const Subcase* const> lineage, std::int64_t id)
{
    for (const Subcase* section : lineage)
    {
        if (const CaseEntry* entry = ownSet(*section, id))
            return entry;
    }
    return nullptr;
}

const CaseEntry* CaseControl::ownParam(const Subcase& subcase, std::string_view name)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (entry->kind == EntryKind::Param && entry->variant == name)
            return &*entry;
    }
    return nullptr;
}

std::vector<CaseControl::CommandDifference> CaseControl::differences(std::uint32_t firstKey, std::uint32_t secondKey,
                                                                     const std::vector<std::string>& ignored) const
{
    const auto isIgnored = [&](std::string_view name)
    { return std::find(ignored.begin(), ignored.end(), name) != ignored.end(); };
    // The commands (name, variant) and the parameters either section sees.
    std::set<std::pair<std::string, std::string>> commands;
    std::set<std::string> params;
    for (const std::uint32_t key : {firstKey, secondKey})
    {
        for (const Subcase* section : lineage(key))
        {
            for (const CaseEntry& entry : section->entries)
            {
                if (entry.kind == EntryKind::Command && !isIgnored(entry.name))
                    commands.emplace(entry.name, entry.variant);
                else if (entry.kind == EntryKind::Param && !isIgnored(PARAMETER_COMMAND))
                    params.insert(entry.variant);
            }
        }
    }
    const auto textOf = [](const CaseEntry* entry) -> std::optional<std::string>
    {
        if (entry == nullptr)
            return std::nullopt;
        std::string text;
        if (!entry->options.empty())
        {
            text = "(";
            for (std::size_t index = 0; index < entry->options.size(); ++index)
                text += (index == 0 ? "" : ",") + entry->options[index];
            text += ") ";
        }
        return text + entry->value.format();
    };
    const auto effectiveParam = [&](std::uint32_t key, const std::string& name) -> const CaseEntry*
    {
        for (const Subcase* section : lineage(key))
        {
            if (const CaseEntry* entry = ownParam(*section, name))
                return entry;
        }
        return nullptr;
    };
    std::vector<CommandDifference> found;
    for (const auto& [name, variant] : commands)
    {
        auto first = textOf(effectiveCommand(firstKey, name, variant));
        auto second = textOf(effectiveCommand(secondKey, name, variant));
        if (first != second)
            found.push_back(CommandDifference{name, variant, std::move(first), std::move(second)});
    }
    for (const std::string& name : params)
    {
        auto first = textOf(effectiveParam(firstKey, name));
        auto second = textOf(effectiveParam(secondKey, name));
        if (first != second)
            found.push_back(
                CommandDifference{std::string(PARAMETER_COMMAND), name, std::move(first), std::move(second)});
    }
    return found;
}

bool CaseControl::sameOptions(const std::vector<std::string>& left, const std::vector<std::string>& right)
{
    return sorted(left) == sorted(right);
}

CaseEntry& CaseControl::append(Subcase& subcase, CaseEntry entry)
{
    entry.key = newKey();
    // format reference: an OUTPUT(PLOT) or OUTPUT(XYPLOT) block ends its subcase: a command added to the subcase goes
    // before it.
    std::size_t position = subcase.entries.size();
    for (std::size_t index = 0; index < subcase.entries.size(); ++index)
    {
        const CaseEntry& existing = subcase.entries[index];
        if (existing.kind == EntryKind::OutputBlock && (existing.variant == "Plot" || existing.variant == "CurvePlot"))
        {
            position = index;
            break;
        }
    }
    return *subcase.entries.insert(subcase.entries.begin() + static_cast<std::ptrdiff_t>(position), std::move(entry));
}

Subcase& CaseControl::addSubcase(SubcaseKind kind, std::int64_t id)
{
    if (kind == SubcaseKind::Global || kind == SubcaseKind::Step || kind == SubcaseKind::Substep ||
        kind == SubcaseKind::AuxiliaryCase)
        throw base::UsageError("a subcase is a SUBCASE, SUBCOM, SYM, SYMCOM or REPCASE");
    Subcase subcase;
    subcase.key = newKey();
    subcase.kind = kind;
    subcase.id = id;
    subcase.header = makeHeader(subcaseCommandName(kind));
    subcase.header.key = newKey();
    // format reference: the structure plotter and X-Y plot commands follow the last subcase: a block of the old last
    // subcase moves with them to the new one.
    Subcase& last = m_segments.back();
    if (last.kind != SubcaseKind::Global && last.children.empty())
    {
        for (std::size_t index = 0; index < last.entries.size(); ++index)
        {
            const CaseEntry& entry = last.entries[index];
            if (entry.kind != EntryKind::OutputBlock || (entry.variant != "Plot" && entry.variant != "CurvePlot"))
                continue;
            subcase.entries.assign(std::make_move_iterator(last.entries.begin() + static_cast<std::ptrdiff_t>(index)),
                                   std::make_move_iterator(last.entries.end()));
            last.entries.erase(last.entries.begin() + static_cast<std::ptrdiff_t>(index), last.entries.end());
            break;
        }
    }
    m_segments.push_back(std::move(subcase));
    return m_segments.back();
}

CaseControl::AdditionPoint CaseControl::additionPoint() const
{
    return AdditionPoint{m_segments.size(), m_segments.back(), m_nextKey};
}

void CaseControl::restore(AdditionPoint point)
{
    if (point.segmentCount == 0 || point.segmentCount > m_segments.size())
        throw base::UsageError("restore: the Case Control has fewer sections than at its addition point");
    m_segments.erase(m_segments.begin() + static_cast<std::ptrdiff_t>(point.segmentCount), m_segments.end());
    m_segments.back() = std::move(point.last);
    m_nextKey = point.nextKey;
}

Subcase& CaseControl::addAuxiliaryCase()
{
    Subcase auxcase;
    auxcase.key = newKey();
    auxcase.kind = SubcaseKind::AuxiliaryCase;
    auxcase.header = makeHeader(subcaseCommandName(SubcaseKind::AuxiliaryCase));
    auxcase.header.key = newKey();
    m_segments.push_back(std::move(auxcase));
    return m_segments.back();
}

Subcase& CaseControl::addChild(std::uint32_t parentKey, SubcaseKind kind, std::int64_t id)
{
    if (kind != SubcaseKind::Step && kind != SubcaseKind::Substep)
        throw base::UsageError("a step is a STEP or a SUBSTEP");
    Subcase* parent = findSubcase(parentKey);
    if (parent == nullptr)
        throw base::UsageError("a step goes in a subcase or a step");
    Subcase child;
    child.key = newKey();
    child.kind = kind;
    child.id = id;
    child.header = makeHeader(subcaseCommandName(kind));
    child.header.key = newKey();
    parent->children.push_back(std::move(child));
    return parent->children.back();
}

bool CaseControl::removeSubcase(std::uint32_t key)
{
    Subcase* target = findSubcase(key);
    if (target == nullptr || target->kind == SubcaseKind::Global || holdsIncludes(*target))
        return false;
    Subcase* parent = nullptr;
    for (Subcase& segment : m_segments)
    {
        if (parent == nullptr)
            parent = findParent(segment, key);
    }
    std::vector<Subcase>& list = parent != nullptr ? parent->children : m_segments;
    list.erase(std::find_if(list.begin(), list.end(), [key](const Subcase& item) { return item.key == key; }));
    return true;
}

void CaseControl::setNumber(Subcase& subcase, std::int64_t id)
{
    if (subcase.kind == SubcaseKind::Global || subcase.kind == SubcaseKind::AuxiliaryCase)
        throw base::UsageError("the global section and an AUXCASE section have no number");
    if (subcase.id == id)
        return;
    subcase.id = id;
    subcase.header.original.clear();
}

CaseEntry& CaseControl::setCommand(Subcase& subcase, const CommandDefinition& definition, std::string_view variant,
                                   std::vector<std::string> options, CaseValue value)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (entry->kind != EntryKind::Command || entry->name != definition.name || entry->variant != variant)
            continue;
        if (entry->value == value && entry->options == options)
            return *entry;
        entry->value = std::move(value);
        entry->options = std::move(options);
        entry->original.clear();
        return *entry;
    }
    CaseEntry entry;
    entry.kind = EntryKind::Command;
    entry.name = definition.name;
    entry.variant = std::string(variant);
    entry.options = std::move(options);
    entry.value = std::move(value);
    return append(subcase, std::move(entry));
}

CaseEntry& CaseControl::setOutput(Subcase& subcase, const CommandDefinition& definition,
                                  std::vector<std::string> options, CaseValue selection)
{
    for (CaseEntry& entry : subcase.entries)
    {
        if (entry.kind != EntryKind::Output || entry.name != definition.name || !sameOptions(entry.options, options))
            continue;
        if (entry.value == selection && entry.options == options)
            return entry;
        entry.value = std::move(selection);
        entry.options = std::move(options);
        entry.original.clear();
        return entry;
    }
    CaseEntry entry;
    entry.kind = EntryKind::Output;
    entry.name = definition.name;
    entry.options = std::move(options);
    entry.value = std::move(selection);
    return append(subcase, std::move(entry));
}

CaseEntry& CaseControl::setSet(Subcase& subcase, std::int64_t id, CaseSet members)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (!isSetEntry(*entry, id))
            continue;
        if (entry->set == members)
            return *entry;
        entry->set = std::move(members);
        entry->original.clear();
        return *entry;
    }
    CaseEntry entry;
    entry.kind = EntryKind::Set;
    entry.name = SET_COMMAND;
    entry.number = id;
    entry.set = std::move(members);
    return append(subcase, std::move(entry));
}

CaseEntry& CaseControl::setParam(Subcase& subcase, std::string_view name, CaseValue value)
{
    for (auto entry = subcase.entries.rbegin(); entry != subcase.entries.rend(); ++entry)
    {
        if (entry->kind != EntryKind::Param || entry->variant != name)
            continue;
        if (entry->value == value)
            return *entry;
        entry->value = std::move(value);
        entry->original.clear();
        return *entry;
    }
    CaseEntry entry;
    entry.kind = EntryKind::Param;
    entry.name = PARAMETER_COMMAND;
    entry.variant = std::string(name);
    entry.value = std::move(value);
    return append(subcase, std::move(entry));
}

bool CaseControl::removeCommand(Subcase& subcase, std::string_view name, std::string_view variant)
{
    const auto matches = [&](const CaseEntry& entry)
    { return entry.kind == EntryKind::Command && entry.name == name && entry.variant == variant; };
    const auto end = std::remove_if(subcase.entries.begin(), subcase.entries.end(), matches);
    const bool removed = end != subcase.entries.end();
    subcase.entries.erase(end, subcase.entries.end());
    return removed;
}

bool CaseControl::removeOutputs(Subcase& subcase, std::string_view name, const std::vector<std::string>* options)
{
    const auto matches = [&](const CaseEntry& entry)
    {
        return entry.kind == EntryKind::Output && entry.name == name &&
               (options == nullptr || sameOptions(entry.options, *options));
    };
    const auto end = std::remove_if(subcase.entries.begin(), subcase.entries.end(), matches);
    const bool removed = end != subcase.entries.end();
    subcase.entries.erase(end, subcase.entries.end());
    return removed;
}

bool CaseControl::removeSet(Subcase& subcase, std::int64_t id)
{
    const auto end = std::remove_if(subcase.entries.begin(), subcase.entries.end(),
                                    [id](const CaseEntry& entry) { return isSetEntry(entry, id); });
    const bool removed = end != subcase.entries.end();
    subcase.entries.erase(end, subcase.entries.end());
    return removed;
}

bool CaseControl::removeParam(Subcase& subcase, std::string_view name)
{
    const auto end = std::remove_if(subcase.entries.begin(), subcase.entries.end(), [&](const CaseEntry& entry)
                                    { return entry.kind == EntryKind::Param && entry.variant == name; });
    const bool removed = end != subcase.entries.end();
    subcase.entries.erase(end, subcase.entries.end());
    return removed;
}

bool CaseControl::removeEntry(std::uint32_t entryKey)
{
    const auto location = findEntry(entryKey);
    if (!location)
        return false;
    location->subcase->entries.erase(location->subcase->entries.begin() + static_cast<std::ptrdiff_t>(location->index));
    return true;
}

namespace
{

// RANDOM = n may name a Case Control SET (format reference: the SET lists RANDPS and RANDT1 set IDs), and SEALL, SEDR,
// SUPER, ... = n a SET of superelement numbers (format reference: n is the number of a previously appearing SET, which
// must differ from the number of any superelement), not a set of Bulk Data cards or a superelement: such a number is
// not a reference.
bool namesCaseSet(const CommandDefinition& definition, std::int64_t number, const std::vector<std::int64_t>& sets)
{
    return (definition.reference == ReferenceTarget::RandomSet ||
            definition.reference == ReferenceTarget::Superelement) &&
           std::find(sets.begin(), sets.end(), number) != sets.end();
}

void visitSection(Subcase& section, ReferenceVisitor& visitor, const CaseControl::ScopeCallback& onScope,
                  std::vector<std::int64_t> sets)
{
    if (onScope)
        onScope(section.kind, section.id);
    const CommandCatalogue& catalogue = CommandCatalogue::instance();
    for (const CaseEntry& entry : section.entries)
    {
        if (entry.kind == EntryKind::Set)
            sets.push_back(entry.number);
    }
    for (CaseEntry& entry : section.entries)
    {
        if (entry.kind != EntryKind::Command || entry.value.kind != CaseValueKind::Integer)
            continue;
        const CommandDefinition* definition = catalogue.findByName(entry.name);
        if (definition == nullptr || !definition->reference || namesCaseSet(*definition, entry.value.integer, sets))
            continue;
        const auto id = base::toEntityId(entry.value.integer);
        if (!id)
            continue;
        const base::EntityId updated = visitor.visit(definition->name, *definition->reference, *id);
        if (updated != *id)
        {
            entry.value.integer = updated;
            entry.original.clear();
        }
    }
    for (Subcase& child : section.children)
        visitSection(child, visitor, onScope, sets);
}

} // namespace

void CaseControl::visitReferences(ReferenceVisitor& visitor, const ScopeCallback& onScope)
{
    // The SETs of the global section are known in every subcase.
    std::vector<std::int64_t> globalSets;
    if (!m_segments.empty())
    {
        for (const CaseEntry& entry : m_segments.front().entries)
        {
            if (entry.kind == EntryKind::Set)
                globalSets.push_back(entry.number);
        }
    }
    for (Subcase& segment : m_segments)
        visitSection(segment, visitor, onScope, globalSets);
}

namespace
{

bool isBlockLine(const CaseEntry& entry) noexcept
{
    return entry.kind == EntryKind::OutputBlock;
}

void collectBlockKeys(const Subcase& section, std::vector<std::uint32_t>& keys)
{
    for (const CaseEntry& entry : section.entries)
    {
        if (isBlockLine(entry))
            keys.push_back(entry.key);
    }
    for (const Subcase& child : section.children)
        collectBlockKeys(child, keys);
}

} // namespace

CaseControl::Extent CaseControl::blockExtent(const Subcase& subcase, std::size_t markerIndex)
{
    const CommandCatalogue& catalogue = CommandCatalogue::instance();
    std::size_t last = markerIndex + 1;
    while (last < subcase.entries.size())
    {
        const CaseEntry& entry = subcase.entries[last];
        bool belongs = false;
        switch (entry.kind)
        {
        case EntryKind::Command:
        {
            const CommandDefinition* definition = catalogue.findByName(entry.name);
            belongs = definition != nullptr && definition->isBlockCommand;
            break;
        }
        case EntryKind::Set:
        case EntryKind::Raw:
            belongs = true;
            break;
        default:
            break;
        }
        if (!belongs)
            break;
        ++last;
    }
    return Extent{markerIndex + 1, last};
}

CaseEntry& CaseControl::addBlock(std::string kind)
{
    CaseEntry marker;
    marker.kind = EntryKind::OutputBlock;
    marker.name = OUTPUT_BLOCK_COMMAND;
    marker.variant = std::move(kind);
    marker.options.push_back(marker.variant);
    marker.key = newKey();
    // At the end of the file: in the last subcase, or in its last step or substep.
    Subcase* target = &m_segments.back();
    while (!target->children.empty())
        target = &target->children.back();
    target->entries.push_back(std::move(marker));
    return target->entries.back();
}

CaseEntry& CaseControl::addBlockCommand(std::uint32_t markerKey, const CommandDefinition& definition,
                                        std::string arguments)
{
    const auto marker = findEntry(markerKey);
    if (!marker || !isBlockLine(marker->subcase->entries[marker->index]))
        throw base::UsageError("not the line of an output block");
    Subcase& subcase = *marker->subcase;
    CaseEntry entry;
    entry.kind = EntryKind::Command;
    entry.name = definition.name;
    const std::string_view trimmed = base::trimAscii(arguments);
    if (!trimmed.empty())
        entry.value = CaseValue::ofText(std::string(trimmed));
    entry.key = newKey(); // plot commands are not indented
    const Extent extent = blockExtent(subcase, marker->index);
    return *subcase.entries.insert(subcase.entries.begin() + static_cast<std::ptrdiff_t>(extent.last),
                                   std::move(entry));
}

bool CaseControl::removeBlock(std::uint32_t markerKey)
{
    const auto marker = findEntry(markerKey);
    if (!marker || !isBlockLine(marker->subcase->entries[marker->index]))
        return false;
    Subcase& subcase = *marker->subcase;
    const Extent extent = blockExtent(subcase, marker->index);
    subcase.entries.erase(subcase.entries.begin() + static_cast<std::ptrdiff_t>(marker->index),
                          subcase.entries.begin() + static_cast<std::ptrdiff_t>(extent.last));
    return true;
}

std::vector<std::uint32_t> CaseControl::blockKeys() const
{
    std::vector<std::uint32_t> keys;
    for (const Subcase& segment : m_segments)
        collectBlockKeys(segment, keys);
    return keys;
}

} // namespace oor::model
