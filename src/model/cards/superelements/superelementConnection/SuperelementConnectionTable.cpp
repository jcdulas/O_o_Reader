#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// The connections are read from the first field after the options (the format reference starts them on the second line,
// field 9, and a card that starts them earlier means the same).
constexpr int FIRST_PAIR_FIELD = 5;
// Where the writer starts them.
constexpr int FIRST_PAIR_FIELD_WRITTEN = 9;
constexpr int FIELDS_PER_LINE = 8;

// The next field from `field` on that is not blank, or past `last`.
int skipBlanks(const FieldReader& reader, int field, int last)
{
    while (field <= last && reader.isBlank(field))
        ++field;
    return field;
}

// One side of a connection: a point, or "first THRU last". `field` is left after what was read.
IdRun readSide(FieldReader& reader, int& field, int last, const char* name)
{
    field = skipBlanks(reader, field, last);
    if (field > last)
    {
        reader.error("card.missing-field", "a connection needs the point of each superelement", name);
        return {};
    }
    IdRun run;
    run.first = run.last = reader.requiredInteger(field, name);
    ++field;
    const int thru = skipBlanks(reader, field, last);
    if (thru <= last && (reader.text(thru) == "Through"))
    {
        const int end = skipBlanks(reader, thru + 1, last);
        if (end > last)
        {
            reader.error("card.missing-field", "THRU needs a last point", name);
            field = end;
            return run;
        }
        run.last = reader.requiredInteger(end, name);
        field = end + 1;
    }
    return run;
}

// Writes one side from `field`: a point is one field, a range three.
void writeSide(FieldWriter& writer, int& field, const IdRun& run)
{
    writer.integer(field++, run.first);
    if (run.last != run.first)
    {
        writer.text(field++, "Through");
        writer.integer(field++, run.last);
    }
}

// True when the field is the first of a line of the card, where a THRU is not accepted.
bool startsLine(int field) noexcept
{
    return (field - 1) % FIELDS_PER_LINE == 0;
}

} // namespace

std::optional<SuperelementConnectionCard> SuperelementConnectionFormat::read(FieldReader& reader)
{
    SuperelementConnectionCard card;
    card.superelementIdA = reader.requiredInteger(1, "superelementIdA");
    card.superelementIdB = reader.requiredInteger(2, "superelementIdB");
    card.tolerance = reader.real(3, "tolerance", 0.0);
    card.location = readKeyword<SuperelementBulkOptionsLocation>(
        reader, 4, "location", SUPERELEMENT_BULK_LOCATION_WORDS, SuperelementBulkOptionsLocation::Yes);
    const int last = reader.lastUsedField();
    int field = FIRST_PAIR_FIELD;
    while (!reader.failed() && skipBlanks(reader, field, last) <= last)
    {
        SuperelementConnectionPair pair;
        pair.a = readSide(reader, field, last, "nodeIdA");
        pair.b = readSide(reader, field, last, "nodeIdB");
        card.pairs.push_back(pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementConnectionFormat::check(const SuperelementConnectionCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementIdA, "superelementIdA", true))
        return problem;
    if (const auto problem = checkSuperelementId(card.superelementIdB, "superelementIdB", true))
        return problem;
    if (const auto problem = checkReal(card.tolerance, "tolerance", RealRule::NonNegative))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one connection " + std::string(CardProblem::FIELD) + " GIDB1",
                           "nodeIdA1"};
    for (const SuperelementConnectionPair& pair : card.pairs)
    {
        for (const IdRun& side : {pair.a, pair.b})
        {
            if (side.first <= 0 || side.last <= 0)
                return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer", "nodeIdA"};
            if (side.last < side.first)
                return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "nodeIdA"};
        }
        if (pair.a.last - pair.a.first != pair.b.last - pair.b.first)
            return CardProblem{"card.bad-value", "the two THRU ranges of a connection must have as many points",
                               "nodeIdB"};
    }
    return std::nullopt;
}

void SuperelementConnectionFormat::write(const SuperelementConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementIdA);
    writer.integer(2, card.superelementIdB);
    writer.realUnlessDefault(3, card.tolerance, 0.0);
    writeKeywordUnlessDefault(writer, 4, SUPERELEMENT_BULK_LOCATION_WORDS, card.location,
                              SuperelementBulkOptionsLocation::Yes);
    int field = FIRST_PAIR_FIELD_WRITTEN;
    for (const SuperelementConnectionPair& pair : card.pairs)
    {
        // A THRU is never the first field of a line: a range that would put one there starts a field later.
        if (pair.a.last != pair.a.first)
        {
            while (startsLine(field + 1) || startsLine(field + 4))
                ++field;
        }
        writeSide(writer, field, pair.a);
        writeSide(writer, field, pair.b);
    }
}

std::uint32_t SuperelementConnectionTable::append(const SuperelementConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementIdA.push(card.superelementIdA);
    m_superelementIdB.push(card.superelementIdB);
    m_cards.push(card);
    return row;
}

void SuperelementConnectionTable::store(std::uint32_t row, const SuperelementConnectionCard& card)
{
    m_superelementIdA.set(row, card.superelementIdA);
    m_superelementIdB.set(row, card.superelementIdB);
    m_cards.set(row, card);
}

void SuperelementConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementIdA.keepRows(rows);
    m_superelementIdB.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementConnectionCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementIdA", card.superelementIdA);
    visitSuperelement(visitor, "superelementIdB", card.superelementIdB);
    m_superelementIdA.set(row, card.superelementIdA);
    m_superelementIdB.set(row, card.superelementIdB);
    SuperelementScope sideA(visitor, card.superelementIdA);
    SuperelementScope sideB(visitor, card.superelementIdB);
    for (SuperelementConnectionPair& pair : card.pairs)
    {
        visitRun(sideA, "nodeIdA", pair.a, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
        visitRun(sideB, "nodeIdB", pair.b, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
    }
}

} // namespace oor::model
