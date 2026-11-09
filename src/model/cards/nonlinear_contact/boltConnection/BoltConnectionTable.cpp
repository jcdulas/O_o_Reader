#include "oor/model/cards/nonlinear_contact/boltConnection/BoltConnectionTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr std::string_view TOP = "Top";
constexpr std::string_view BOTTOM = "Bottom";

} // namespace

std::optional<BoltConnectionCard> BoltConnectionFormat::read(FieldReader& reader)
{
    BoltConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.controlNode = reader.requiredInteger(2, "controlNode");
    const std::vector<ContactSection> sections = splitContactSections(reader, {TOP, BOTTOM});
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error(
                    "card.bad-value",
                    "a continuation line of a " + std::string(reader.cardName()) + " starts with TOP or BOTTOM",
                    "topList");
            continue;
        }
        std::vector<base::EntityId> ids = readContactIds(reader, section.first + 1, section.last,
                                                         section.keyword == TOP ? "topNodes" : "bottomNodes");
        std::vector<base::EntityId>& list = section.keyword == TOP ? card.topNodes : card.bottomNodes;
        if (!list.empty())
            reader.error("card.bad-value", std::string(section.keyword) + " is given twice",
                         section.keyword == TOP ? "topList" : "bottomList");
        list = std::move(ids);
    }
    if (findContactSection(sections, TOP) == nullptr || findContactSection(sections, BOTTOM) == nullptr)
        reader.error("card.missing-field",
                     "a " + std::string(reader.cardName()) + " has a TOP and a BOTTOM list of grid points", "topList");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BoltConnectionFormat::check(const BoltConnectionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.controlNode <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                           "controlNode"};
    if (card.topNodes.empty() || card.bottomNodes.empty())
        return CardProblem{"card.missing-field", "the bolt needs grid points in TOP and in BOTTOM", "topList"};
    // the same number of grid points on both sides, taken as pairs.
    if (card.topNodes.size() != card.bottomNodes.size())
        return CardProblem{"card.bad-value", "TOP and BOTTOM have the same number of grid points", "bottomList"};
    for (const base::EntityId grid : card.topNodes)
    {
        if (grid <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "topNodes"};
    }
    for (const base::EntityId grid : card.bottomNodes)
    {
        if (grid <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "bottomNodes"};
        if (grid == card.controlNode)
            return CardProblem{
                "card.bad-value",
                "the control grid point " + CardProblem::fieldNamed("controlNode") + " is not in the BOTTOM list",
                "bottomList"};
    }
    return std::nullopt;
}

void BoltConnectionFormat::write(const BoltConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.controlNode);
    const int next = writeContactIdList(writer, firstFieldOfLine(1), TOP, card.topNodes);
    writeContactIdList(writer, next, BOTTOM, card.bottomNodes);
}

std::uint32_t BoltConnectionTable::append(const BoltConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_controlNode.push(card.controlNode);
    m_topNodes.push(card.topNodes);
    m_bottomNodes.push(card.bottomNodes);
    return row;
}

BoltConnectionCard BoltConnectionTable::card(std::uint32_t row) const
{
    BoltConnectionCard card;
    card.id = m_id.get(row);
    card.controlNode = m_controlNode.get(row);
    const auto top = m_topNodes.get(row);
    card.topNodes.assign(top.begin(), top.end());
    const auto bottom = m_bottomNodes.get(row);
    card.bottomNodes.assign(bottom.begin(), bottom.end());
    return card;
}

void BoltConnectionTable::store(std::uint32_t row, const BoltConnectionCard& card)
{
    m_id.set(row, card.id);
    m_controlNode.set(row, card.controlNode);
    m_topNodes.set(row, card.topNodes);
    m_bottomNodes.set(row, card.bottomNodes);
}

void BoltConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_topNodes.keepRows(rows);
    m_bottomNodes.keepRows(rows);
}

void BoltConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdAt(m_controlNode, row, visitor, "controlNode", ReferenceTarget::Node);
    visitContactIdColumn(m_topNodes, row, visitor, "topNodes", ReferenceTarget::Node);
    visitContactIdColumn(m_bottomNodes, row, visitor, "bottomNodes", ReferenceTarget::Node);
}

} // namespace oor::model
