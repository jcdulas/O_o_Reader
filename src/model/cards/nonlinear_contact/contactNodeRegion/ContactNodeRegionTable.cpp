#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{true, true};
constexpr int FIRST_GRID_FIELD_101 = 4;
constexpr int FIRST_GRID_FIELD_700 = 2;

} // namespace

std::optional<ContactNodeRegionCard> ContactNodeRegionFormat::read(FieldReader& reader)
{
    ContactNodeRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    // Field 3 is DIM in the first layout (blank, 2D or 3D) and a grid point in the SOL 700 one, or the THRU of a range
    // that starts at the first grid point (format reference BCGRID, SOL 700: THRU makes a range; BCGRID 21 1001
    // THRU 1010).
    card.sol700 = !reader.isBlank(3) &&
                  (base::NumberParser::parseInteger(reader.text(3)).has_value() || (reader.text(3) == "Through"));
    if (card.sol700)
    {
        card.grids = readContactRuns(reader, FIRST_GRID_FIELD_700, reader.lastUsedField(), "nodeId", RULES);
    }
    else
    {
        card.bodyParamsId = reader.integer(2, "bodyParamsId", 0);
        card.dimension =
            readKeyword(reader, 3, "dimension", {"TwoDimensional", "ThreeDimensional"}, "ThreeDimensional");
        card.grids = readContactRuns(reader, FIRST_GRID_FIELD_101, reader.lastUsedField(), "nodeId", RULES);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactNodeRegionFormat::check(const ContactNodeRegionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.bodyParamsId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of body parameters, or blank",
                           "bodyParamsId"};
    if (auto problem = checkKeyword(card.dimension, "dimension", {"TwoDimensional", "ThreeDimensional"}))
        return problem;
    if (card.sol700 && (card.bodyParamsId != 0 || card.dimension != "ThreeDimensional"))
        return CardProblem{"card.bad-value",
                           "the SOL 700 form has no " + std::string(CardProblem::FIELD) + " and no " +
                               CardProblem::fieldNamed("dimension"),
                           "bodyParamsId"};
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "nodeId"};
    return checkContactRuns(card.grids, "nodeId", RULES);
}

void ContactNodeRegionFormat::write(const ContactNodeRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.sol700)
    {
        writeContactRuns(writer, FIRST_GRID_FIELD_700, card.grids);
        return;
    }
    writer.integerUnlessDefault(2, card.bodyParamsId, 0);
    writer.textUnlessDefault(3, card.dimension, "ThreeDimensional");
    writeContactRuns(writer, FIRST_GRID_FIELD_101, card.grids);
}

std::uint32_t ContactNodeRegionTable::append(const ContactNodeRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bodyParamsId.push(card.bodyParamsId);
    m_dimension.push(card.dimension);
    m_sol700.push(card.sol700 ? 1 : 0);
    m_runs.push(toContactTriples(card.grids));
    return row;
}

ContactNodeRegionCard ContactNodeRegionTable::card(std::uint32_t row) const
{
    ContactNodeRegionCard card;
    card.id = m_id.get(row);
    card.bodyParamsId = m_bodyParamsId.get(row);
    card.dimension = m_dimension.get(row);
    card.sol700 = m_sol700.get(row) != 0;
    card.grids = toContactRuns(m_runs.get(row));
    return card;
}

void ContactNodeRegionTable::store(std::uint32_t row, const ContactNodeRegionCard& card)
{
    m_id.set(row, card.id);
    m_bodyParamsId.set(row, card.bodyParamsId);
    m_dimension.set(row, card.dimension);
    m_sol700.set(row, card.sol700 ? 1 : 0);
    m_runs.set(row, toContactTriples(card.grids));
}

void ContactNodeRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bodyParamsId.keepRows(rows);
    m_dimension.keepRows(rows);
    m_sol700.keepRows(rows);
    m_runs.keepRows(rows);
}

void ContactNodeRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdAt(m_bodyParamsId, row, visitor, "bodyParamsId", ReferenceTarget::ContactBodyParameters);
    visitContactTriples(m_runs, row, visitor, "nodeId", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
