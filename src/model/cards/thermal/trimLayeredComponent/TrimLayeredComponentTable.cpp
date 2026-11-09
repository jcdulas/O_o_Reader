#include "oor/model/cards/thermal/trimLayeredComponent/TrimLayeredComponentTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_LAYER_FIELD = 9; // the layers start on the second line, one to a line

// The first field of the line of layer number `index` (from 0).
constexpr int layerField(std::size_t index) noexcept
{
    return FIRST_LAYER_FIELD + FIELDS_PER_LINE * static_cast<int>(index);
}

bool lineIsBlank(const FieldReader& reader, int first)
{
    for (int field = first; field < first + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

} // namespace

std::optional<TrimLayeredComponentCard> TrimLayeredComponentFormat::read(FieldReader& reader)
{
    TrimLayeredComponentCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is blank: the layers start on the second line",
                         "");
            break;
        }
    }
    // A layer to a line, in the first four fields; a line with nothing on it is a gap of the layout.
    for (int first = FIRST_LAYER_FIELD; first <= reader.lastUsedField(); first += FIELDS_PER_LINE)
    {
        if (lineIsBlank(reader, first))
            continue;
        TrimLayeredComponentLayer layer;
        layer.globalPlyId = reader.requiredInteger(first, "globalPlyId");
        layer.matId = reader.requiredInteger(first + 1, "plyMatId");
        layer.thickness = reader.requiredReal(first + 2, "plyThickness");
        layer.scaleFactor = reader.real(first + 3, "plyScaleFactor", TRIM_LAYERED_DEFAULT_SCALE);
        for (int field = first + 4; field < first + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
            {
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                                 " is not used: a layer has four fields",
                             "");
                break;
            }
        }
        card.layers.push_back(layer);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TrimLayeredComponentFormat::check(const TrimLayeredComponentCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.layers.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one layer (" + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("plyMatId1") + ", " + CardProblem::fieldNamed("plyThickness1") +
                               ")",
                           "globalPlyId1"};
    std::vector<base::EntityId> numbers;
    numbers.reserve(card.layers.size());
    for (const TrimLayeredComponentLayer& layer : card.layers)
    {
        if (const auto problem = checkRequiredId(layer.globalPlyId, "globalPlyId"))
            return problem;
        if (const auto problem = checkRequiredId(layer.matId, "plyMatId"))
            return problem;
        if (const auto problem = checkReal(layer.thickness, "plyThickness", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(layer.scaleFactor, "plyScaleFactor", RealRule::NonNegative))
            return problem;
        numbers.push_back(layer.globalPlyId);
    }
    // the global layer number is unique among the layers of the entry.
    std::sort(numbers.begin(), numbers.end());
    if (std::adjacent_find(numbers.begin(), numbers.end()) != numbers.end())
        return CardProblem{
            "card.bad-value",
            "a global layer ID (" + std::string(CardProblem::FIELD) + ") is used by two layers of the card",
            "globalPlyId"};
    return std::nullopt;
}

void TrimLayeredComponentFormat::write(const TrimLayeredComponentCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.layers.size(); ++index)
    {
        const TrimLayeredComponentLayer& layer = card.layers[index];
        const int first = layerField(index);
        writer.integer(first, layer.globalPlyId);
        writer.integer(first + 1, layer.matId);
        writer.real(first + 2, layer.thickness);
        writer.realUnlessDefault(first + 3, layer.scaleFactor, TRIM_LAYERED_DEFAULT_SCALE);
    }
}

std::uint32_t TrimLayeredComponentTable::append(const TrimLayeredComponentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TrimLayeredComponentTable::store(std::uint32_t row, const TrimLayeredComponentCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TrimLayeredComponentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TrimLayeredComponentTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TrimLayeredComponentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID of the card is the SET3 it names: the ID column follows the reference.
    TrimLayeredComponentCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Set, card.id);
    m_id.set(row, card.id);
    // The materials are a MATPE1, MAT1 or MAT10.
    for (TrimLayeredComponentLayer& layer : card.layers)
        visitId(visitor, "plyMatId", ReferenceTarget::Material, layer.matId);
}

} // namespace oor::model
