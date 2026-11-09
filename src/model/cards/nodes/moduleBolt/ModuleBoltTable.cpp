#include "oor/model/cards/nodes/moduleBolt/ModuleBoltTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

namespace
{

constexpr std::string_view TOP = "Top";
constexpr std::string_view BOTTOM = "Bottom";

// The problem with a list of grid points, if there is one: every ID is a positive integer.
std::optional<CardProblem> checkGrids(const std::vector<base::EntityId>& grids, const char* field)
{
    for (const base::EntityId grid : grids)
    {
        if (grid <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", field};
    }
    return std::nullopt;
}

} // namespace

std::optional<ModuleBoltCard> ModuleBoltFormat::read(FieldReader& reader)
{
    ModuleBoltCard card;
    card.id = reader.requiredInteger(1, "id");
    card.controlNode = reader.requiredInteger(2, "controlNode");
    card.controlModuleId = reader.integer(3, "controlModuleId", 0);
    card.topModuleId = reader.integer(4, "topModuleId", 0);
    card.bottomModuleId = reader.integer(5, "bottomModuleId", 0);
    for (int field = 6; field <= CONTACT_LINE_FIELDS; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the first line of an " +
                             std::string(reader.cardName()) +
                             " is not used and must be "
                             "blank",
                         "");
    }
    // The lines after the first: TOP and BOTTOM start a list, and a line with no keyword goes on with the list above.
    const std::vector<ContactSection> sections = splitContactSections(reader, {TOP, BOTTOM});
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error("card.bad-value",
                             "a continuation line of an " + std::string(reader.cardName()) + " starts with " +
                                 std::string("topList") + " or " + std::string("bottomList"),
                             "topList");
            continue;
        }
        const bool isTop = section.keyword == TOP;
        std::vector<base::EntityId> ids =
            readContactIds(reader, section.first + 1, section.last, isTop ? "topNodes" : "bottomNodes");
        std::vector<base::EntityId>& list = isTop ? card.topNodes : card.bottomNodes;
        if (!list.empty())
            reader.error("card.bad-value", std::string(section.keyword) + " is given twice",
                         isTop ? "topList" : "bottomList");
        list = std::move(ids);
    }
    if (findContactSection(sections, TOP) == nullptr || findContactSection(sections, BOTTOM) == nullptr)
        reader.error("card.missing-field",
                     "an " + std::string(reader.cardName()) + " has a " + std::string("topList") + " and a " +
                         std::string("bottomList") + " list of grid points",
                     "topList");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleBoltFormat::check(const ModuleBoltCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.controlNode, "controlNode"))
        return problem;
    if (const auto problem = checkBoundaryModule(card.controlModuleId, "controlModuleId"))
        return problem;
    if (const auto problem = checkBoundaryModule(card.topModuleId, "topModuleId"))
        return problem;
    if (const auto problem = checkBoundaryModule(card.bottomModuleId, "bottomModuleId"))
        return problem;
    // MODC above 0 is the module of one of the two sides.
    if (card.controlModuleId > 0 && card.controlModuleId != card.topModuleId &&
        card.controlModuleId != card.bottomModuleId)
        return CardProblem{"card.bad-value",
                           "{field} above zero must be " + CardProblem::fieldNamed("topModuleId") + " or " +
                               CardProblem::fieldNamed("bottomModuleId"),
                           "controlModuleId"};
    if (card.topNodes.empty())
        return CardProblem{"card.missing-field", "the bolt needs grid points in {field}", "topList"};
    if (card.bottomNodes.empty())
        return CardProblem{"card.missing-field", "the bolt needs grid points in {field}", "bottomList"};
    // the same number of grid points on both sides, taken as pairs.
    if (card.topNodes.size() != card.bottomNodes.size())
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("topList") + " and {field} have the same number of grid points",
                           "bottomList"};
    if (const auto problem = checkGrids(card.topNodes, "topNodes"))
        return problem;
    if (const auto problem = checkGrids(card.bottomNodes, "bottomNodes"))
        return problem;
    // The control grid point is not in the BOTTOM list; with the modules, that is a question of the same module.
    if (card.controlModuleId == card.bottomModuleId &&
        std::find(card.bottomNodes.begin(), card.bottomNodes.end(), card.controlNode) != card.bottomNodes.end())
        return CardProblem{"card.bad-value",
                           "the control grid point " + CardProblem::fieldNamed("controlNode") + " is not in the " +
                               CardProblem::fieldNamed("bottomList") + " list",
                           "bottomNodes"};
    return std::nullopt;
}

void ModuleBoltFormat::write(const ModuleBoltCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.controlNode);
    writer.integerUnlessDefault(3, card.controlModuleId, 0);
    writer.integerUnlessDefault(4, card.topModuleId, 0);
    writer.integerUnlessDefault(5, card.bottomModuleId, 0);
    const int next = writeContactIdList(writer, firstFieldOfLine(1), TOP, card.topNodes);
    writeContactIdList(writer, next, BOTTOM, card.bottomNodes);
}

std::uint32_t ModuleBoltTable::append(const ModuleBoltCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleBoltTable::store(std::uint32_t row, const ModuleBoltCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleBoltTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleBoltTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleBoltTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleBoltCard& card = m_cards.edit(row);
    visitBoundaryId(visitor, "controlNode", ReferenceTarget::Node, card.controlModuleId, card.controlNode);
    for (base::EntityId& grid : card.topNodes)
        visitBoundaryId(visitor, "topNodes", ReferenceTarget::Node, card.topModuleId, grid);
    for (base::EntityId& grid : card.bottomNodes)
        visitBoundaryId(visitor, "bottomNodes", ReferenceTarget::Node, card.bottomModuleId, grid);
}

} // namespace oor::model
