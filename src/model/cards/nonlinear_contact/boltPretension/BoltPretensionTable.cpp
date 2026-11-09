#include "oor/model/cards/nonlinear_contact/boltPretension/BoltPretensionTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::string_view ELEM = "Elems";
constexpr std::string_view GRID = "Nodes";

// The names of the fields of the axis, N1 to N3.
constexpr std::array<const char*, 3> AXIS_NAMES{"directionX", "directionY", "directionZ"};

} // namespace

std::optional<BoltPretensionCard> BoltPretensionFormat::read(FieldReader& reader)
{
    BoltPretensionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.controlNode = reader.requiredInteger(2, "controlNode");
    card.form = reader.integer(3, "form", 0);
    for (std::size_t axis = 0; axis < 3; ++axis)
        card.axis[axis] = reader.real(4 + static_cast<int>(axis), AXIS_NAMES[axis], 0.0);
    card.offset = reader.real(7, "offset", 0.0);
    card.idType = readKeyword(reader, 8, "idType", {"List", "Set"}, "List");
    const std::vector<ContactSection> sections = splitContactSections(reader, {ELEM, GRID});
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error("card.bad-value",
                             "a continuation line of a " + std::string(reader.cardName()) + " starts with ELEM or GRID",
                             "elements");
            continue;
        }
        std::vector<base::EntityId> ids = readContactIds(reader, section.first + 1, section.last, "elements");
        (section.keyword == ELEM ? card.elements : card.nodes) = std::move(ids);
    }
    if (findContactSection(sections, ELEM) == nullptr)
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " has an ELEM list", "elements");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BoltPretensionFormat::check(const BoltPretensionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.controlNode <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                           "controlNode"};
    if (card.form != 0 && card.form != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "form"};
    if (auto problem = checkKeyword(card.idType, "idType", {"List", "Set"}))
        return problem;
    if (!std::isfinite(card.offset))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "offset"};
    const bool bySet = card.idType == "Set";
    for (const base::EntityId id : card.elements)
    {
        if (id <= 0)
            return CardProblem{"card.bad-id", "an element ID must be a positive integer", "elements"};
    }
    for (const base::EntityId id : card.nodes)
    {
        if (id <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "nodes"};
    }
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the bolt needs elements", "elements"};
    if (bySet && (card.elements.size() != 1 || card.nodes.size() > 1))
        return CardProblem{"card.bad-value",
                           "with " + CardProblem::fieldNamed("idType") + " SET the ELEM and GRID lists are one " +
                               std::string("TypedListSet3") + " ID each",
                           "elements"};
    if (card.form == 0)
    {
        // the axis comes from the elements, so there must be at least two (or one SET3 of them).
        if (!bySet && card.elements.size() < 2)
            return CardProblem{"card.bad-value",
                               "with " + CardProblem::fieldNamed("form") + " 0 the bolt needs at least two elements",
                               "elements"};
        return std::nullopt;
    }
    // the axis of FORM 1 is not of zero length; the cross-section is given by grid points.
    if (card.axis[0] == 0.0 && card.axis[1] == 0.0 && card.axis[2] == 0.0)
        return CardProblem{"card.bad-value",
                           "with " + CardProblem::fieldNamed("form") + " 1 the axis " +
                               CardProblem::fieldNamed("directionX") + ", " + CardProblem::fieldNamed("directionY") +
                               ", " + CardProblem::fieldNamed("directionZ") + " cannot be of zero length",
                           "directionX"};
    if (card.nodes.empty())
        return CardProblem{
            "card.missing-field",
            "with " + CardProblem::fieldNamed("form") + " 1 the bolt needs the GRID list of the cross-section",
            "nodes"};
    return std::nullopt;
}

void BoltPretensionFormat::write(const BoltPretensionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.controlNode);
    writer.integerUnlessDefault(3, card.form, 0);
    for (std::size_t axis = 0; axis < 3; ++axis)
        writer.realUnlessDefault(4 + static_cast<int>(axis), card.axis[axis], 0.0);
    writer.realUnlessDefault(7, card.offset, 0.0);
    writer.textUnlessDefault(8, card.idType, "List");
    int line = writeContactIdList(writer, firstFieldOfLine(1), ELEM, card.elements);
    if (!card.nodes.empty())
        writeContactIdList(writer, line, GRID, card.nodes);
}

std::uint32_t BoltPretensionTable::append(const BoltPretensionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BoltPretensionTable::store(std::uint32_t row, const BoltPretensionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BoltPretensionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BoltPretensionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BoltPretensionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    BoltPretensionCard& card = m_cards.edit(row);
    visitContactId(card.controlNode, visitor, "controlNode", ReferenceTarget::Node);
    const bool bySet = card.idType == "Set";
    visitContactIds(card.elements, visitor, "elements", bySet ? ReferenceTarget::Set : ReferenceTarget::Element);
    visitContactIds(card.nodes, visitor, "nodes", bySet ? ReferenceTarget::Set : ReferenceTarget::Node);
}

} // namespace oor::model
