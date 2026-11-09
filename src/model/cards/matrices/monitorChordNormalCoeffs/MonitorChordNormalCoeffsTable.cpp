#include "oor/model/cards/matrices/monitorChordNormalCoeffs/MonitorChordNormalCoeffsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"
#include "oor/model/core/Model.hpp"

#include <set>

namespace oor::model
{

namespace
{

constexpr int MOMENT_CHORD_FRACTION_FIELD = 9; // the first field of the continuation line
constexpr int FIRST_AERO_PANEL_FIELD = 10;
constexpr double DEFAULT_MOMENT_CHORD_FRACTION = 0.25; // the quarter chord
constexpr std::string_view ALL_WORD = "All";

} // namespace

std::optional<MonitorChordNormalCoeffsCard> MonitorChordNormalCoeffsFormat::read(FieldReader& reader)
{
    MonitorChordNormalCoeffsCard card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.momentChordFraction =
        reader.real(MOMENT_CHORD_FRACTION_FIELD, "momentChordFraction", DEFAULT_MOMENT_CHORD_FRACTION);
    // The CAERIDs fill the fields from the tenth on; a blank field is the padding of a continuation line. The word ALL
    // is the first of them, and the only one.
    const int last = reader.lastUsedField();
    for (int field = FIRST_AERO_PANEL_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == ALL_WORD))
        {
            if (!card.aeroPanelIds.empty() || card.allStrips)
                reader.error("card.bad-value", "ALL can only be the first " + std::string("aeroPanelIds"),
                             "aeroPanelIds");
            card.allStrips = true;
            continue;
        }
        card.aeroPanelIds.push_back(reader.requiredInteger(field, "aeroPanelIds"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorChordNormalCoeffsFormat::check(const MonitorChordNormalCoeffsCard& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (const auto problem = checkRealRange(card.momentChordFraction, "momentChordFraction", 0.0, 1.0))
        return problem;
    if (card.allStrips && !card.aeroPanelIds.empty())
        return CardProblem{"card.bad-value", "no {field} may follow ALL", "aeroPanelIds"};
    if (!card.allStrips && card.aeroPanelIds.empty())
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required: the ID of a lifting surface panel, or ALL",
                           "aeroPanelIds"};
    for (const base::EntityId id : card.aeroPanelIds)
    {
        if (const auto problem = checkRequiredId(id, "aeroPanelIds"))
            return problem;
    }
    return std::nullopt;
}

void MonitorChordNormalCoeffsFormat::write(const MonitorChordNormalCoeffsCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writer.realUnlessDefault(MOMENT_CHORD_FRACTION_FIELD, card.momentChordFraction, DEFAULT_MOMENT_CHORD_FRACTION);
    if (card.allStrips)
    {
        writer.text(FIRST_AERO_PANEL_FIELD, ALL_WORD);
        return;
    }
    int field = FIRST_AERO_PANEL_FIELD;
    for (const base::EntityId id : card.aeroPanelIds)
        writer.integer(field++, id);
}

std::uint32_t MonitorChordNormalCoeffsTable::append(const MonitorChordNormalCoeffsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void MonitorChordNormalCoeffsTable::store(std::uint32_t row, const MonitorChordNormalCoeffsCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void MonitorChordNormalCoeffsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

void MonitorChordNormalCoeffsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The format reference supports CAERO1 elements only; the target is every aerodynamic element, and a rule between
    // cards says which ones are CAERO1.
    for (base::EntityId& id : m_cards.edit(row).aeroPanelIds)
        visitId(visitor, "aeroPanelIds", ReferenceTarget::AeroElement, id);
}

void MonitorChordNormalCoeffsTable::finishRead(Model&, const ReadContext& context)
{
    const std::string card = std::string(NAME);
    std::set<std::string> names;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        const std::string name = m_name.get(row);
        if (!names.insert(name).second && reported.insert(name).second)
            context.sink.error("deck.duplicate-monitor-chord-normal-coeffs",
                               "the " + card + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
