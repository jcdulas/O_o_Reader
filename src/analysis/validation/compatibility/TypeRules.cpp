#include "TypeRules.hpp"

namespace oor::analysis
{

bool isConductionMaterialField(std::string_view property, std::string_view field) noexcept
{
    if (property == "ShellProp")
        return field == "membraneMatId";
    return isOneOf(property, {"SolidProp", "RodProp", "TubeProp", "BarProp", "BarShapeProp", "BeamProp",
                              "BeamShapeProp", "CompProp", "CompPropGlobal"});
}

namespace
{

// The message of a finding: the card type `user` points at `what` `id`, a card of the type `name`. A message names the
// card types by their deck keyword (docs/naming.md).
std::string wrongTypeMessage(const model::Model& model, std::string_view what, base::EntityId id, std::string_view name,
                             std::string_view user, std::initializer_list<std::string_view> needs)
{
    return "uses " + std::string(what) + " " + std::to_string(id) + ", which is a " + std::string(name) + ": " +
           std::string(user) + " needs a " + cardLabels(model, needs);
}

} // namespace

void reportWrongPropertyType(const ValidationContext& context, FindingSink& sink,
                             std::initializer_list<std::string_view> elements,
                             std::initializer_list<std::string_view> allowed,
                             std::initializer_list<std::string_view> needs)
{
    const model::Model& model = context.model();
    for (const CardReference& use : context.references().elementProperties)
    {
        const std::string_view element = cardNameAt(model, use.location);
        // Only the property of the element: a CFAST names the shell properties it joins in IDA and IDB.
        if (use.field != "propId" || !isOneOf(element, elements))
            continue;
        const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, use.id);
        if (!property)
            continue; // a property that does not exist is the business of ref.unresolved
        const std::string_view name = cardNameAt(model, *property);
        if (isOneOf(name, allowed))
            continue;
        sink.reportCard(use.location, use.field,
                        wrongTypeMessage(model, "property", use.id, name, element, needs.size() > 0 ? needs : allowed),
                        "point the element at a property of the right type");
    }
}

void reportRefusedMaterial(const ValidationContext& context, FindingSink& sink,
                           std::initializer_list<std::string_view> properties,
                           std::initializer_list<std::string_view> refused,
                           std::initializer_list<std::string_view> needs)
{
    const model::Model& model = context.model();
    for (const CardReference& use : context.references().propertyMaterials)
    {
        const std::string_view property = cardNameAt(model, use.location);
        if (!isOneOf(property, properties))
            continue;
        const std::optional<model::CardLocation> material = model.locate(base::IdSpace::Material, use.id);
        if (!material)
            continue;
        const std::string_view name = cardNameAt(model, *material);
        if (!isOneOf(name, refused))
            continue;
        sink.reportCard(use.location, use.field, wrongTypeMessage(model, "material", use.id, name, property, needs),
                        "use a material of the right type");
    }
}

void reportUnlistedMaterial(const ValidationContext& context, FindingSink& sink,
                            std::initializer_list<std::string_view> properties,
                            std::initializer_list<std::string_view> allowed,
                            std::initializer_list<std::string_view> needs)
{
    const model::Model& model = context.model();
    for (const CardReference& use : context.references().propertyMaterials)
    {
        const std::string_view property = cardNameAt(model, use.location);
        if (!isOneOf(property, properties))
            continue;
        const std::optional<model::CardLocation> material = model.locate(base::IdSpace::Material, use.id);
        if (!material)
            continue;
        const std::string_view name = cardNameAt(model, *material);
        if (isOneOf(name, allowed))
            continue;
        sink.reportCard(use.location, use.field, wrongTypeMessage(model, "material", use.id, name, property, needs),
                        "use a material of the right type");
    }
}

} // namespace oor::analysis
