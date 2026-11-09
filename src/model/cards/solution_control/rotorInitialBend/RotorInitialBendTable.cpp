#include "oor/model/cards/solution_control/rotorInitialBend/RotorInitialBendTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_ROW_FIELD = 9; // the keyword of the first row

// The keywords of the rows, in the order the card writes them.
enum class RowKind : std::uint8_t
{
    Unbcord,
    Kink,
    Offset,
    Brgdpr
};

constexpr std::array<std::string_view, 4> ROW_WORDS{"UnbalanceFrame", "KinkDeformation", "Offset",
                                                    "BearingDeflectionProfile"};

bool isBlankLine(const FieldReader& reader, int line)
{
    for (int field = line; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// A row has its keyword and `values` values; the rest of the line is blank.
void requireBlankRest(FieldReader& reader, int line, int values, std::string_view keyword)
{
    for (int field = line + 1 + values; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "a " + std::string(keyword) + " row of a " + std::string(reader.cardName()) + " has " +
                             std::to_string(values) + " values",
                         "lineKeyword");
            return;
        }
    }
}

} // namespace

std::optional<RotorInitialBendCard> RotorInitialBendFormat::read(FieldReader& reader)
{
    RotorInitialBendCard card;
    card.id = reader.requiredInteger(1, "bendTableId");
    card.rotorId = reader.requiredInteger(2, "rotorId");
    card.loadCombinationId = reader.requiredInteger(3, "loadId");
    for (int field = 4; field < FIRST_ROW_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "the first line of a " + std::string(reader.cardName()) + " has the three numbers only", "");
            break;
        }
    }
    std::array<bool, ROW_WORDS.size()> seen{};
    std::optional<RowKind> current;
    for (int line = FIRST_ROW_FIELD; line <= reader.lastUsedField() && !reader.failed(); line += FIELDS_PER_LINE)
    {
        if (isBlankLine(reader, line))
            continue; // padding
        if (!reader.isBlank(line))
        {
            current = parseKeyword<RowKind>(ROW_WORDS, reader.text(line));
            if (!current)
            {
                reader.error("card.bad-value",
                             "a row of a " + std::string(reader.cardName()) + " starts with " + keywordList(ROW_WORDS) +
                                 ", not \"" + std::string(reader.text(line)) + "\"",
                             "lineKeyword");
                break;
            }
            if (seen[static_cast<std::size_t>(*current)])
            {
                reader.error("card.bad-value",
                             std::string(keywordText(ROW_WORDS, *current)) +
                                 " is given once: its other rows follow it with a blank first field",
                             "lineKeyword");
                break;
            }
            seen[static_cast<std::size_t>(*current)] = true;
        }
        else if (!current)
        {
            reader.error("card.bad-value",
                         "the first row of a " + std::string(reader.cardName()) + " starts with a keyword",
                         "lineKeyword");
            break;
        }
        const std::string_view word = keywordText(ROW_WORDS, *current);
        switch (*current)
        {
        case RowKind::Unbcord:
            if (card.planeVector)
            {
                reader.error("card.bad-value", "UNBCORD has one row", "lineKeyword");
                break;
            }
            card.planeVector = RotorPlaneVector{{reader.requiredReal(line + 1, "coordinate1"),
                                                 reader.requiredReal(line + 2, "coordinate2"),
                                                 reader.requiredReal(line + 3, "coordinate3")}};
            requireBlankRest(reader, line, 3, word);
            break;
        case RowKind::Kink:
            card.kinks.push_back(RotorKink{reader.requiredReal(line + 1, "station"),
                                           reader.requiredReal(line + 2, "angle"),
                                           reader.requiredReal(line + 3, "phase")});
            requireBlankRest(reader, line, 3, word);
            break;
        case RowKind::Offset:
            card.offsets.push_back(RotorOffset{reader.requiredReal(line + 1, "station"),
                                               reader.requiredReal(line + 2, "offset"),
                                               reader.requiredReal(line + 3, "phase")});
            requireBlankRest(reader, line, 3, word);
            break;
        case RowKind::Brgdpr:
            card.bearings.push_back(RotorBearingPair{reader.requiredInteger(line + 1, "rotorNode"),
                                                     reader.requiredInteger(line + 2, "statorNode")});
            requireBlankRest(reader, line, 2, word);
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorInitialBendFormat::check(const RotorInitialBendCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "bendTableId"))
        return problem;
    if (const auto problem = checkRequiredId(card.rotorId, "rotorId"))
        return problem;
    if (const auto problem = checkRequiredId(card.loadCombinationId, "loadId"))
        return problem;
    if (card.planeVector)
    {
        static constexpr std::array<const char*, 3> NAMES{"coordinate1", "coordinate2", "coordinate3"};
        for (std::size_t index = 0; index < NAMES.size(); ++index)
        {
            if (const auto problem = checkReal(card.planeVector->components[index], NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    for (const RotorKink& kink : card.kinks)
    {
        if (const auto problem = checkReal(kink.station, "station", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(kink.angle, "angle", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(kink.phase, "phase", RealRule::Finite))
            return problem;
    }
    for (const RotorOffset& offset : card.offsets)
    {
        if (const auto problem = checkReal(offset.station, "station", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(offset.value, "offset", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(offset.phase, "phase", RealRule::Finite))
            return problem;
    }
    if (card.bearings.empty())
        return CardProblem{"card.missing-field", "the card needs a BRGDPR row: a pair of rotor and stator grid points",
                           "rotorNode"};
    for (const RotorBearingPair& pair : card.bearings)
    {
        if (const auto problem = checkRequiredId(pair.rotorNode, "rotorNode"))
            return problem;
        if (const auto problem = checkRequiredId(pair.statorNode, "statorNode"))
            return problem;
    }
    return std::nullopt;
}

void RotorInitialBendFormat::write(const RotorInitialBendCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.rotorId);
    writer.integer(3, card.loadCombinationId);
    int line = FIRST_ROW_FIELD;
    // The keyword is on the first row of its kind only.
    if (card.planeVector)
    {
        writer.text(line, keywordText(ROW_WORDS, RowKind::Unbcord));
        for (std::size_t index = 0; index < card.planeVector->components.size(); ++index)
            writer.real(line + 1 + static_cast<int>(index), card.planeVector->components[index]);
        line += FIELDS_PER_LINE;
    }
    for (std::size_t index = 0; index < card.kinks.size(); ++index, line += FIELDS_PER_LINE)
    {
        if (index == 0)
            writer.text(line, keywordText(ROW_WORDS, RowKind::Kink));
        writer.real(line + 1, card.kinks[index].station);
        writer.real(line + 2, card.kinks[index].angle);
        writer.real(line + 3, card.kinks[index].phase);
    }
    for (std::size_t index = 0; index < card.offsets.size(); ++index, line += FIELDS_PER_LINE)
    {
        if (index == 0)
            writer.text(line, keywordText(ROW_WORDS, RowKind::Offset));
        writer.real(line + 1, card.offsets[index].station);
        writer.real(line + 2, card.offsets[index].value);
        writer.real(line + 3, card.offsets[index].phase);
    }
    for (std::size_t index = 0; index < card.bearings.size(); ++index, line += FIELDS_PER_LINE)
    {
        if (index == 0)
            writer.text(line, keywordText(ROW_WORDS, RowKind::Brgdpr));
        writer.integer(line + 1, card.bearings[index].rotorNode);
        writer.integer(line + 2, card.bearings[index].statorNode);
    }
}

std::uint32_t RotorInitialBendTable::append(const RotorInitialBendCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_rotorId.push(card.rotorId);
    m_loadCombinationId.push(card.loadCombinationId);
    m_cards.push(card);
    return row;
}

void RotorInitialBendTable::store(std::uint32_t row, const RotorInitialBendCard& card)
{
    m_id.set(row, card.id);
    m_rotorId.set(row, card.rotorId);
    m_loadCombinationId.set(row, card.loadCombinationId);
    m_cards.set(row, card);
}

void RotorInitialBendTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_rotorId.keepRows(rows);
    m_loadCombinationId.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorInitialBendTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RotorInitialBendTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RotorInitialBendCard& card = m_cards.edit(row);
    visitId(visitor, "rotorId", ReferenceTarget::Rotor, card.rotorId);
    visitId(visitor, "loadId", ReferenceTarget::LoadCombinationSet, card.loadCombinationId);
    m_rotorId.set(row, card.rotorId);
    m_loadCombinationId.set(row, card.loadCombinationId);
    for (RotorBearingPair& pair : card.bearings)
    {
        visitId(visitor, "rotorNode", ReferenceTarget::Node, pair.rotorNode);
        visitId(visitor, "statorNode", ReferenceTarget::Node, pair.statorNode);
    }
}

} // namespace oor::model
