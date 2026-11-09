#include "oor/model/cards/explicit/eulerDefaultFlowBoundary/EulerDefaultFlowBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_PROPERTY_FIELD = 9; // TYPE1; each pair takes two fields
constexpr EulerFlowBoundaryPropertyRules RULES{false};

} // namespace

std::optional<EulerDefaultFlowBoundaryCard> EulerDefaultFlowBoundaryFormat::read(FieldReader& reader)
{
    EulerDefaultFlowBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.solverType = readKeyword(reader, 3, "solverType", EULER_SOLVER_WORDS, EulerSolverType::Hydro);
    const int last = reader.lastUsedField();
    std::size_t index = 0;
    for (int field = FIRST_PROPERTY_FIELD; field <= last; field += 2, ++index)
    {
        if (const std::optional<EulerFlowBoundaryProperty> property = readFlowProperty(reader, field, index, RULES))
            card.properties.push_back(*property);
    }
    requireBlankFields(reader, {2, 4, 5, 6, 7, 8}, last);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerDefaultFlowBoundaryFormat::check(const EulerDefaultFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.solverType == EulerSolverType::FirstOrder || card.solverType == EulerSolverType::SecondOrder)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be HYDRO, STRENGTH, MMHYDRO or MMSTREN",
                           "solverType"};
    for (std::size_t index = 0; index < card.properties.size(); ++index)
    {
        if (const auto problem = checkFlowProperty(card.properties[index], index, RULES))
            return problem;
    }
    return std::nullopt;
}

void EulerDefaultFlowBoundaryFormat::write(const EulerDefaultFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 3, EULER_SOLVER_WORDS, card.solverType, EulerSolverType::Hydro);
    for (std::size_t index = 0; index < card.properties.size(); ++index)
        writeFlowProperty(writer, FIRST_PROPERTY_FIELD + 2 * static_cast<int>(index), card.properties[index]);
}

std::uint32_t EulerDefaultFlowBoundaryTable::append(const EulerDefaultFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerDefaultFlowBoundaryTable::store(std::uint32_t row, const EulerDefaultFlowBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerDefaultFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerDefaultFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerDefaultFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerDefaultFlowBoundaryCard& card = m_cards.edit(row);
    for (std::size_t index = 0; index < card.properties.size(); ++index)
        visitFlowProperty(visitor, card.properties[index], index);
}

} // namespace oor::model
