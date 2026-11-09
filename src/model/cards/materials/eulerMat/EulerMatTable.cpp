#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <array>
#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELD_LINEAR_BULK_VISCOSITY = 9;
constexpr int FIELD_QUADRATIC_BULK_VISCOSITY = 10;
constexpr int FIELD_BULK_VISCOSITY_KIND = 13;

constexpr std::array<std::string_view, 2> BULK_VISCOSITY_KIND_NAMES{"ExternalExplicitProgram",
                                                                    "ExternalExplicitSolver"};

} // namespace

std::optional<EulerMatCard> EulerMatFormat::read(FieldReader& reader)
{
    EulerMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.density = reader.requiredReal(2, "density");
    card.equationOfStateId = reader.requiredInteger(3, "equationOfStateId");
    card.shearModelId = reader.integer(4, "shearModelId", 0);
    card.yieldModelId = reader.integer(5, "yieldModelId", 0);
    card.failureModelId = reader.integer(6, "failureModelId", 0);
    card.spallModelId = reader.integer(7, "spallModelId", 0);
    card.linearBulkViscosity = reader.real(FIELD_LINEAR_BULK_VISCOSITY, "linearBulkViscosity", 0.0);
    card.quadraticBulkViscosity = reader.real(FIELD_QUADRATIC_BULK_VISCOSITY, "quadraticBulkViscosity", 1.0);
    if (const std::string_view word = reader.text(FIELD_BULK_VISCOSITY_KIND); !word.empty())
    {
        if ((word == BULK_VISCOSITY_KIND_NAMES[0]))
            card.bulkViscosityKind = EulerMatBulkType::Standard;
        else if ((word == BULK_VISCOSITY_KIND_NAMES[1]))
            card.bulkViscosityKind = EulerMatBulkType::Enhanced;
        else
            reader.error("card.bad-value",
                         std::string("bulkViscosityKind") + " must name the standard or the enhanced model, not \"" +
                             std::string(word) + "\"",
                         "bulkViscosityKind");
    }
    requireBlankFields(reader, {8, 11, 12}, FIELD_BULK_VISCOSITY_KIND);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerMatFormat::check(const EulerMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.density > 0.0) || !std::isfinite(card.density))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "density"};
    if (card.equationOfStateId <= 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be the ID of an equation of state, a positive integer",
            "equationOfStateId"};
    if (card.shearModelId < 0 || card.yieldModelId < 0 || card.failureModelId < 0 || card.spallModelId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("yieldModelId") + ", " +
                               CardProblem::fieldNamed("failureModelId") + " and " +
                               CardProblem::fieldNamed("spallModelId") + " must be positive IDs, or blank",
                           "shearModelId"};
    if (!(card.linearBulkViscosity >= 0.0) || !std::isfinite(card.linearBulkViscosity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "linearBulkViscosity"};
    if (!(card.quadraticBulkViscosity >= 0.0) || !std::isfinite(card.quadraticBulkViscosity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "quadraticBulkViscosity"};
    return std::nullopt;
}

void EulerMatFormat::write(const EulerMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.density);
    writer.integer(3, card.equationOfStateId);
    writer.integerUnlessDefault(4, card.shearModelId, 0);
    writer.integerUnlessDefault(5, card.yieldModelId, 0);
    writer.integerUnlessDefault(6, card.failureModelId, 0);
    writer.integerUnlessDefault(7, card.spallModelId, 0);
    writer.realUnlessDefault(FIELD_LINEAR_BULK_VISCOSITY, card.linearBulkViscosity, 0.0);
    writer.realUnlessDefault(FIELD_QUADRATIC_BULK_VISCOSITY, card.quadraticBulkViscosity, 1.0);
    if (card.bulkViscosityKind == EulerMatBulkType::Enhanced)
    {
        writer.text(FIELD_BULK_VISCOSITY_KIND, BULK_VISCOSITY_KIND_NAMES[1]);
    }
}

std::uint32_t EulerMatTable::append(const EulerMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerMatTable::store(std::uint32_t row, const EulerMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerMatCard& card = m_cards.edit(row);
    visitId(visitor, "equationOfStateId", ReferenceTarget::EquationOfState, card.equationOfStateId);
    visitId(visitor, "shearModelId", ReferenceTarget::ShearModel, card.shearModelId);
    visitId(visitor, "yieldModelId", ReferenceTarget::YieldModel, card.yieldModelId);
    visitId(visitor, "failureModelId", ReferenceTarget::FailureModel, card.failureModelId);
    visitId(visitor, "spallModelId", ReferenceTarget::SpallModel, card.spallModelId);
}

} // namespace oor::model
