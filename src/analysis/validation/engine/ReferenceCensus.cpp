#include "oor/analysis/validation/engine/ReferenceCensus.hpp"

#include "oor/model/references/ReferenceChecker.hpp"

namespace oor::analysis
{

ReferenceCensus ReferenceCensus::build(const model::Model& model)
{
    ReferenceCensus census;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    model::ReferenceChecker::scan(
        model,
        [&census, &registry](const model::ReferenceUse& use)
        {
            if (!use.inCaseControl)
            {
                const model::CardCategory category = registry.entry(use.location.table).info.category;
                if (use.target == model::ReferenceTarget::Property && category == model::CardCategory::Element)
                    census.elementProperties.push_back(CardReference{use.location, std::string(use.field), use.id});
                else if (use.target == model::ReferenceTarget::Material && category == model::CardCategory::Property)
                    census.propertyMaterials.push_back(CardReference{use.location, std::string(use.field), use.id});
                else if (use.target == model::ReferenceTarget::ElementInRange &&
                         category == model::CardCategory::ElementList)
                    census.elementListMembers.push_back(CardReference{use.location, std::string(use.field), use.id});
                else if (use.target == model::ReferenceTarget::Property && category == model::CardCategory::Property)
                    census.propertyExtensions.push_back(CardReference{use.location, std::string(use.field), use.id});
            }
            switch (use.target)
            {
            case model::ReferenceTarget::Node:
            case model::ReferenceTarget::NodeInRange:
            case model::ReferenceTarget::ScalarOrGridPoint:
                census.nodes.insert(use.id);
                break;
            case model::ReferenceTarget::Property:
            case model::ReferenceTarget::PropertyInRange:
                census.properties.insert(use.id);
                break;
            case model::ReferenceTarget::Material:
                census.materials.insert(use.id);
                break;
            case model::ReferenceTarget::Coordinate:
                census.coordinates.insert(use.id);
                break;
            case model::ReferenceTarget::LoadCombinationSet:
            case model::ReferenceTarget::ExcitationSet:
            case model::ReferenceTarget::DynamicLoadSet:
                census.loadSets.insert(use.id);
                break;
            case model::ReferenceTarget::ConstraintSet:
                census.constraintSets.insert(use.id);
                break;
            default:
                break;
            }
            if (use.resolved)
                return;
            UnresolvedUse missing;
            missing.location = use.location;
            missing.field = std::string(use.field);
            missing.target = use.target;
            missing.id = use.id;
            missing.inCaseControl = use.inCaseControl;
            missing.scope = std::string(use.scope);
            missing.scopeNumber = use.scopeNumber;
            census.unresolved.push_back(std::move(missing));
        });
    return census;
}

} // namespace oor::analysis
