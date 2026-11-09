#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropDesignFields.hpp"

#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum ShearPanelPropKind : std::int32_t
{
    Thickness,
    NonstructuralMass,
    EdgeFactor1,
    EdgeFactor2,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "thickness, nonStructMass, extensionalFactor1, extensionalFactor2";

DesignFieldLookup foundKind(std::int32_t kind)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    switch (kind)
    {
    case Thickness:
        return DesignFieldLookup::found(designFacts(kind, "thickness", Positive, Linear));
    case NonstructuralMass:
        return DesignFieldLookup::found(designFacts(kind, "nonStructMass", Positive, Linear));
    case EdgeFactor1:
        return DesignFieldLookup::found(designFacts(kind, "extensionalFactor1", Signed));
    default:
        return DesignFieldLookup::found(designFacts(kind, "extensionalFactor2", Signed));
    }
}

DesignFieldLookup findField(const ShearPanelPropCard&, const DesignField& field)
{
    if (field.name == "thickness")
        return foundKind(Thickness);
    if (field.name == "nonStructMass")
        return foundKind(NonstructuralMass);
    if (field.name == "extensionalFactor1")
        return foundKind(EdgeFactor1);
    if (field.name == "extensionalFactor2")
        return foundKind(EdgeFactor2);
    return DesignFieldLookup::refused(refusalOfField("ShearPanelProp", field, DESIGNABLE));
}

double readField(const ShearPanelPropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Thickness:
        return card.thickness;
    case NonstructuralMass:
        return card.nonStructMass;
    case EdgeFactor1:
        return card.extensionalFactor1;
    default:
        return card.extensionalFactor2;
    }
}

void writeFields(ShearPanelPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Thickness:
            card.thickness = item.value;
            break;
        case NonstructuralMass:
            card.nonStructMass = item.value;
            break;
        case EdgeFactor1:
            card.extensionalFactor1 = item.value;
            break;
        default:
            card.extensionalFactor2 = item.value;
            break;
        }
    }
}

// The contract columns: t, nsm, f1, f2.
DesignColumn columnOf(const ShearPanelPropCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Thickness:
        return {"thickness"};
    case NonstructuralMass:
        return {"nonStructMass"};
    case EdgeFactor1:
        return {"extensionalFactor1"};
    default:
        return {"extensionalFactor2"};
    }
}

} // namespace

const DesignableCard& shearPanelPropDesignFields()
{
    static const TableDesignFields<ShearPanelPropTable> FIELDS("ShearPanelProp",
                                                               {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
