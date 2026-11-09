#include "AeroCheckSupport.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroComponent/AeroComponentTable.hpp"
#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"

namespace oor::analysis
{

std::unordered_map<std::string, ControlLabelOwner> controlLabels(const model::Model& model)
{
    std::unordered_map<std::string, ControlLabelOwner> labels;
    forEachCard<model::AeroControlSurfaceTable>(
        model, [&](const model::AeroControlSurfaceCard& card, model::CardLocation where)
        { labels.try_emplace(std::string(card.label), "AeroControlSurface", card.id, where); });
    forEachCard<model::AeroRigidTrimVariableTable>(
        model, [&](const model::AeroRigidTrimVariableCard& card, model::CardLocation where)
        { labels.try_emplace(std::string(card.label), "AeroRigidTrimVariable", card.id, where); });
    forEachCard<model::AeroControllerTable>(
        model, [&](const model::AeroControllerCard& card, model::CardLocation where)
        { labels.try_emplace(std::string(card.label), "AeroController", card.id, where); });
    return labels;
}

std::unordered_set<std::string> componentNames(const model::Model& model)
{
    std::unordered_set<std::string> names;
    forEachCard<model::AeroComponentTable>(model, [&](const model::AeroComponentCard& card, model::CardLocation)
                                           { names.insert(std::string(card.name)); });
    forEachCard<model::AeroComponentUnionTable>(model,
                                                [&](const model::AeroComponentUnionCard& card, model::CardLocation)
                                                { names.insert(std::string(card.name)); });
    return names;
}

std::optional<std::size_t> aefactSize(const model::Model& model, base::EntityId list)
{
    const auto* table = model.findTable<model::AeroFactorListTable>();
    if (table == nullptr || list <= 0)
        return std::nullopt;
    const std::optional<model::CardLocation> found =
        model.locateInType(model::typeIndexOf<model::AeroFactorListTable>(), list);
    if (!found)
        return std::nullopt;
    return table->valuesAt(found->row).size();
}

std::optional<std::int64_t> divisionElements(const model::Model& model, std::int32_t count, base::EntityId list)
{
    if (count > 0)
        return count;
    const std::optional<std::size_t> points = aefactSize(model, list);
    if (!points)
        return std::nullopt;
    return *points >= 2 ? static_cast<std::int64_t>(*points) - 1 : 0;
}

} // namespace oor::analysis
