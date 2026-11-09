#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"

#include "ExplicitParamCatalogueData.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <numeric>

namespace oor::model
{

namespace
{

constexpr double INFINITY_VALUE = std::numeric_limits<double>::infinity();

std::string formatNumber(double value)
{
    return base::NumberFormatter::formatRealFree(value);
}

// "the integer 5", "the word YES": what a field holds, for a message.
std::string describeValue(const ExplicitParamValue& value)
{
    switch (value.kind)
    {
    case ExplicitParamValueKind::Blank:
        return "nothing";
    case ExplicitParamValueKind::Integer:
        return "the integer " + std::to_string(value.integerValue);
    case ExplicitParamValueKind::Real:
        return "a real number";
    case ExplicitParamValueKind::Text:
        return "the word " + value.textValue;
    }
    return "a value";
}

// "YES or NO", "BEAMS, SHELLS or SOLIDS".
std::string wordList(const ExplicitParamSlot& slot)
{
    const std::vector<std::string_view> words = slot.allowedWords();
    std::string phrase;
    for (std::size_t index = 0; index < words.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == words.size() ? " or " : ", ";
        phrase += words[index];
    }
    return phrase;
}

bool isAllowedWord(const ExplicitParamSlot& slot, std::string_view text)
{
    for (const std::string_view word : slot.allowedWords())
    {
        if ((word == text))
            return true;
    }
    return false;
}

CardProblem wrongKind(const ExplicitParamSlot& slot, const std::string& expected, const ExplicitParamValue& value)
{
    return CardProblem{"card.bad-value",
                       std::string(slot.name) + " must be " + expected + ", found " + describeValue(value), slot.name};
}

CardProblem outOfRange(const ExplicitParamSlot& slot)
{
    return CardProblem{"card.bad-value", std::string(slot.name) + " " + slot.range.describe(), slot.name};
}

// RigidAssembly(id) or RigidMat(id): the kind of body ('F' for a FULLRIG assembly of an RBE2, 'M' for a MATRIG) and its
// number.
struct RigidBodyName
{
    char kind;
    base::EntityId id;
};

std::optional<RigidBodyName> parseRigidBodyName(std::string_view text)
{
    constexpr std::string_view ASSEMBLY = "RigidAssembly(";
    constexpr std::string_view MATERIAL = "RigidMat(";
    const bool assembly = text.starts_with(ASSEMBLY);
    const std::string_view prefix = assembly ? ASSEMBLY : MATERIAL;
    if (!text.starts_with(prefix) || text.size() <= prefix.size() + 1 || text.back() != ')')
        return std::nullopt;
    const std::string_view digits = text.substr(prefix.size(), text.size() - prefix.size() - 1);
    if (!std::ranges::all_of(digits, [](char character) { return character >= '0' && character <= '9'; }))
        return std::nullopt;
    const auto number = base::NumberParser::parseInteger(digits);
    if (!number || *number <= 0 || *number > std::numeric_limits<base::EntityId>::max())
        return std::nullopt;
    return RigidBodyName{assembly ? 'F' : 'M', static_cast<base::EntityId>(*number)};
}

std::optional<CardProblem> checkWord(const ExplicitParamSlot& slot, const ExplicitParamValue& value)
{
    if (value.kind != ExplicitParamValueKind::Text)
        return wrongKind(slot, slot.words.empty() ? "a word" : wordList(slot), value);
    if (!slot.words.empty() && !isAllowedWord(slot, value.textValue))
        return CardProblem{"card.bad-value",
                           std::string(slot.name) + " must be " + wordList(slot) + ", not " + value.textValue,
                           slot.name};
    return std::nullopt;
}

// The problem with a value in its slot, if there is one. A blank value is a problem only where the slot is required.
std::optional<CardProblem> checkSlotValue(const ExplicitParamSlot& slot, const ExplicitParamValue& value)
{
    if (value.isBlank())
    {
        if (slot.required())
            return CardProblem{"card.missing-field", std::string(slot.name) + " is required", slot.name};
        return std::nullopt;
    }
    switch (slot.kind)
    {
    case ExplicitParamSlotKind::Integer:
        if (value.kind != ExplicitParamValueKind::Integer)
            return wrongKind(slot, "an integer", value);
        if (!slot.range.contains(value.integerValue))
            return outOfRange(slot);
        return std::nullopt;
    case ExplicitParamSlotKind::Real:
        if (!value.isNumber())
            return wrongKind(slot, "a real number", value);
        if (!slot.range.contains(value.number()))
            return outOfRange(slot);
        return std::nullopt;
    case ExplicitParamSlotKind::Word:
        return checkWord(slot, value);
    case ExplicitParamSlotKind::IntegerOrWord:
        if (value.kind == ExplicitParamValueKind::Text)
            return checkWord(slot, value);
        if (value.kind != ExplicitParamValueKind::Integer)
            return wrongKind(slot, "an integer or " + wordList(slot), value);
        if (!slot.range.contains(value.integerValue))
            return outOfRange(slot);
        return std::nullopt;
    case ExplicitParamSlotKind::Table:
    case ExplicitParamSlotKind::Grid:
        if (value.kind != ExplicitParamValueKind::Integer)
            return wrongKind(slot,
                             slot.kind == ExplicitParamSlotKind::Table
                                 ? "the number of a " + CardProblem::cardNamed("TableDynamic1")
                                 : std::string("the number of a grid point"),
                             value);
        if (value.integerValue <= 0)
            return CardProblem{"card.bad-id", std::string(slot.name) + " must be a positive integer", slot.name};
        return std::nullopt;
    case ExplicitParamSlotKind::RigidBody:
    {
        if (value.kind != ExplicitParamValueKind::Text)
            return wrongKind(slot, "a rigid body name", value);
        if (!slot.words.empty() && isAllowedWord(slot, value.textValue))
            return std::nullopt;
        if (!parseRigidBodyName(value.textValue))
            return CardProblem{"card.bad-value",
                               std::string(slot.name) + " must be RigidAssembly(id) or RigidMat(id)" +
                                   (slot.words.empty() ? "" : " or " + wordList(slot)) + ", not " + value.textValue,
                               slot.name};
        return std::nullopt;
    }
    }
    return std::nullopt;
}

CardProblem selectorProblem(const ExplicitParamEntry& entry, std::span<const ExplicitParamValue> values)
{
    std::string options;
    for (const ExplicitParamForm& form : entry.forms)
        options += (options.empty() ? "" : ", ") + std::string(form.selector);
    const std::size_t index = static_cast<std::size_t>(entry.selectorIndex);
    if (index >= values.size() || values[index].isBlank())
        return CardProblem{"card.missing-field", std::string(entry.selectorName) + " is required: " + options,
                           entry.selectorName};
    return CardProblem{"card.bad-value",
                       std::string(entry.selectorName) + " must be one of " + options + ", not " +
                           (values[index].kind == ExplicitParamValueKind::Text ? values[index].textValue
                                                                               : describeValue(values[index])),
                       entry.selectorName};
}

bool given(std::span<const ExplicitParamValue> values, std::size_t index) noexcept
{
    return index < values.size() && !values[index].isBlank();
}

bool isWord(std::span<const ExplicitParamValue> values, std::size_t index, std::string_view word) noexcept
{
    return index < values.size() && values[index].kind == ExplicitParamValueKind::Text &&
           (values[index].textValue == word);
}

// The rules of the format that tie the values of one entry together.
std::optional<CardProblem> relationProblem(const ExplicitParamEntry& entry, const ExplicitParamForm& form,
                                           std::span<const ExplicitParamValue> values)
{
    if (entry.name == "EulerPartitioning")
    {
        // Distributing the cubes by hand needs the number of cubes in x; the other two follow it when blank.
        if (isWord(values, 0, "User") && !(given(values, 1) && values[1].integerValue > 0))
            return CardProblem{
                "card.missing-field",
                std::string(CardProblem::FIELD) + " must be given and greater than zero when PROCDIR is USER",
                "cubeCountX"};
    }
    else if (entry.name == "EulerSubcyclesMax")
    {
        // The two subcycling processes are independent only when the uncovered fraction is watched here.
        if (isWord(values, 2, "None") && !given(values, 1))
            return CardProblem{"card.missing-field", "DFVUMAX is required when LINKCS is NONE",
                               "maxUncoveredFractionChange"};
    }
    else if (entry.name == "LagrangePartitioning")
    {
        // The vector decomposition (4) needs the direction; the radial and angular ones (5, 6) also the centre.
        constexpr std::array<const char*, 6> NAMES{"directionX", "directionY", "directionZ",
                                                   "centreX",    "centreY",    "centreZ"};
        const std::int32_t option = given(values, 0) ? values[0].integerValue : 1;
        const std::size_t needed = option == 4 ? 3 : (option == 5 || option == 6 ? 6 : 0);
        for (std::size_t index = 0; index < needed; ++index)
        {
            if (!given(values, index + 1))
                return CardProblem{
                    "card.missing-field",
                    std::string(CardProblem::FIELD) + " is required when IOPT is " + std::to_string(option),
                    NAMES[index]};
        }
    }
    else if (entry.name == "SharedMemoryParallel" && form.selector == "BatchSize")
    {
        // The word CPULOOPS says what the next number is: neither one without the other.
        if (given(values, 3) && !given(values, 4))
            return CardProblem{"card.missing-field", "cpuLoopCount is required after ProcessorLoopCount",
                               "cpuLoopCount"};
        if (given(values, 4) && !given(values, 3))
            return CardProblem{"card.missing-field", "cpuLoopCount must follow ProcessorLoopCount", "cpuLoopsWord"};
    }
    else if (entry.name == "RigidBodyMerge")
    {
        // Either the single word AUTO, or the new RigidAssembly(id) and at least one body to merge into it.
        if (isWord(values, 0, "Automatic"))
        {
            if (values.size() > 1)
                return CardProblem{"card.bad-value", "no name can follow Automatic", "mergedBodyNames"};
        }
        else
        {
            if (values.size() < 2)
                return CardProblem{"card.missing-field", "RigidBodyMerge needs rigid bodies to merge or Automatic",
                                   "mergedBodyNames"};
            if (const auto assembly = parseRigidBodyName(values[0].textValue); !assembly || assembly->kind != 'F')
                return CardProblem{"card.bad-value", "newAssemblyName must name a RigidAssembly(id)",
                                   "newAssemblyName"};
        }
    }
    return std::nullopt;
}

} // namespace

std::string_view explicitParamSlotKindName(ExplicitParamSlotKind kind) noexcept
{
    switch (kind)
    {
    case ExplicitParamSlotKind::Integer:
        return "integer";
    case ExplicitParamSlotKind::Real:
        return "real";
    case ExplicitParamSlotKind::Word:
        return "word";
    case ExplicitParamSlotKind::IntegerOrWord:
        return "integer_or_word";
    case ExplicitParamSlotKind::Table:
        return "table";
    case ExplicitParamSlotKind::Grid:
        return "grid";
    case ExplicitParamSlotKind::RigidBody:
        return "rigid_body";
    }
    return "word";
}

bool ExplicitParamRange::contains(double value) const noexcept
{
    if (std::isnan(value) || value < minimum || (excludesMinimum && value == minimum))
        return false;
    return value <= maximum;
}

bool ExplicitParamRange::isLimited() const noexcept
{
    return minimum > -INFINITY_VALUE || maximum < INFINITY_VALUE;
}

std::string ExplicitParamRange::describe() const
{
    const bool hasMinimum = minimum > -INFINITY_VALUE;
    const bool hasMaximum = maximum < INFINITY_VALUE;
    if (hasMinimum && hasMaximum)
    {
        if (excludesMinimum)
            return "must be greater than " + formatNumber(minimum) + " and at most " + formatNumber(maximum);
        return "must be between " + formatNumber(minimum) + " and " + formatNumber(maximum);
    }
    if (hasMinimum)
    {
        if (minimum == 0.0)
            return excludesMinimum ? "must be greater than zero" : "must not be negative";
        return (excludesMinimum ? "must be greater than " : "must be at least ") + formatNumber(minimum);
    }
    if (hasMaximum)
        return "must be at most " + formatNumber(maximum);
    return "must be a number";
}

std::vector<std::string_view> ExplicitParamSlot::allowedWords() const
{
    std::vector<std::string_view> result;
    std::string_view rest = words;
    while (!rest.empty())
    {
        const std::size_t bar = rest.find('|');
        result.push_back(rest.substr(0, bar));
        rest = bar == std::string_view::npos ? std::string_view{} : rest.substr(bar + 1);
    }
    return result;
}

ExplicitParamCatalogue::ExplicitParamCatalogue() : m_entries(explicitParamCatalogueEntries())
{
    m_byName.resize(m_entries.size());
    std::iota(m_byName.begin(), m_byName.end(), std::size_t{0});
    std::ranges::sort(
        m_byName, [this](std::size_t left, std::size_t right) { return m_entries[left].name < m_entries[right].name; });
}

const ExplicitParamCatalogue& ExplicitParamCatalogue::instance()
{
    static const ExplicitParamCatalogue CATALOGUE;
    return CATALOGUE;
}

const ExplicitParamEntry* ExplicitParamCatalogue::find(std::string_view name) const
{
    const auto found =
        std::ranges::lower_bound(m_byName, name, {}, [this](std::size_t index) { return m_entries[index].name; });
    if (found != m_byName.end() && m_entries[*found].name == name)
        return &m_entries[*found];
    return nullptr;
}

const ExplicitParamForm* ExplicitParamCatalogue::formOf(const ExplicitParamEntry& entry,
                                                        std::span<const ExplicitParamValue> values)
{
    if (entry.forms.size() == 1)
        return &entry.forms.front();
    if (entry.selectorIndex < 0)
        return nullptr;
    const std::size_t index = static_cast<std::size_t>(entry.selectorIndex);
    if (index >= values.size() || values[index].kind != ExplicitParamValueKind::Text)
        return nullptr;
    for (const ExplicitParamForm& form : entry.forms)
    {
        if ((form.selector == values[index].textValue))
            return &form;
    }
    return nullptr;
}

const ExplicitParamSlot* ExplicitParamCatalogue::slotOf(const ExplicitParamCard& card, std::size_t index) const
{
    const ExplicitParamEntry* entry = find(card.entryName);
    if (entry == nullptr)
        return nullptr;
    const ExplicitParamForm* form = formOf(*entry, card.values);
    if (form == nullptr)
        return nullptr;
    if (index < form->slots.size())
        return &form->slots[index];
    return form->tail ? &*form->tail : nullptr;
}

std::optional<CardProblem> ExplicitParamCatalogue::problemWith(const ExplicitParamCard& card) const
{
    const ExplicitParamEntry* entry = find(card.entryName);
    if (entry == nullptr)
        return std::nullopt;
    const ExplicitParamForm* form = formOf(*entry, card.values);
    if (form == nullptr)
        return selectorProblem(*entry, card.values);

    const ExplicitParamValue blank;
    for (std::size_t index = 0; index < form->slots.size(); ++index)
    {
        const ExplicitParamValue& value = index < card.values.size() ? card.values[index] : blank;
        if (const auto problem = checkSlotValue(form->slots[index], value))
            return problem;
    }

    const std::size_t fixed = form->slots.size();
    const std::size_t listed = card.values.size() > fixed ? card.values.size() - fixed : 0;
    if (!form->tail)
    {
        if (listed > 0)
        {
            std::string setting = std::string(entry->name);
            if (!form->selector.empty() && entry->forms.size() > 1)
                setting += "," + std::string(form->selector);
            return CardProblem{"card.bad-value",
                               CardProblem::cardNamed(ExplicitParamTable::NAME) + "," + setting + " has at most " +
                                   std::to_string(fixed) + " values",
                               ""};
        }
    }
    else
    {
        for (std::size_t index = fixed; index < card.values.size(); ++index)
        {
            if (const auto problem = checkSlotValue(*form->tail, card.values[index]))
                return problem;
        }
        if (listed < form->tailMinimum)
            return CardProblem{"card.missing-field",
                               std::string(form->tail->name) + " is required: give at least " +
                                   std::to_string(form->tailMinimum) + " after the others",
                               form->tail->name};
    }
    return relationProblem(*entry, *form, card.values);
}

std::optional<std::string_view> slotWord(const ExplicitParamSlot& slot, std::string_view text)
{
    for (const std::string_view word : slot.allowedWords())
    {
        if ((word == text))
            return word;
    }
    return std::nullopt;
}

void normaliseExplicitParam(ExplicitParamCard& card)
{
    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    // A word of the slot is kept in the spelling of the catalogue (its neutral name); any other text (a name of the
    // user, or a typed rigid-body designator) exactly as supplied.
    for (std::size_t index = 0; index < card.values.size(); ++index)
    {
        ExplicitParamValue& value = card.values[index];
        if (value.kind != ExplicitParamValueKind::Text)
            continue;
        const ExplicitParamSlot* slot = catalogue.slotOf(card, index);
        if (const std::optional<std::string_view> word =
                slot != nullptr ? slotWord(*slot, value.textValue) : std::nullopt)
            value.textValue = std::string(*word);
    }
    while (!card.values.empty() && card.values.back().isBlank())
        card.values.pop_back();
    for (std::size_t index = 0; index < card.values.size(); ++index)
    {
        if (card.values[index].kind != ExplicitParamValueKind::Integer)
            continue;
        const ExplicitParamSlot* slot = catalogue.slotOf(card, index);
        if (slot != nullptr && slot->kind == ExplicitParamSlotKind::Real)
            card.values[index] = ExplicitParamValue::real(static_cast<double>(card.values[index].integerValue));
    }
}

std::vector<ExplicitParamValue> resolveExplicitParamValues(const ExplicitParamCard& card)
{
    std::vector<ExplicitParamValue> values = card.values;
    const ExplicitParamEntry* entry = ExplicitParamCatalogue::instance().find(card.entryName);
    if (entry == nullptr)
        return values;
    const ExplicitParamForm* form = ExplicitParamCatalogue::formOf(*entry, card.values);
    if (form == nullptr)
        return values;
    if (values.size() < form->slots.size())
        values.resize(form->slots.size());
    for (std::size_t index = 0; index < form->slots.size(); ++index)
    {
        if (!values[index].isBlank())
            continue;
        const ExplicitParamDefault& fallback = form->slots[index].defaultValue;
        switch (fallback.kind)
        {
        case ExplicitParamDefaultKind::Integer:
            values[index] = ExplicitParamValue::integer(fallback.integerValue);
            break;
        case ExplicitParamDefaultKind::Real:
            values[index] = ExplicitParamValue::real(fallback.realValue);
            break;
        case ExplicitParamDefaultKind::Word:
            values[index] = ExplicitParamValue::text(std::string(fallback.word));
            break;
        case ExplicitParamDefaultKind::SameAs:
            values[index] = values[fallback.slot];
            break;
        case ExplicitParamDefaultKind::None:
        case ExplicitParamDefaultKind::Required:
        case ExplicitParamDefaultKind::Computed:
            break;
        }
    }
    return values;
}

std::optional<std::string> explicitParamIdentity(const ExplicitParamCard& card)
{
    const ExplicitParamEntry* entry = ExplicitParamCatalogue::instance().find(card.entryName);
    if (entry == nullptr || entry->identityValues < 0)
        return std::nullopt;
    std::string identity = std::string(entry->name);
    for (std::size_t index = 0; index < static_cast<std::size_t>(entry->identityValues); ++index)
    {
        identity += ',';
        if (index < card.values.size() && card.values[index].kind == ExplicitParamValueKind::Text)
            identity += card.values[index].textValue;
    }
    return identity;
}

std::optional<ExplicitParamReference> explicitParamReference(const ExplicitParamCard& card, std::size_t index)
{
    if (index >= card.values.size())
        return std::nullopt;
    const ExplicitParamSlot* slot = ExplicitParamCatalogue::instance().slotOf(card, index);
    if (slot == nullptr)
        return std::nullopt;
    const ExplicitParamValue& value = card.values[index];
    switch (slot->kind)
    {
    case ExplicitParamSlotKind::Table:
        if (value.kind == ExplicitParamValueKind::Integer && value.integerValue > 0)
            return ExplicitParamReference{ReferenceTarget::DynamicTable, value.integerValue};
        return std::nullopt;
    case ExplicitParamSlotKind::Grid:
        if (value.kind == ExplicitParamValueKind::Integer && value.integerValue > 0)
            return ExplicitParamReference{ReferenceTarget::Node, value.integerValue};
        return std::nullopt;
    case ExplicitParamSlotKind::RigidBody:
        if (value.kind == ExplicitParamValueKind::Text)
        {
            const std::optional<RigidBodyName> name = parseRigidBodyName(value.textValue);
            if (name && name->kind == 'M')
                return ExplicitParamReference{ReferenceTarget::Material, name->id};
        }
        return std::nullopt;
    case ExplicitParamSlotKind::Integer:
    case ExplicitParamSlotKind::Real:
    case ExplicitParamSlotKind::Word:
    case ExplicitParamSlotKind::IntegerOrWord:
        return std::nullopt;
    }
    return std::nullopt;
}

void visitExplicitParamReferences(ExplicitParamCard& card, ReferenceVisitor& visitor)
{
    for (std::size_t index = 0; index < card.values.size(); ++index)
    {
        const std::optional<ExplicitParamReference> reference = explicitParamReference(card, index);
        if (!reference)
            continue;
        const ExplicitParamSlot* slot = ExplicitParamCatalogue::instance().slotOf(card, index);
        const std::string field = slot->name;
        const base::EntityId updated = visitor.visit(field, reference->target, reference->id);
        if (updated == reference->id)
            continue;
        ExplicitParamValue& value = card.values[index];
        if (slot->kind == ExplicitParamSlotKind::RigidBody)
            value.textValue = "RigidMat(" + std::to_string(updated) + ")";
        else
            value.integerValue = updated;
    }
}

} // namespace oor::model
