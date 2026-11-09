#include "oor/model/cards/nonlinear_contact/contactBeamRadius/ContactBeamRadiusTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The ranges go up (ID1 THRU ID2 BY N with N > 0).
constexpr ContactRunRules RULES{true, false};

} // namespace

std::optional<ContactBeamRadiusCard> ContactBeamRadiusFormat::read(FieldReader& reader)
{
    ContactBeamRadiusCard card;
    card.radius = reader.requiredReal(1, "radius");
    card.idKind = readKeyword(reader, 2, "idKind", {"ElemId", "Body", "All"}, "ElemId");
    card.ids = readContactRuns(reader, 3, reader.lastUsedField(), "ids", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBeamRadiusFormat::check(const ContactBeamRadiusCard& card)
{
    if (!(card.radius > 0.0) || !std::isfinite(card.radius))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "radius"};
    if (auto problem = checkKeyword(card.idKind, "idKind", {"ElemId", "Body", "All"}))
        return problem;
    // with ALL every ID is blank.
    if (card.idKind == "All")
    {
        if (!card.ids.empty())
            return CardProblem{"card.bad-value",
                               "the IDs must be blank with " + CardProblem::fieldNamed("idKind") + " ALL", "ids"};
        return std::nullopt;
    }
    if (card.ids.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one ID, or " + CardProblem::fieldNamed("idKind") + " ALL", "ids"};
    return checkContactRuns(card.ids, "ids", RULES);
}

void ContactBeamRadiusFormat::write(const ContactBeamRadiusCard& card, FieldWriter& writer)
{
    writer.real(1, card.radius);
    writer.textUnlessDefault(2, card.idKind, "ElemId");
    // An ID list that follows a blank TYPE is read as elements: the default is not written, but the IDs stay in place.
    writeContactRuns(writer, 3, card.ids);
}

std::uint32_t ContactBeamRadiusTable::append(const ContactBeamRadiusCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_radius.push(card.radius);
    m_idKind.push(card.idKind);
    m_runs.push(toContactTriples(card.ids));
    return row;
}

ContactBeamRadiusCard ContactBeamRadiusTable::card(std::uint32_t row) const
{
    ContactBeamRadiusCard card;
    card.radius = m_radius.get(row);
    card.idKind = m_idKind.get(row);
    card.ids = toContactRuns(m_runs.get(row));
    return card;
}

void ContactBeamRadiusTable::store(std::uint32_t row, const ContactBeamRadiusCard& card)
{
    m_radius.set(row, card.radius);
    m_idKind.set(row, card.idKind);
    m_runs.set(row, toContactTriples(card.ids));
}

void ContactBeamRadiusTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_radius.keepRows(rows);
    m_idKind.keepRows(rows);
    m_runs.keepRows(rows);
}

void ContactBeamRadiusTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const std::string idKind = m_idKind.get(row);
    if (idKind == "Body")
        visitContactTriples(m_runs, row, visitor, "ids", ReferenceTarget::ContactBody, ReferenceTarget::ContactBody);
    else if (idKind == "ElemId")
        visitContactTriples(m_runs, row, visitor, "ids", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
}

} // namespace oor::model
