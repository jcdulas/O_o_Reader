#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerPropCard> EulerPropFormat::read(FieldReader& reader)
{
    EulerPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.type = readKeyword(reader, 3, "solverType", EULER_SOLVER_WORDS, EulerSolverType::Hydro);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerPropFormat::check(const EulerPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    // the Riemann solvers do not handle void elements, and the analysis stops when they meet one.
    const bool riemann = card.type == EulerSolverType::FirstOrder || card.type == EulerSolverType::SecondOrder;
    if (card.matId == 0 && riemann)
        return CardProblem{"card.bad-value",
                           "a property of " + CardProblem::fieldNamed("solverType") +
                               " 1STORDER or 2NDORDER needs a material (" + std::string(CardProblem::FIELD) +
                               "): the Riemann solvers do not handle void elements",
                           "matId"};
    return std::nullopt;
}

void EulerPropFormat::write(const EulerPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writeKeywordUnlessDefault(writer, 3, EULER_SOLVER_WORDS, card.type, EulerSolverType::Hydro);
}

std::uint32_t EulerPropTable::append(const EulerPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerPropTable::store(std::uint32_t row, const EulerPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
