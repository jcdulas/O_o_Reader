#include "oor/model/cards/properties/springOffsetProp/SpringOffsetPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// The values are on the third line of the grid: the second one is drawn empty.
constexpr int STIFFNESS_FIELD = 17;
constexpr int TENSILE_FAILURE_FORCE_FIELD = 18;
constexpr int COMPRESSIVE_FAILURE_FORCE_FIELD = 19;
constexpr int LOAD_FIELD = 20;
constexpr int UNLOAD_FIELD = 21;
constexpr int LAST_FIELD = UNLOAD_FIELD;

} // namespace

std::optional<SpringOffsetPropCard> SpringOffsetPropFormat::read(FieldReader& reader)
{
    SpringOffsetPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stiffness = reader.optionalReal(STIFFNESS_FIELD, "stiffness");
    card.tensileFailureForce = reader.optionalReal(TENSILE_FAILURE_FORCE_FIELD, "tensileFailureForce");
    card.compressiveFailureForce = reader.optionalReal(COMPRESSIVE_FAILURE_FORCE_FIELD, "compressiveFailureForce");
    card.loadingTableId = reader.integer(LOAD_FIELD, "loadingTableId", 0);
    card.unloadingTableId = reader.integer(UNLOAD_FIELD, "unloadingTableId", 0);
    // Fields 2 to 16 are the cells of the grid that the format leaves empty.
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SpringOffsetPropFormat::check(const SpringOffsetPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.loadingTableId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the positive ID of a " +
                               CardProblem::cardNamed("TableDynamic1"),
                           "loadingTableId"};
    if (card.unloadingTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the positive ID of a table",
                           "unloadingTableId"};
    if (card.isLinear())
    {
        // With K, LOAD and UNLOAD are not allowed.
        if (card.loadingTableId != 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is not allowed when " +
                                   CardProblem::fieldNamed("stiffness") + " is given",
                               "loadingTableId"};
        if (card.unloadingTableId != 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is not allowed when " +
                                   CardProblem::fieldNamed("stiffness") + " is given",
                               "unloadingTableId"};
        if (const auto problem = checkReal(*card.stiffness, "stiffness", RealRule::Positive))
            return problem;
        if (const auto problem = checkOptionalReal(card.tensileFailureForce, "tensileFailureForce", RealRule::Positive))
            return problem;
        return checkOptionalReal(card.compressiveFailureForce, "compressiveFailureForce", RealRule::Positive);
    }
    // With LOAD, K, FAILMTF and FAILMCF are not allowed, and the failure forces need K besides.
    if (card.loadingTableId == 0)
        return CardProblem{"card.missing-field",
                           "the card needs " + CardProblem::fieldNamed("stiffness") + " (a linear spring) or " +
                               CardProblem::fieldNamed("loadingTableId") + " (a nonlinear one)",
                           "stiffness"};
    if (card.tensileFailureForce)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is not allowed when " +
                               CardProblem::fieldNamed("stiffness") + " is not given",
                           "tensileFailureForce"};
    if (card.compressiveFailureForce)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is not allowed when " +
                               CardProblem::fieldNamed("stiffness") + " is not given",
                           "compressiveFailureForce"};
    return std::nullopt;
}

void SpringOffsetPropFormat::write(const SpringOffsetPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.stiffness)
    {
        writer.real(STIFFNESS_FIELD, *card.stiffness);
        if (card.tensileFailureForce)
            writer.real(TENSILE_FAILURE_FORCE_FIELD, *card.tensileFailureForce);
        if (card.compressiveFailureForce)
            writer.real(COMPRESSIVE_FAILURE_FORCE_FIELD, *card.compressiveFailureForce);
        return;
    }
    writer.integer(LOAD_FIELD, card.loadingTableId);
    if (card.unloadingTableId != 0)
        writer.integer(UNLOAD_FIELD, card.unloadingTableId);
}

std::uint32_t SpringOffsetPropTable::append(const SpringOffsetPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(realToColumn(card.stiffness));
    m_tensileFailureForce.push(realToColumn(card.tensileFailureForce));
    m_compressiveFailureForce.push(realToColumn(card.compressiveFailureForce));
    m_loadingTableId.push(card.loadingTableId);
    m_unloadingTableId.push(card.unloadingTableId);
    return row;
}

SpringOffsetPropCard SpringOffsetPropTable::card(std::uint32_t row) const
{
    SpringOffsetPropCard card;
    card.id = m_id.get(row);
    card.stiffness = realFromColumn(m_stiffness.get(row));
    card.tensileFailureForce = realFromColumn(m_tensileFailureForce.get(row));
    card.compressiveFailureForce = realFromColumn(m_compressiveFailureForce.get(row));
    card.loadingTableId = m_loadingTableId.get(row);
    card.unloadingTableId = m_unloadingTableId.get(row);
    return card;
}

void SpringOffsetPropTable::store(std::uint32_t row, const SpringOffsetPropCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, realToColumn(card.stiffness));
    m_tensileFailureForce.set(row, realToColumn(card.tensileFailureForce));
    m_compressiveFailureForce.set(row, realToColumn(card.compressiveFailureForce));
    m_loadingTableId.set(row, card.loadingTableId);
    m_unloadingTableId.set(row, card.unloadingTableId);
}

void SpringOffsetPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_tensileFailureForce.keepRows(rows);
    m_compressiveFailureForce.keepRows(rows);
    m_loadingTableId.keepRows(rows);
    m_unloadingTableId.keepRows(rows);
}

void SpringOffsetPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // Both curves are force against displacement tables of the dynamic table space (see the comment of the card for
    // the TABLExx of UNLOAD).
    if (const base::EntityId loadingTableId = m_loadingTableId.get(row); loadingTableId > 0)
        m_loadingTableId.set(row, visitor.visit("loadingTableId", ReferenceTarget::DynamicTable, loadingTableId));
    if (const base::EntityId unloadingTableId = m_unloadingTableId.get(row); unloadingTableId > 0)
        m_unloadingTableId.set(row, visitor.visit("unloadingTableId", ReferenceTarget::DynamicTable, unloadingTableId));
}

} // namespace oor::model
