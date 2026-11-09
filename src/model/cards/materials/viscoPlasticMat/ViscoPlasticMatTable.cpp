#include "oor/model/cards/materials/viscoPlasticMat/ViscoPlasticMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_ANAND_FIELD = 3; // PREXF; the nine constants are in fields 3 to 11

constexpr const char* ANAND_NAMES[ViscoPlasticMatCard::ANAND_CONSTANTS] = {
    "preExponentialFactor",  "activationEnergy",      "stressMultiplier",
    "strainRateSensitivity", "saturationCoefficient", "saturationRateSensitivity",
    "hardeningConstant",     "hardeningSensitivity",  "initialDeformationResistance"};

} // namespace

std::optional<ViscoPlasticMatCard> ViscoPlasticMatFormat::read(FieldReader& reader)
{
    ViscoPlasticMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if ((reader.text(2) == "AnandModel"))
    {
        card.model = ViscoPlasticMatModel::Anand;
        for (std::size_t index = 0; index < card.anand.size(); ++index)
            card.anand[index] = reader.requiredReal(FIRST_ANAND_FIELD + static_cast<int>(index), ANAND_NAMES[index]);
        requireBlankFields(reader, {}, FIRST_ANAND_FIELD + static_cast<int>(ViscoPlasticMatCard::ANAND_CONSTANTS) - 1);
    }
    else
    {
        // a common deck leaves A blank for the user subroutine, which is the same as A = 0.0.
        card.rateCoefficient = reader.real(2, "rateCoefficient", 0.0);
        card.rateTableId = reader.integer(3, "rateTableId", 0);
        card.stressExponent = reader.real(4, "stressExponent", 0.0);
        card.strainExponent = reader.real(5, "strainExponent", 0.0);
        card.temperatureExponent = reader.real(6, "temperatureExponent", 0.0);
        card.timeExponent = reader.real(7, "timeExponent", 0.0);
        card.userSubroutineFlag = reader.integer(8, "userSubroutineFlag", 0);
        requireBlankFields(reader, {}, 8);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViscoPlasticMatFormat::check(const ViscoPlasticMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.model == ViscoPlasticMatModel::Anand)
    {
        for (std::size_t index = 0; index < card.anand.size(); ++index)
        {
            if (!std::isfinite(card.anand[index]))
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                                   ANAND_NAMES[index]};
        }
        return std::nullopt;
    }
    if (!std::isfinite(card.rateCoefficient) || !std::isfinite(card.stressExponent) ||
        !std::isfinite(card.strainExponent) || !std::isfinite(card.temperatureExponent) ||
        !std::isfinite(card.timeExponent))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("stressExponent") + ", " +
                               CardProblem::fieldNamed("strainExponent") + ", " +
                               CardProblem::fieldNamed("temperatureExponent") + " and " +
                               CardProblem::fieldNamed("timeExponent") + " must be finite numbers",
                           "rateCoefficient"};
    if (card.rateTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a TABL3Di, or blank",
                           "rateTableId"};
    if (card.userSubroutineFlag != 0 && card.userSubroutineFlag != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "userSubroutineFlag"};
    return std::nullopt;
}

void ViscoPlasticMatFormat::write(const ViscoPlasticMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.model == ViscoPlasticMatModel::Anand)
    {
        writer.text(2, "AnandModel");
        for (std::size_t index = 0; index < card.anand.size(); ++index)
            writer.real(FIRST_ANAND_FIELD + static_cast<int>(index), card.anand[index]);
        return;
    }
    writer.real(2, card.rateCoefficient);
    writer.integerUnlessDefault(3, card.rateTableId, 0);
    writer.realUnlessDefault(4, card.stressExponent, 0.0);
    writer.realUnlessDefault(5, card.strainExponent, 0.0);
    writer.realUnlessDefault(6, card.temperatureExponent, 0.0);
    writer.realUnlessDefault(7, card.timeExponent, 0.0);
    writer.integerUnlessDefault(8, card.userSubroutineFlag, 0);
}

std::uint32_t ViscoPlasticMatTable::append(const ViscoPlasticMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ViscoPlasticMatTable::store(std::uint32_t row, const ViscoPlasticMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ViscoPlasticMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ViscoPlasticMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ViscoPlasticMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card extends; IT3D names a TABL3D0, TABL3D1 or TABL3D2 (0 when blank,
    // and not in use by the Anand model, whose card leaves it at 0).
    ViscoPlasticMatCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    if (card.model == ViscoPlasticMatModel::PowerLaw && card.rateTableId > 0)
        card.rateTableId = visitor.visit("rateTableId", ReferenceTarget::MultiDimensionalTable, card.rateTableId);
}

} // namespace oor::model
