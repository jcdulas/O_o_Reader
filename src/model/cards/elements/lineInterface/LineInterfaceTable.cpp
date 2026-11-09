#include "oor/model/cards/elements/lineInterface/LineInterfaceTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <charconv>
#include <cstdint>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int BOUNDARY_EXPRESSION_FIELD = 9; // the boundary expression starts on the first continuation line
constexpr std::size_t MAX_BOUNDARY_ID_DIGITS = 10;

bool isDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

// One item of the boundary expression, blanks already removed: id or id(Linear) or id(Quadratic), the BID with an
// optional sign.
std::optional<LineInterfaceBoundary> parseItem(std::string_view item, std::string& problem)
{
    const std::string shown = "the boundary \"" + std::string(item) + "\" of the boundary expression";
    std::size_t position = 0;
    bool negative = false;
    if (position < item.size() && (item[position] == '+' || item[position] == '-'))
    {
        negative = item[position] == '-';
        ++position;
    }
    const std::size_t digitsBegin = position;
    while (position < item.size() && isDigit(item[position]))
        ++position;
    const std::size_t digitCount = position - digitsBegin;
    if (digitCount == 0 || digitCount > MAX_BOUNDARY_ID_DIGITS)
    {
        problem = shown + " must start with a non-zero integer (BID)";
        return std::nullopt;
    }
    std::int64_t value = 0;
    std::from_chars(item.data() + digitsBegin, item.data() + position, value);
    if (value == 0 || value > std::numeric_limits<base::EntityId>::max())
    {
        problem = shown + " must have a BID that is a non-zero 32-bit integer";
        return std::nullopt;
    }
    LineInterfaceBoundary boundary;
    boundary.boundaryId = static_cast<base::EntityId>(negative ? -value : value);
    if (position == item.size())
        return boundary; // no parentheses: linear
    if (item.size() - position < 3 || item[position] != '(' || item[item.size() - 1] != ')')
    {
        problem = shown + " must be id or id(Linear) or id(Quadratic)";
        return std::nullopt;
    }
    const std::optional<LineInterfaceInterpolation> interpolation = parseKeyword<LineInterfaceInterpolation>(
        LINE_INTERFACE_INTERPOLATION_WORDS, item.substr(position + 1, item.size() - position - 2));
    if (!interpolation)
    {
        problem = shown + " has an interpolation that is not Linear or Quadratic";
        return std::nullopt;
    }
    boundary.interpolation = *interpolation;
    return boundary;
}

} // namespace

std::optional<std::vector<LineInterfaceBoundary>> parseLineInterfaceList(std::string_view text,
                                                                             std::string& problem)
{
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
        {
            problem = "a name or number cannot contain whitespace";
            return std::nullopt;
        }
    }
    constexpr std::string_view OPENING = "boundaries=(";
    const std::string_view whole = compact;
    if (whole.size() <= OPENING.size() || !whole.starts_with(OPENING) || whole.back() != ')')
    {
        problem = "the boundary expression must read boundaries=(id1(Linear),id2(Quadratic),...)";
        return std::nullopt;
    }
    const std::string_view body = whole.substr(OPENING.size(), whole.size() - OPENING.size() - 1);
    std::vector<LineInterfaceBoundary> boundaries;
    int depth = 0; // an item ends at a comma outside the parentheses of its interpolation
    std::size_t begin = 0;
    for (std::size_t index = 0; index <= body.size(); ++index)
    {
        if (index < body.size() && !(body[index] == ',' && depth == 0))
        {
            if (body[index] == '(')
                ++depth;
            else if (body[index] == ')')
                --depth;
            continue;
        }
        const std::optional<LineInterfaceBoundary> boundary = parseItem(body.substr(begin, index - begin), problem);
        if (!boundary)
            return std::nullopt;
        boundaries.push_back(*boundary);
        begin = index + 1;
    }
    return boundaries;
}

std::string formatLineInterfaceList(const std::vector<LineInterfaceBoundary>& boundaries, bool explicitLinear)
{
    std::string text = "boundaries=(";
    for (std::size_t index = 0; index < boundaries.size(); ++index)
    {
        if (index > 0)
            text += ',';
        text += std::to_string(boundaries[index].boundaryId);
        if (boundaries[index].interpolation == LineInterfaceInterpolation::Quadratic)
            text += "(Quadratic)";
        else if (explicitLinear)
            text += "(Linear)";
    }
    text += ')';
    return text;
}

std::optional<LineInterfaceCard> LineInterfaceFormat::read(FieldReader& reader)
{
    LineInterfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "kind", LINE_INTERFACE_TYPE_WORDS, LineInterfaceType::GridList);
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, reader.slotCount());
    const std::string text = reader.spanText(9, reader.lastUsedField());
    if (text.empty())
        reader.error("card.missing-field", "boundaryList is required", "boundaryList");
    else
    {
        std::string problem;
        const auto boundaries = parseLineInterfaceList(text, problem);
        if (boundaries)
            card.boundaries = *boundaries;
        else
            reader.error("card.bad-value", problem, "boundaryList");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LineInterfaceFormat::check(const LineInterfaceCard& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    // at least two boundaries.
    if (card.boundaries.size() < 2)
        return CardProblem{"card.bad-value", "the card needs at least two boundaries in its {field}", "boundaryList"};
    for (const LineInterfaceBoundary& boundary : card.boundaries)
    {
        if (boundary.boundaryId == 0)
            return CardProblem{"card.bad-id", "the BID of a boundary must not be zero", "boundaryList"};
    }
    return std::nullopt;
}

void LineInterfaceFormat::write(const LineInterfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, LINE_INTERFACE_TYPE_WORDS, card.type, LineInterfaceType::GridList);
    // Linear is the blank default of an interpolation: written in parentheses only when defaults are written.
    writer.text(BOUNDARY_EXPRESSION_FIELD, formatLineInterfaceList(card.boundaries, !writer.omitsDefaults()));
}

std::uint32_t LineInterfaceTable::append(const LineInterfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void LineInterfaceTable::store(std::uint32_t row, const LineInterfaceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void LineInterfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void LineInterfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (LineInterfaceBoundary& boundary : m_cards.edit(row).boundaries)
    {
        const base::EntityId updated =
            visitor.visit("boundaryList", ReferenceTarget::GeometricBoundary, std::abs(boundary.boundaryId));
        boundary.boundaryId = boundary.boundaryId < 0 ? -updated : updated;
    }
}

void LineInterfaceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
