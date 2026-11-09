#include "oor/model/cards/thermal/forcedConvectionProp/ForcedConvectionPropTable.hpp"

#include <string>

namespace oor::model
{

std::optional<ForcedConvectionPropCard> ForcedConvectionPropFormat::read(FieldReader& reader)
{
    ForcedConvectionPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.matrixForm = reader.integer(3, "matrixForm", 0);
    card.massFlowConvection = reader.integer(4, "massFlowConvection", 0);
    card.coefficient = reader.requiredReal(5, "coefficient");
    card.reynoldsExponent = reader.real(6, "reynoldsExponent", 0.0);
    card.prandtlExponentInner = reader.real(7, "prandtlExponentInner", 0.0);
    card.prandtlExponentOuter = reader.real(8, "prandtlExponentOuter", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ForcedConvectionPropFormat::check(const ForcedConvectionPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the convection property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.matrixForm != 0 && card.matrixForm != 1 && card.matrixForm != 10 && card.matrixForm != 11 &&
        card.matrixForm != 20 && card.matrixForm != 21)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 10, 11, 20 or 21",
                           "matrixForm"};
    if (card.massFlowConvection != 0 && card.massFlowConvection != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "massFlowConvection"};
    if (!(card.coefficient > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "coefficient"};
    if (!(card.reynoldsExponent >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "reynoldsExponent"};
    if (!(card.prandtlExponentInner >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "prandtlExponentInner"};
    if (!(card.prandtlExponentOuter >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "prandtlExponentOuter"};
    return std::nullopt;
}

void ForcedConvectionPropFormat::write(const ForcedConvectionPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.matrixForm, 0);
    writer.integerUnlessDefault(4, card.massFlowConvection, 0);
    writer.real(5, card.coefficient);
    writer.realUnlessDefault(6, card.reynoldsExponent, 0.0);
    writer.realUnlessDefault(7, card.prandtlExponentInner, 0.0);
    writer.realUnlessDefault(8, card.prandtlExponentOuter, 0.0);
}

std::uint32_t ForcedConvectionPropTable::append(const ForcedConvectionPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_matrixForm.push(card.matrixForm);
    m_massFlowConvection.push(card.massFlowConvection);
    m_coefficient.push(card.coefficient);
    m_reynoldsExponent.push(card.reynoldsExponent);
    m_prandtlExponentInner.push(card.prandtlExponentInner);
    m_prandtlExponentOuter.push(card.prandtlExponentOuter);
    return row;
}

ForcedConvectionPropCard ForcedConvectionPropTable::card(std::uint32_t row) const
{
    ForcedConvectionPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.matrixForm = m_matrixForm.get(row);
    card.massFlowConvection = m_massFlowConvection.get(row);
    card.coefficient = m_coefficient.get(row);
    card.reynoldsExponent = m_reynoldsExponent.get(row);
    card.prandtlExponentInner = m_prandtlExponentInner.get(row);
    card.prandtlExponentOuter = m_prandtlExponentOuter.get(row);
    return card;
}

void ForcedConvectionPropTable::store(std::uint32_t row, const ForcedConvectionPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_matrixForm.set(row, card.matrixForm);
    m_massFlowConvection.set(row, card.massFlowConvection);
    m_coefficient.set(row, card.coefficient);
    m_reynoldsExponent.set(row, card.reynoldsExponent);
    m_prandtlExponentInner.set(row, card.prandtlExponentInner);
    m_prandtlExponentOuter.set(row, card.prandtlExponentOuter);
}

void ForcedConvectionPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_matrixForm.keepRows(rows);
    m_massFlowConvection.keepRows(rows);
    m_coefficient.keepRows(rows);
    m_reynoldsExponent.keepRows(rows);
    m_prandtlExponentInner.keepRows(rows);
    m_prandtlExponentOuter.keepRows(rows);
}

void ForcedConvectionPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::ThermalMaterial, matId));
}

} // namespace oor::model
