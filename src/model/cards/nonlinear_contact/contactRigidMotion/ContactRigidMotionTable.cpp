#include "oor/model/cards/nonlinear_contact/contactRigidMotion/ContactRigidMotionTable.hpp"

namespace oor::model
{

std::optional<ContactRigidMotionCard> ContactRigidMotionFormat::read(FieldReader& reader)
{
    ContactRigidMotionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceNode = reader.integer(2, "referenceNode", 0);
    card.control = reader.integer(3, "control", 0);
    const std::vector<ContactSection> sections = splitContactSections(reader, {APPROV, GROW});
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            // The head: nothing after CONTROL on the first line, and at most the one line of motion after it.
            for (int field = 4; field <= CONTACT_LINE_FIELDS; ++field)
            {
                if (!reader.isBlank(field))
                    reader.error("card.bad-value",
                                 "a " + std::string(reader.cardName()) + " has " + std::string("id") + ", " +
                                     std::string("referenceNode") + " and " + std::string("control") +
                                     " on its first line",
                                 "control");
            }
            if (section.last > 2 * CONTACT_LINE_FIELDS)
                reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has one line of motion",
                             "loadNode");
            if (section.last >= 2 * CONTACT_LINE_FIELDS)
                card.motion = readContactMotion(reader, firstFieldOfLine(1));
        }
        else if (section.keyword == APPROV)
            card.approach = readContactApproach(reader, section);
        else if (section.keyword == GROW)
            card.growth = readContactGrowth(reader, section);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactRigidMotionFormat::check(const ContactRigidMotionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.referenceNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a grid point ID, or blank",
                           "referenceNode"};
    if (card.control < -1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is -1, 0 or a grid point number",
                           "control"};
    if (auto problem = checkContactMotion(card.motion))
        return problem;
    if (card.approach)
    {
        if (auto problem = checkContactApproach(*card.approach))
            return problem;
    }
    if (card.growth)
    {
        if (auto problem = checkContactGrowth(*card.growth))
            return problem;
    }
    return std::nullopt;
}

void ContactRigidMotionFormat::write(const ContactRigidMotionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.referenceNode, 0);
    writer.integerUnlessDefault(3, card.control, 0);
    int line = writeContactMotion(writer, firstFieldOfLine(1), card.motion);
    if (card.approach)
        line = writeContactApproach(writer, line, *card.approach);
    if (card.growth)
        writeContactGrowth(writer, line, *card.growth);
}

std::uint32_t ContactRigidMotionTable::append(const ContactRigidMotionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactRigidMotionTable::store(std::uint32_t row, const ContactRigidMotionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactRigidMotionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactRigidMotionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactRigidMotionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactRigidMotionCard& card = m_cards.edit(row);
    visitContactId(card.referenceNode, visitor, "referenceNode", ReferenceTarget::Node);
    if (card.control > 0)
        visitContactId(card.control, visitor, "control", ReferenceTarget::Node);
    visitContactId(card.motion.loadNode, visitor, "loadNode", ReferenceTarget::Node);
}

} // namespace oor::model
