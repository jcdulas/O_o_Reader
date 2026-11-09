#include "oor/model/cards/materials/progressiveFailureMat/ProgressiveFailureMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <initializer_list>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line
constexpr int FIRST_ENTRY_FIELD = 9;

struct EntryShape
{
    const char* keyword;
    const char* name;
    std::size_t firstLine;
    std::size_t secondLine;
};

constexpr EntryShape SHAPES[] = {{"Temperature", "temperature", 4, 0},
                                 {"Unidirectional", "unidirectional", 4, 0},
                                 {"WovenFabric", "woven", 4, 0},
                                 {"FiberDirection", "fibreDirection", 5, 4},
                                 {"CrossDirection", "crossDirection", 5, 4},
                                 {"ThicknessDirection", "thicknessDirection", 5, 4},
                                 {"WarpDirection", "warpDirection", 5, 4},
                                 {"FiberCrossShear", "fibreCrossShear", 6, 0},
                                 {"CrossThicknessShear", "crossThicknessShear", 6, 0},
                                 {"ThicknessFiberShear", "thicknessFibreShear", 6, 0},
                                 {"WarpFiberShear", "warpFibreShear", 6, 0},
                                 {"FiberThicknessShear", "fibreThicknessShear", 6, 0},
                                 {"ThicknessWarpShear", "thicknessWarpShear", 6, 0}};

const EntryShape* shapeOf(std::string_view keyword)
{
    for (const EntryShape& shape : SHAPES)
    {
        if ((keyword == shape.keyword))
            return &shape;
    }
    return nullptr;
}

bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

bool anyGiven(const std::vector<std::optional<double>>& values, std::size_t from)
{
    for (std::size_t index = from; index < values.size(); ++index)
    {
        if (values[index])
            return true;
    }
    return false;
}

} // namespace

std::optional<ProgressiveFailureMatCard> ProgressiveFailureMatFormat::read(FieldReader& reader)
{
    ProgressiveFailureMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
        card.failure = std::string(word);
    card.density = reader.real(3, "density", 0.0);
    for (int field = 4; field < FIRST_ENTRY_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return card;
        }
    }
    const int last = reader.lastUsedField();
    int position = FIRST_ENTRY_FIELD;
    while (position <= last && !reader.failed())
    {
        const std::string_view keyword = reader.text(position);
        if (keyword.empty())
        {
            if (lineHasValues(reader, position))
            {
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) + " starts with a keyword (" +
                                 fieldNameList({"temperature", "unidirectional", "woven", "fibreDirection"}) + ", ...)",
                             "");
                return card;
            }
            position += LINE;
            continue;
        }
        const EntryShape* shape = shapeOf(keyword);
        if (shape == nullptr)
        {
            reader.error(
                "card.bad-value",
                "\"" + std::string(keyword) + "\" does not start a line of a " + std::string(reader.cardName()), "");
            return card;
        }
        ProgressiveFailureMatEntry entry;
        entry.keyword = shape->keyword;
        for (std::size_t index = 0; index < shape->firstLine; ++index)
            entry.values.push_back(reader.optionalReal(position + 1 + static_cast<int>(index), shape->name));
        for (int field = position + 1 + static_cast<int>(shape->firstLine); field < position + LINE; ++field)
        {
            if (!reader.isBlank(field))
            {
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of the " + std::string(shape->name) + " line of a " +
                                 std::string(reader.cardName()) + " is not used",
                             shape->name);
                return card;
            }
        }
        position += LINE;
        if (shape->secondLine != 0)
        {
            // The compression line of a direction: a blank first field, then its four values.
            const bool present = position <= last && reader.isBlank(position) && lineHasValues(reader, position);
            for (std::size_t index = 0; index < shape->secondLine; ++index)
                entry.values.push_back(
                    present ? reader.optionalReal(position + 1 + static_cast<int>(index), shape->name) : std::nullopt);
            if (present)
            {
                for (int field = position + 1 + static_cast<int>(shape->secondLine); field < position + LINE; ++field)
                {
                    if (!reader.isBlank(field))
                    {
                        reader.error("card.bad-value",
                                     "field " + std::to_string(field) + " of the compression line of a " +
                                         std::string(reader.cardName()) + " is not used",
                                     shape->name);
                        return card;
                    }
                }
                position += LINE;
            }
        }
        card.entries.push_back(std::move(entry));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ProgressiveFailureMatFormat::check(const ProgressiveFailureMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.failure.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a failure theory name (CUBO)",
                           "failure"};
    if (!std::isfinite(card.density))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "density"};
    int temperature = 0;
    int unidirectional = 0;
    int woven = 0;
    for (const ProgressiveFailureMatEntry& entry : card.entries)
    {
        const EntryShape* shape = shapeOf(entry.keyword);
        if (shape == nullptr)
            return CardProblem{"card.bad-value", "\"" + entry.keyword + "\" is not a keyword of the card", ""};
        if (entry.values.size() != shape->firstLine + shape->secondLine)
            return CardProblem{"card.bad-value",
                               "the " + std::string(CardProblem::FIELD) + " line of the card has " +
                                   std::to_string(shape->firstLine + shape->secondLine) + " values",
                               shape->name};
        for (const std::optional<double>& value : entry.values)
        {
            if (value && !std::isfinite(*value))
                return CardProblem{
                    "card.bad-value",
                    "the values of the " + std::string(CardProblem::FIELD) + " line must be finite numbers",
                    shape->name};
        }
        if (entry.keyword == "Temperature" && ++temperature > 1)
            return CardProblem{"card.bad-value", "the card has one " + std::string(CardProblem::FIELD) + " line",
                               "temperature"};
        if (entry.keyword == "Unidirectional" && ++unidirectional > 1)
            return CardProblem{"card.bad-value", "the card has one " + std::string(CardProblem::FIELD) + " line",
                               "unidirectional"};
        if (entry.keyword == "WovenFabric" && ++woven > 1)
            return CardProblem{"card.bad-value", "the card has one " + std::string(CardProblem::FIELD) + " line",
                               "woven"};
    }
    return std::nullopt;
}

void ProgressiveFailureMatFormat::write(const ProgressiveFailureMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.failure, ProgressiveFailureMatCard::DEFAULT_FAILURE);
    writer.realUnlessDefault(3, card.density, 0.0);
    int position = FIRST_ENTRY_FIELD;
    for (const ProgressiveFailureMatEntry& entry : card.entries)
    {
        const EntryShape* shape = shapeOf(entry.keyword);
        if (shape == nullptr || entry.values.size() != shape->firstLine + shape->secondLine)
            continue; // check() has refused such a card
        writer.text(position, shape->keyword);
        for (std::size_t index = 0; index < shape->firstLine; ++index)
        {
            if (entry.values[index])
                writer.real(position + 1 + static_cast<int>(index), *entry.values[index]);
        }
        position += LINE;
        if (shape->secondLine != 0 && anyGiven(entry.values, shape->firstLine))
        {
            for (std::size_t index = 0; index < shape->secondLine; ++index)
            {
                if (entry.values[shape->firstLine + index])
                    writer.real(position + 1 + static_cast<int>(index), *entry.values[shape->firstLine + index]);
            }
            position += LINE;
        }
    }
}

std::uint32_t ProgressiveFailureMatTable::append(const ProgressiveFailureMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ProgressiveFailureMatTable::store(std::uint32_t row, const ProgressiveFailureMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ProgressiveFailureMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ProgressiveFailureMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
