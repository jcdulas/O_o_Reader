#include "oor/query/named/common/ThermalMaterials.hpp"

#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"

namespace oor::query
{

std::optional<model::CardLocation> thermalMaterialOfElement(CardFacts& facts, const model::Model& model,
                                                            model::CardLocation element)
{
    base::EntityId material = 0;
    if (const base::EntityId pid = facts.firstReference(element, model::ReferenceTarget::Property); pid != 0)
    {
        if (const auto property = model.locate(base::IdSpace::Property, pid))
            material = facts.firstReference(*property, model::ReferenceTarget::Material);
    }
    else
    {
        material = facts.firstReference(element, model::ReferenceTarget::Material); // a CONROD holds its material
    }
    if (material == 0)
        return std::nullopt;
    return model.locate(base::IdSpace::ThermalMaterial, material);
}

double heatGenerationMultiplier(CardFacts& facts, const model::Model& model, model::CardLocation element)
{
    const std::optional<model::CardLocation> thermal = thermalMaterialOfElement(facts, model, element);
    if (!thermal)
        return 1.0;
    const model::CardTable& table = *model.findTable(thermal->table);
    if (thermal->table == model::typeIndexOf<model::HeatIsoMatTable>())
        return static_cast<const model::HeatIsoMatTable&>(table).card(thermal->row).heatGenerationFactor;
    if (thermal->table == model::typeIndexOf<model::HeatAnisoMatTable>())
        return static_cast<const model::HeatAnisoMatTable&>(table).card(thermal->row).heatGenerationFactor;
    return 1.0;
}

std::optional<double> isotropicConductivity(const model::Model& model, model::CardLocation material)
{
    if (material.table != model::typeIndexOf<model::HeatIsoMatTable>())
        return std::nullopt;
    return static_cast<const model::HeatIsoMatTable&>(*model.findTable(material.table)).card(material.row).conductivity;
}

bool conductivityDependsOnTemperature(const model::Model& model, base::EntityId mid)
{
    const model::HeatIsoMatTemperatureTable* table = model.findTable<model::HeatIsoMatTemperatureTable>();
    if (table == nullptr)
        return false;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const model::HeatIsoMatTemperatureCard card = table->card(row);
        if (card.id == mid && card.conductivityTableId != 0)
            return true;
    }
    return false;
}

} // namespace oor::query
