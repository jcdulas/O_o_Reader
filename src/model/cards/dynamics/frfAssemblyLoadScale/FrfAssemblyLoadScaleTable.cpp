#include "oor/model/cards/dynamics/frfAssemblyLoadScale/FrfAssemblyLoadScaleTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<FrfAssemblyLoadScaleCard> FrfAssemblyLoadScaleFormat::read(FieldReader& reader)
{
    FrfAssemblyLoadScaleCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frfComponent = readFrfComponent(reader, 2, "frfComponent");
    card.pointId = reader.requiredInteger(3, "pointId");
    card.components = reader.integer(4, "components", 0);
    card.scaleFactor = reader.requiredReal(5, "scaleFactor");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("scaleFactor"), "scaleFactor");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfAssemblyLoadScaleFormat::check(const FrfAssemblyLoadScaleCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkFrfComponent(card.frfComponent, "frfComponent"))
        return problem;
    if (const auto problem = checkRequiredId(card.pointId, "pointId"))
        return problem;
    if (const auto problem = checkSingleComponent(card.components, "components"))
        return problem;
    return checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite);
}

void FrfAssemblyLoadScaleFormat::write(const FrfAssemblyLoadScaleCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeFrfComponent(writer, 2, card.frfComponent);
    writer.integer(3, card.pointId);
    writer.integerUnlessDefault(4, card.components, 0);
    writer.real(5, card.scaleFactor);
}

std::uint32_t FrfAssemblyLoadScaleTable::append(const FrfAssemblyLoadScaleCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frfComponentId.push(card.frfComponent.id);
    m_frfComponentName.push(card.frfComponent.name);
    m_pointId.push(card.pointId);
    m_components.push(card.components);
    m_scaleFactor.push(card.scaleFactor);
    return row;
}

FrfAssemblyLoadScaleCard FrfAssemblyLoadScaleTable::card(std::uint32_t row) const
{
    FrfAssemblyLoadScaleCard card;
    card.id = m_id.get(row);
    card.frfComponent = FrfComponentRef{m_frfComponentId.get(row), m_frfComponentName.get(row)};
    card.pointId = m_pointId.get(row);
    card.components = m_components.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    return card;
}

void FrfAssemblyLoadScaleTable::store(std::uint32_t row, const FrfAssemblyLoadScaleCard& card)
{
    m_id.set(row, card.id);
    m_frfComponentId.set(row, card.frfComponent.id);
    m_frfComponentName.set(row, card.frfComponent.name);
    m_pointId.set(row, card.pointId);
    m_components.set(row, card.components);
    m_scaleFactor.set(row, card.scaleFactor);
}

void FrfAssemblyLoadScaleTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frfComponentId.keepRows(rows);
    m_frfComponentName.keepRows(rows);
    m_pointId.keepRows(rows);
    m_components.keepRows(rows);
    m_scaleFactor.keepRows(rows);
}

void FrfAssemblyLoadScaleTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The component named by its ID; a name is not an ID. The point belongs to the component's own model.
    if (const base::EntityId frfComponent = m_frfComponentId.get(row); frfComponent > 0)
        m_frfComponentId.set(row, visitor.visit("frfComponent", ReferenceTarget::FrfComponent, frfComponent));
}

} // namespace oor::model
