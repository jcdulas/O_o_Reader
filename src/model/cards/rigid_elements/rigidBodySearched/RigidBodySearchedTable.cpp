#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string_view>

namespace oor::model
{

namespace
{

constexpr int TYPE_FIELD = 3;
constexpr int UNUSED_FIELD = 4;
constexpr int FIRST_COORDINATE_FIELD = 9;
constexpr int FIRST_LIST_FIELD = 12;

// Reads the two exclusion lists that start in the first list field and go on to the end of the card: GNi, the word
// ENDL, then GMj. Blank fields are skipped (the format reference allows them for readability and the lists run over
// continuation lines). A THRU follows a single ID and is followed by the last ID of the range; the format reference
// advises against a THRU in the first field of a line, which reads without trouble and is never written. Without an
// ENDL the whole list would be a GNi list with no end, which the format reference forbids.
void readExclusionLists(FieldReader& reader, RigidBodySearchedCard& card)
{
    std::vector<IdRun>* current = &card.independentNodes;
    const char* name = "independentNodes";
    bool endlSeen = false;
    bool afterRange = false; // the last run of the current list was closed by a THRU, which another THRU cannot follow
    const int last = reader.lastUsedField();
    for (int field = FIRST_LIST_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        const std::string_view text = reader.text(field);
        if ((text == "EndList"))
        {
            if (endlSeen)
            {
                reader.error("card.bad-value",
                             "the " + std::string("independentNodes") +
                                 " list is closed by the first ENDL: a second one is not allowed",
                             "listEndMarker");
                return;
            }
            endlSeen = true;
            current = &card.dependentNodes;
            name = "dependentNodes";
            afterRange = false;
            continue;
        }
        if ((text == "Through"))
        {
            if (current->empty() || afterRange)
            {
                reader.error("card.bad-value", "THRU must follow a single ID of the list", "through");
                return;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last ID", "through");
                return;
            }
            const base::EntityId end = reader.requiredInteger(next, name);
            if (reader.failed())
                return;
            if (end < current->back().first)
            {
                reader.error("card.bad-value", "the end of a THRU range must not be below its start", name);
                return;
            }
            current->back().last = end;
            afterRange = true;
            field = next;
            continue;
        }
        const base::EntityId id = reader.requiredInteger(field, name);
        current->push_back(IdRun{id, id});
        afterRange = false;
    }
    if (!reader.failed() && !endlSeen && !card.independentNodes.empty())
        reader.error("card.missing-field", "a " + std::string("independentNodes") + " list must end with ENDL",
                     "listEndMarker");
}

// Visits the IDs of an exclusion list in place: a single ID is a node, the ends of a range are nodes that may not
// exist.
void visitExclusionList(ReferenceVisitor& visitor, std::string_view field, std::vector<IdRun>& runs)
{
    for (IdRun& run : runs)
    {
        if (run.first == run.last)
        {
            run.first = visitor.visit(field, ReferenceTarget::Node, run.first);
            run.last = run.first;
            continue;
        }
        run.first = visitor.visit(field, ReferenceTarget::NodeInRange, run.first);
        run.last = visitor.visit(field, ReferenceTarget::NodeInRange, run.last);
    }
}

} // namespace

std::optional<RigidBodySearchedCard> RigidBodySearchedFormat::read(FieldReader& reader)
{
    RigidBodySearchedCard card;
    card.id = reader.requiredInteger(1, "id");
    card.spotNode = reader.integer(2, "spotNode", 0);
    card.type = readOptionalKeyword<RigidBodySearchedType>(reader, TYPE_FIELD, "kind", RIGID_BODY_SEARCHED_TYPE_WORDS);
    card.referenceTemperature = reader.real(5, "referenceTemperature", 0.0);
    card.radius = reader.requiredReal(6, "radius");
    card.dependentComponents =
        base::sortedDigits(reader.integer(7, "dependentComponents", RigidBodySearchedCard::DEFAULT_COMPONENTS));
    card.alphaCoefficient = reader.real(8, "alphaCoefficient", 0.0);
    card.xStation = reader.optionalReal(FIRST_COORDINATE_FIELD, "xStation");
    card.yStation = reader.optionalReal(FIRST_COORDINATE_FIELD + 1, "yStation");
    card.zStation = reader.optionalReal(FIRST_COORDINATE_FIELD + 2, "zStation");
    requireBlankFields(reader, {UNUSED_FIELD}, reader.slotCount());
    readExclusionLists(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidBodySearchedFormat::check(const RigidBodySearchedCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.spotNode, "spotNode"))
        return problem;
    if (const auto problem = checkReal(card.referenceTemperature, "referenceTemperature", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Finite))
        return problem;
    // The format reference takes a positive or a negative radius, never zero.
    if (card.radius == 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " must be a positive radius, or negative to make the two grid points coincident",
                           "radius"};
    if (card.dependentComponents <= 0 || !base::isComponentSet(card.dependentComponents))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "dependentComponents"};
    if (const auto problem = checkReal(card.alphaCoefficient, "alphaCoefficient", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.xStation, "xStation", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.yStation, "yStation", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.zStation, "zStation", RealRule::Finite))
        return problem;
    // The search location is GS or the coordinates, one of them.
    const bool coordinatesGiven = card.xStation || card.yStation || card.zStation;
    if (card.spotNode > 0 && coordinatesGiven)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " and the coordinates of the search location are both given: give it once",
                           "spotNode"};
    if (card.spotNode == 0 && !coordinatesGiven)
        return CardProblem{
            "card.missing-field",
            CardProblem::fieldNamed("spotNode") + " is blank and the coordinates give no search location", "xStation"};
    if (const auto problem = checkIdRuns(card.independentNodes, "independentNodes", IdSign::Positive, true))
        return problem;
    return checkIdRuns(card.dependentNodes, "dependentNodes", IdSign::Positive, true);
}

void RigidBodySearchedFormat::write(const RigidBodySearchedCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.spotNode, 0);
    writeOptionalKeyword(writer, TYPE_FIELD, RIGID_BODY_SEARCHED_TYPE_WORDS, card.type);
    writer.realUnlessDefault(5, card.referenceTemperature, 0.0);
    writer.real(6, card.radius);
    writer.integerUnlessDefault(7, card.dependentComponents, RigidBodySearchedCard::DEFAULT_COMPONENTS);
    writer.realUnlessDefault(8, card.alphaCoefficient, 0.0);
    if (card.xStation)
        writer.real(FIRST_COORDINATE_FIELD, *card.xStation);
    if (card.yStation)
        writer.real(FIRST_COORDINATE_FIELD + 1, *card.yStation);
    if (card.zStation)
        writer.real(FIRST_COORDINATE_FIELD + 2, *card.zStation);
    // The ENDL that closes GNi comes whenever there is a list at all: a GMj list alone starts with it.
    if (card.independentNodes.empty() && card.dependentNodes.empty())
        return;
    int field = writeIdRuns(writer, FIRST_LIST_FIELD, card.independentNodes, writer.compressesRanges());
    writer.text(field++, "EndList");
    writeIdRuns(writer, field, card.dependentNodes, writer.compressesRanges());
}

std::uint32_t RigidBodySearchedTable::append(const RigidBodySearchedCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_spotNode.push(card.spotNode);
    m_cards.push(card);
    return row;
}

void RigidBodySearchedTable::store(std::uint32_t row, const RigidBodySearchedCard& card)
{
    m_id.set(row, card.id);
    m_spotNode.set(row, card.spotNode);
    m_cards.set(row, card);
}

void RigidBodySearchedTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_spotNode.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidBodySearchedTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RigidBodySearchedTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RigidBodySearchedCard& card = m_cards.edit(row);
    visitId(visitor, "spotNode", ReferenceTarget::Node, card.spotNode);
    m_spotNode.set(row, card.spotNode);
    visitExclusionList(visitor, "independentNodes", card.independentNodes);
    visitExclusionList(visitor, "dependentNodes", card.dependentNodes);
}

} // namespace oor::model
