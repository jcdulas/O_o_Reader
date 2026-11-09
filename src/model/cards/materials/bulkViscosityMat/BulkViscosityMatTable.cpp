#include "oor/model/cards/materials/bulkViscosityMat/BulkViscosityMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <array>
#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 2> BULK_VISCOSITY_KIND_NAMES{"ExternalExplicitProgram",
                                                                    "ExternalExplicitSolver"};

} // namespace

std::optional<BulkViscosityMatCard> BulkViscosityMatFormat::read(FieldReader& reader)
{
    BulkViscosityMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
    {
        if ((word == BULK_VISCOSITY_KIND_NAMES[0]))
            card.bulkViscosityKind = BulkViscosityMatType::Standard;
        else if ((word == BULK_VISCOSITY_KIND_NAMES[1]))
            card.bulkViscosityKind = BulkViscosityMatType::Enhanced;
        else
            reader.error("card.bad-value",
                         std::string("bulkViscosityKind") + " must name the standard or the enhanced model, not \"" +
                             std::string(word) + "\"",
                         "bulkViscosityKind");
    }
    card.quadraticBulkViscosity = reader.real(3, "quadraticBulkViscosity", 0.0);
    card.linearBulkViscosity = reader.real(4, "linearBulkViscosity", 1.0);
    card.compressionScale = reader.real(5, "compressionScale", 1.0);
    card.membraneDamping = reader.real(6, "membraneDamping", 0.0);
    card.initialMetricRelaxation =
        reader.real(7, "initialMetricRelaxation", BulkViscosityMatCard::DEFAULT_INITIAL_METRIC_RELAXATION);
    requireBlankFields(reader, {}, 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BulkViscosityMatFormat::check(const BulkViscosityMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.quadraticBulkViscosity >= 0.0) || !std::isfinite(card.quadraticBulkViscosity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "quadraticBulkViscosity"};
    if (!(card.linearBulkViscosity >= 0.0) || !std::isfinite(card.linearBulkViscosity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "linearBulkViscosity"};
    if (!(card.compressionScale >= 0.0) || !std::isfinite(card.compressionScale))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "compressionScale"};
    if (!(card.membraneDamping >= 0.0) || !std::isfinite(card.membraneDamping))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "membraneDamping"};
    if (!(card.initialMetricRelaxation > 0.0 && card.initialMetricRelaxation < 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 and less than 1.0",
                           "initialMetricRelaxation"};
    return std::nullopt;
}

void BulkViscosityMatFormat::write(const BulkViscosityMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.bulkViscosityKind == BulkViscosityMatType::Enhanced)
    {
        writer.text(2, BULK_VISCOSITY_KIND_NAMES[1]);
    }
    writer.realUnlessDefault(3, card.quadraticBulkViscosity, 0.0);
    writer.realUnlessDefault(4, card.linearBulkViscosity, 1.0);
    writer.realUnlessDefault(5, card.compressionScale, 1.0);
    writer.realUnlessDefault(6, card.membraneDamping, 0.0);
    writer.realUnlessDefault(7, card.initialMetricRelaxation, BulkViscosityMatCard::DEFAULT_INITIAL_METRIC_RELAXATION);
}

std::uint32_t BulkViscosityMatTable::append(const BulkViscosityMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bulkViscosityKind.push(static_cast<std::uint8_t>(card.bulkViscosityKind));
    m_quadraticBulkViscosity.push(card.quadraticBulkViscosity);
    m_linearBulkViscosity.push(card.linearBulkViscosity);
    m_compressionScale.push(card.compressionScale);
    m_membraneDamping.push(card.membraneDamping);
    m_initialMetricRelaxation.push(card.initialMetricRelaxation);
    return row;
}

BulkViscosityMatCard BulkViscosityMatTable::card(std::uint32_t row) const
{
    BulkViscosityMatCard card;
    card.id = m_id.get(row);
    card.bulkViscosityKind = static_cast<BulkViscosityMatType>(m_bulkViscosityKind.get(row));
    card.quadraticBulkViscosity = m_quadraticBulkViscosity.get(row);
    card.linearBulkViscosity = m_linearBulkViscosity.get(row);
    card.compressionScale = m_compressionScale.get(row);
    card.membraneDamping = m_membraneDamping.get(row);
    card.initialMetricRelaxation = m_initialMetricRelaxation.get(row);
    return card;
}

void BulkViscosityMatTable::store(std::uint32_t row, const BulkViscosityMatCard& card)
{
    m_id.set(row, card.id);
    m_bulkViscosityKind.set(row, static_cast<std::uint8_t>(card.bulkViscosityKind));
    m_quadraticBulkViscosity.set(row, card.quadraticBulkViscosity);
    m_linearBulkViscosity.set(row, card.linearBulkViscosity);
    m_compressionScale.set(row, card.compressionScale);
    m_membraneDamping.set(row, card.membraneDamping);
    m_initialMetricRelaxation.set(row, card.initialMetricRelaxation);
}

void BulkViscosityMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bulkViscosityKind.keepRows(rows);
    m_quadraticBulkViscosity.keepRows(rows);
    m_linearBulkViscosity.keepRows(rows);
    m_compressionScale.keepRows(rows);
    m_membraneDamping.keepRows(rows);
    m_initialMetricRelaxation.keepRows(rows);
}

void BulkViscosityMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
}

} // namespace oor::model
