#include "oor/model/cards/properties/ArbitrarySection.hpp"

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

namespace oor::model
{

namespace
{

CardProblem problemOf(const char* field, std::string message)
{
    return CardProblem{"card.bad-value", std::move(message), field};
}

std::optional<base::EntityId> parseId(std::string_view text)
{
    const std::optional<std::int64_t> value = base::NumberParser::parseInteger(text);
    if (!value)
        return std::nullopt;
    return base::toEntityId(*value);
}

// The parts of `text` between the commas that are not inside parentheses or brackets; nothing when they do not match.
std::optional<std::vector<std::string_view>> splitTopLevel(std::string_view text)
{
    std::vector<std::string_view> parts;
    int depth = 0;
    std::size_t begin = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const char character = text[index];
        if (character == '(' || character == '[')
        {
            ++depth;
        }
        else if (character == ')' || character == ']')
        {
            if (--depth < 0)
                return std::nullopt;
        }
        else if (character == ',' && depth == 0)
        {
            parts.push_back(text.substr(begin, index - begin));
            begin = index + 1;
        }
    }
    if (depth != 0)
        return std::nullopt;
    parts.push_back(text.substr(begin));
    return parts;
}

// Description keys are exact neutral field names.
std::optional<ArbitrarySectionKey> keyOf(std::string_view name)
{
    using Key = ArbitrarySectionKey;
    if (name == "outerPerimeter")
        return Key::OuterPerimeter;
    if (name == "outerMesh")
        return Key::OuterMesh;
    if (name == "innerPerimeter")
        return Key::InnerPerimeter;
    if (name == "branch")
        return Key::Branch;
    if (name == "thickness")
        return Key::Thickness;
    if (name == "core")
        return Key::Core;
    if (name == "layer")
        return Key::Layer;
    if (name == "nonStructMass")
        return Key::NonstructuralMass;
    return std::nullopt;
}

// True for the keywords that the format reference lets carry an "(id)".
bool takesLabel(ArbitrarySectionKey key) noexcept
{
    using Key = ArbitrarySectionKey;
    return key == Key::InnerPerimeter || key == Key::Branch || key == Key::Thickness || key == Key::Core ||
           key == Key::Layer;
}

// "points=(start,end)".
std::optional<SectionSegment> parseSegment(std::string_view text)
{
    if (text.size() < 11 || !text.starts_with("points=(") || text.back() != ')')
        return std::nullopt;
    const std::string_view inner = text.substr(8, text.size() - 9);
    const std::size_t comma = inner.find(',');
    if (comma == std::string_view::npos)
        return std::nullopt;
    const std::optional<base::EntityId> start = parseId(inner.substr(0, comma));
    const std::optional<base::EntityId> end = parseId(inner.substr(comma + 1));
    if (!start || !end)
        return std::nullopt;
    return SectionSegment{*start, *end};
}

// The value of a thickness or a core: a number, or "[number,points=(start,end)]". `integer` says whether the number is
// an ID.
std::optional<CardProblem> parseNumberAndSegment(std::string_view value, bool integer, ArbitrarySectionItem& item)
{
    const char* field = sectionKeyName(item.key);
    const auto setNumber = [&](std::string_view text) -> bool
    {
        if (integer)
        {
            const std::optional<base::EntityId> id = parseId(text);
            if (id)
                item.target = *id;
            return id.has_value();
        }
        const std::optional<double> real = base::NumberParser::parseReal(text);
        if (real)
            item.value = *real;
        return real.has_value();
    };
    const std::string what = integer ? "an integer" : "a number";
    if (value.empty() || value.front() != '[')
    {
        if (!setNumber(value))
            return problemOf(field, std::string(field) + " needs " + what + " or [" + (integer ? "ID" : "value") +
                                        ",points=(start,end)], found \"" + std::string(value) + "\"");
        return std::nullopt;
    }
    std::optional<std::vector<std::string_view>> parts;
    if (value.back() == ']')
        parts = splitTopLevel(value.substr(1, value.size() - 2));
    if (!parts || parts->size() != 2)
        return problemOf(field, std::string(field) + " needs [" + (integer ? "ID" : "value") +
                                    ",points=(start,end)], found \"" + std::string(value) + "\"");
    if (!setNumber((*parts)[0]))
        return problemOf(field, std::string(field) + " needs " + what + " in front of points=, found \"" +
                                    std::string((*parts)[0]) + "\"");
    item.segment = parseSegment((*parts)[1]);
    if (!item.segment)
        return problemOf(field, std::string(field) + " needs points=(start,end) with two POINT IDs, found \"" +
                                    std::string((*parts)[1]) + "\"");
    return std::nullopt;
}

// One item "KEY=value" or "KEY(id)=value".
std::optional<CardProblem> parseItem(std::string_view part, unsigned allowedKeys, ArbitrarySectionItem& item)
{
    const std::size_t equals = part.find('=');
    if (equals == std::string_view::npos)
        return problemOf("description", "\"" + std::string(part) + "\" is not an item of the form KEY=value");
    const std::string_view head = part.substr(0, equals);
    const std::string_view value = part.substr(equals + 1);
    std::string_view name = head;
    std::optional<std::string_view> labelText;
    if (const std::size_t open = head.find('('); open != std::string_view::npos)
    {
        if (head.back() != ')')
            return problemOf("description", "the (id) of \"" + std::string(head) + "\" is not closed");
        name = head.substr(0, open);
        labelText = head.substr(open + 1, head.size() - open - 2);
    }
    const std::optional<ArbitrarySectionKey> key = keyOf(name);
    if (!key)
        return problemOf("description", "\"" + std::string(name) +
                                            "\" is not a keyword of the description (outerPerimeter, outerMesh, "
                                            "innerPerimeter, branch, thickness, core, layer, nonStructMass)");
    const char* field = sectionKeyName(*key);
    if ((allowedKeys & sectionKeyBit(*key)) == 0)
        return problemOf(field, std::string(field) + " is not a keyword of this card");
    item.key = *key;
    if (labelText)
    {
        if (!takesLabel(*key))
            return problemOf(field, std::string(field) + " takes no (id)");
        const std::optional<base::EntityId> label = parseId(*labelText);
        if (!label || *label < 1)
            return problemOf(field, "the (id) of " + std::string(field) + " must be a positive integer");
        item.label = *label;
    }
    switch (*key)
    {
    case ArbitrarySectionKey::OuterPerimeter:
    case ArbitrarySectionKey::OuterMesh:
    case ArbitrarySectionKey::InnerPerimeter:
    case ArbitrarySectionKey::Branch:
    {
        const std::optional<base::EntityId> id = parseId(value);
        if (!id)
            return problemOf(field, std::string(field) + " needs an integer, found \"" + std::string(value) + "\"");
        item.target = *id;
        return std::nullopt;
    }
    case ArbitrarySectionKey::NonstructuralMass:
    {
        const std::optional<double> real = base::NumberParser::parseReal(value);
        if (!real)
            return problemOf(field, "nonStructMass needs a number, found \"" + std::string(value) + "\"");
        item.value = *real;
        return std::nullopt;
    }
    case ArbitrarySectionKey::Thickness:
        return parseNumberAndSegment(value, false, item);
    case ArbitrarySectionKey::Core:
        return parseNumberAndSegment(value, true, item);
    case ArbitrarySectionKey::Layer:
    {
        std::optional<std::vector<std::string_view>> parts;
        if (!value.empty() && value.front() == '[' && value.back() == ']')
            parts = splitTopLevel(value.substr(1, value.size() - 2));
        if (!parts || parts->size() != 2)
            return problemOf(field, "layer needs [PCID,SETID], found \"" + std::string(value) + "\"");
        const std::optional<base::EntityId> pcid = parseId((*parts)[0]);
        const std::optional<base::EntityId> set = parseId((*parts)[1]);
        if (!pcid || !set)
            return problemOf(field, "layer needs two integers, [PCID,SETID], found \"" + std::string(value) + "\"");
        item.target = *pcid;
        item.layerSet = *set;
        return std::nullopt;
    }
    }
    return std::nullopt;
}

} // namespace

const char* sectionKeyName(ArbitrarySectionKey key) noexcept
{
    switch (key)
    {
    case ArbitrarySectionKey::OuterPerimeter:
        return "outerPerimeter";
    case ArbitrarySectionKey::OuterMesh:
        return "outerMesh";
    case ArbitrarySectionKey::InnerPerimeter:
        return "innerPerimeter";
    case ArbitrarySectionKey::Branch:
        return "branch";
    case ArbitrarySectionKey::Thickness:
        return "thickness";
    case ArbitrarySectionKey::Core:
        return "core";
    case ArbitrarySectionKey::Layer:
        return "layer";
    case ArbitrarySectionKey::NonstructuralMass:
        return "nonStructMass";
    }
    return "";
}

std::string printSectionItem(const ArbitrarySectionItem& item)
{
    std::string text = sectionKeyName(item.key);
    if (item.label != 0)
        text += "(" + std::to_string(item.label) + ")";
    text += "=";
    const auto withSegment = [&](const std::string& number)
    {
        if (!item.segment)
            return number;
        return "[" + number + ",points=(" + std::to_string(item.segment->start) + "," +
               std::to_string(item.segment->end) + ")]";
    };
    switch (item.key)
    {
    case ArbitrarySectionKey::OuterPerimeter:
    case ArbitrarySectionKey::OuterMesh:
    case ArbitrarySectionKey::InnerPerimeter:
    case ArbitrarySectionKey::Branch:
        text += std::to_string(item.target);
        break;
    case ArbitrarySectionKey::Thickness:
        text += withSegment(base::NumberFormatter::formatRealFree(item.value));
        break;
    case ArbitrarySectionKey::Core:
        text += withSegment(std::to_string(item.target));
        break;
    case ArbitrarySectionKey::Layer:
        text += "[" + std::to_string(item.target) + "," + std::to_string(item.layerSet) + "]";
        break;
    case ArbitrarySectionKey::NonstructuralMass:
        text += base::NumberFormatter::formatRealFree(item.value);
        break;
    }
    return text;
}

std::vector<std::string> printSectionItems(const std::vector<ArbitrarySectionItem>& items)
{
    std::vector<std::string> texts;
    texts.reserve(items.size());
    for (const ArbitrarySectionItem& item : items)
        texts.push_back(printSectionItem(item));
    return texts;
}

std::optional<CardProblem> parseSectionText(std::string_view text, unsigned allowedKeys,
                                            std::vector<ArbitrarySectionItem>& items)
{
    items.clear();
    std::string compact;
    const auto wordCharacter = [](char c)
    { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); };
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const char character = text[index];
        if (!base::isAsciiSpace(character))
        {
            compact += character;
            continue;
        }
        std::size_t next = index + 1;
        while (next < text.size() && base::isAsciiSpace(text[next]))
            ++next;
        if (!compact.empty() && next < text.size() && wordCharacter(compact.back()) && wordCharacter(text[next]))
            return problemOf("description", "a name or number cannot contain whitespace");
    }
    const std::optional<std::vector<std::string_view>> parts = splitTopLevel(compact);
    if (!parts)
        return problemOf("description", "the parentheses or brackets of the description do not match");
    for (const std::string_view part : *parts)
    {
        if (part.empty()) // a comma at the end of a line, or two in a row (a common deck has "], nonStructMass")
            continue;
        ArbitrarySectionItem item;
        if (const auto problem = parseItem(part, allowedKeys, item))
            return problem;
        items.push_back(item);
    }
    return std::nullopt;
}

std::vector<ArbitrarySectionItem> readSectionItems(FieldReader& reader, int firstField, unsigned allowedKeys)
{
    std::vector<ArbitrarySectionItem> items;
    if (const auto problem = parseSectionText(reader.text(firstField), allowedKeys, items))
        reader.error(problem->code, problem->message, problem->field);
    return items;
}

void writeSectionItems(FieldWriter& writer, int firstField, const std::vector<ArbitrarySectionItem>& items)
{
    std::string text;
    for (const ArbitrarySectionItem& item : items)
    {
        if (!text.empty())
            text += ',';
        text += printSectionItem(item);
    }
    writer.text(firstField, text);
}

std::optional<CardProblem> checkSectionItems(const std::vector<ArbitrarySectionItem>& items, ArbitrarySectionForm form,
                                             unsigned allowedKeys)
{
    using Key = ArbitrarySectionKey;
    std::size_t outer = 0;
    std::size_t mesh = 0;
    std::size_t mass = 0;
    bool otherDescription = false; // an item other than outerMesh and nonStructMass
    for (const ArbitrarySectionItem& item : items)
    {
        const char* field = sectionKeyName(item.key);
        if ((allowedKeys & sectionKeyBit(item.key)) == 0)
            return problemOf(field, std::string(field) + " is not a keyword of this card");
        if (item.key == Key::InnerPerimeter && form != ArbitrarySectionForm::General)
            return problemOf(field, "innerPerimeter is for a general section (GeneralSection) only");
        const bool forProfile =
            item.key == Key::Branch || item.key == Key::Thickness || item.key == Key::Core || item.key == Key::Layer;
        if (forProfile && form == ArbitrarySectionForm::General)
            return problemOf(
                field, std::string(field) + " is for an open or closed profile (OpenProfile or ClosedProfile) only");
        if (item.label < 0 || (item.label != 0 && !takesLabel(item.key)))
            return problemOf(field, "the (id) of " + std::string(field) + " must be a positive integer");
        if (item.key != Key::OuterMesh && item.key != Key::NonstructuralMass)
            otherDescription = true;
        switch (item.key)
        {
        case Key::OuterPerimeter:
            ++outer;
            break;
        case Key::OuterMesh:
            ++mesh;
            break;
        case Key::NonstructuralMass:
            ++mass;
            if (const auto problem = checkReal(item.value, field, RealRule::Positive))
                return problem;
            break;
        case Key::Thickness:
            if (const auto problem = checkReal(item.value, field, RealRule::Positive))
                return problem;
            break;
        default:
            break;
        }
        if (item.key != Key::NonstructuralMass && item.key != Key::Thickness && item.target <= 0)
            return CardProblem{"card.bad-id", std::string(field) + " must name a positive ID", field};
        if (item.key == Key::Layer && item.layerSet <= 0)
            return CardProblem{"card.bad-id", "the SETID of a layer must be a positive ID", field};
        if (item.segment && (item.segment->start <= 0 || item.segment->end <= 0))
            return CardProblem{"card.bad-id", "the POINT IDs of points=(start,end) must be positive", field};
    }
    if (outer > 1)
        return problemOf("outerPerimeter", "outerPerimeter is given twice");
    if (mesh > 1)
        return problemOf("outerMesh", "outerMesh is given twice");
    if (mass > 1)
        return problemOf("nonStructMass", "nonStructMass is given twice");
    if (outer == 0 && mesh == 0)
        return CardProblem{"card.missing-field", "the description needs outerPerimeter (or outerMesh)",
                           "outerPerimeter"};
    // format reference: outerMesh must not appear together with the other keywords of the geometry, such as
    // outerPerimeter or innerPerimeter.
    if (mesh == 1 && (outer > 0 || otherDescription))
        return problemOf("outerMesh",
                         "outerMesh must not appear together with outerPerimeter, innerPerimeter or the other keywords "
                         "of the section");
    // An item without an (id) stands alone: the format reference leaves the (id) out when there is only one item. Two
    // items of a keyword need distinct (id)s, and at most one of them may do without.
    for (std::size_t first = 0; first < items.size(); ++first)
    {
        if (!takesLabel(items[first].key))
            continue;
        for (std::size_t second = first + 1; second < items.size(); ++second)
        {
            if (items[second].key == items[first].key && items[second].label == items[first].label)
                return problemOf(sectionKeyName(items[first].key),
                                 std::string("the items of ") + sectionKeyName(items[first].key) +
                                     " need distinct (id)s, and only one of them may have none");
        }
    }
    return std::nullopt;
}

void visitSectionItems(std::vector<ArbitrarySectionItem>& items, ReferenceVisitor& visitor)
{
    const auto visitSegment = [&](std::optional<SectionSegment>& segment)
    {
        if (!segment)
            return;
        visitId(visitor, "startPoint", ReferenceTarget::Node, segment->start);
        visitId(visitor, "endPoint", ReferenceTarget::Node, segment->end);
    };
    for (ArbitrarySectionItem& item : items)
    {
        switch (item.key)
        {
        case ArbitrarySectionKey::OuterPerimeter:
            visitId(visitor, "outerPerimeter", ReferenceTarget::Set, item.target);
            break;
        case ArbitrarySectionKey::OuterMesh:
            break; // names a BEGIN ARBMODEL section, which no card owns
        case ArbitrarySectionKey::InnerPerimeter:
            visitId(visitor, "innerPerimeter", ReferenceTarget::Set, item.target);
            break;
        case ArbitrarySectionKey::Branch:
            visitId(visitor, "branch", ReferenceTarget::Set, item.target);
            break;
        case ArbitrarySectionKey::Thickness:
            visitSegment(item.segment);
            break;
        case ArbitrarySectionKey::Core:
            visitId(visitor, "core", ReferenceTarget::Property, item.target);
            visitSegment(item.segment);
            break;
        case ArbitrarySectionKey::Layer:
            visitId(visitor, "layer", ReferenceTarget::Property, item.target);
            visitId(visitor, "layerSet", ReferenceTarget::Set, item.layerSet);
            break;
        case ArbitrarySectionKey::NonstructuralMass:
            break;
        }
    }
}

} // namespace oor::model
