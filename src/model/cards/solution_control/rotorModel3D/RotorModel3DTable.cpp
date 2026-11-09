#include "oor/model/cards/solution_control/rotorModel3D/RotorModel3DTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

namespace
{

// The lists of ELEM and PROP have THRU and no BY, and go up.
constexpr ContactRunRules RULES{.allowBy = false, .allowDescending = false};

constexpr int FIELDS_ON_FIRST_LINE = 8;
constexpr int FIRST_SECTION_FIELD = 9; // the keyword of the first continuation line
constexpr int MINIMUM_AXIS_GRIDS = 2;

// Writes a keyword line and the runs after it: the keyword in the first field of the line, the IDs in the seven fields
// that follow and on the lines after it (their first field blank). A range stays on one line (three fields), so one
// that does not fit in what is left of a line starts the next one. Returns the first field of the next free line.
int writeRunSection(FieldWriter& writer, int lineStart, std::string_view keyword, const std::vector<ContactRun>& runs)
{
    writer.text(lineStart, keyword);
    int line = lineStart;
    int column = 1; // the first field of a line is the keyword's
    for (const ContactRun& run : runs)
    {
        const int width = run.isRange() ? 3 : 1;
        if (column + width > CONTACT_LINE_FIELDS)
        {
            line += CONTACT_LINE_FIELDS;
            column = 1;
        }
        writer.integer(line + column++, run.first);
        if (width == 3)
        {
            writer.text(line + column++, "Through");
            writer.integer(line + column++, run.last);
        }
    }
    return line + CONTACT_LINE_FIELDS;
}

} // namespace

std::optional<RotorModel3DCard> RotorModel3DFormat::read(FieldReader& reader)
{
    RotorModel3DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frame = readKeyword<RotorModel3DFrame>(reader, 2, "frame", ROTOR_FRAME_WORDS, RotorModel3DFrame::Fixed);
    for (int field = 3; field <= FIELDS_ON_FIRST_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "the first line of a " + std::string(reader.cardName()) + " has the " + std::string("id") +
                             " and the FRAME only",
                         "");
            break;
        }
    }

    const std::vector<ContactSection> sections = splitContactSections(reader, {"Elements", "Property", "Axis"});
    if (sections.front().last > FIELDS_ON_FIRST_LINE)
        reader.error(
            "card.bad-value",
            "the lines of a " + std::string(reader.cardName()) + " after the first start with ELEM, PROP or AXIS",
            "listKind");
    bool propertiesSeen = false;
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
            continue;
        const int firstValue = section.first + 1;
        if (section.keyword == "Axis")
        {
            const std::vector<base::EntityId> grids = readContactIds(reader, firstValue, section.last, "ids");
            if (grids.empty())
                reader.error("card.missing-field", "AXIS needs grid points", "ids");
            card.axis.insert(card.axis.end(), grids.begin(), grids.end());
            continue;
        }
        const bool isElements = section.keyword == "Elements";
        if (isElements && propertiesSeen)
            reader.error("card.bad-value", "ELEM must come before PROP", "listKind");
        propertiesSeen = propertiesSeen || !isElements;
        const std::vector<ContactRun> runs = readContactRuns(reader, firstValue, section.last, "ids", RULES);
        if (runs.empty())
            reader.error("card.missing-field", section.keyword + " needs element or property IDs", "ids");
        std::vector<ContactRun>& list = isElements ? card.elements : card.properties;
        list.insert(list.end(), runs.begin(), runs.end());
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorModel3DFormat::check(const RotorModel3DCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.elements.empty() && card.properties.empty())
        return CardProblem{"card.missing-field", "the card lists elements (ELEM), properties (PROP) or both",
                           "listKind"};
    if (const auto problem = checkContactRuns(card.elements, "ids", RULES))
        return problem;
    if (const auto problem = checkContactRuns(card.properties, "ids", RULES))
        return problem;
    if (static_cast<int>(card.axis.size()) < MINIMUM_AXIS_GRIDS)
        return CardProblem{"card.missing-field", "the AXIS of the card needs at least two grid points", "ids"};
    for (const base::EntityId grid : card.axis)
    {
        if (const auto problem = checkRequiredId(grid, "ids"))
            return problem;
    }
    return std::nullopt;
}

void RotorModel3DFormat::write(const RotorModel3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, ROTOR_FRAME_WORDS, card.frame, RotorModel3DFrame::Fixed);
    int line = FIRST_SECTION_FIELD;
    if (!card.elements.empty())
        line = writeRunSection(writer, line, "Elements", card.elements);
    if (!card.properties.empty())
        line = writeRunSection(writer, line, "Property", card.properties);
    if (!card.axis.empty())
        writeContactIdList(writer, line, "Axis", card.axis);
}

std::uint32_t RotorModel3DTable::append(const RotorModel3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frame.push(static_cast<std::uint8_t>(card.frame));
    m_cards.push(card);
    return row;
}

void RotorModel3DTable::store(std::uint32_t row, const RotorModel3DCard& card)
{
    m_id.set(row, card.id);
    m_frame.set(row, static_cast<std::uint8_t>(card.frame));
    m_cards.set(row, card);
}

void RotorModel3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frame.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorModel3DTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RotorModel3DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RotorModel3DCard& card = m_cards.edit(row);
    visitContactRuns(card.elements, visitor, "ids", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
    visitContactRuns(card.properties, visitor, "ids", ReferenceTarget::Property, ReferenceTarget::PropertyInRange);
    visitContactIds(card.axis, visitor, "ids", ReferenceTarget::Node);
}

} // namespace oor::model
