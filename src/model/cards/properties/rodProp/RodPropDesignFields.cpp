#include "oor/model/cards/properties/rodProp/RodPropDesignFields.hpp"

#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum RodPropKind : std::int32_t
{
    Area,
    Torsion,
    StressCoefficient,
    NonstructuralMass,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "area, torsionConstant, torsionStressCoefficient, nonStructMass";

DesignFieldLookup foundKind(std::int32_t kind)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    switch (kind)
    {
    case Area:
        return DesignFieldLookup::found(designFacts(kind, "area", Positive, Linear));
    case Torsion:
        return DesignFieldLookup::found(designFacts(kind, "torsionConstant", Positive));
    case StressCoefficient:
        return DesignFieldLookup::found(designFacts(kind, "torsionStressCoefficient", Signed));
    default:
        return DesignFieldLookup::found(designFacts(kind, "nonStructMass", Positive, Linear));
    }
}

DesignFieldLookup findField(const RodPropCard&, const DesignField& field)
{
    if (field.name == "area")
        return foundKind(Area);
    if (field.name == "torsionConstant")
        return foundKind(Torsion);
    if (field.name == "torsionStressCoefficient")
        return foundKind(StressCoefficient);
    if (field.name == "nonStructMass")
        return foundKind(NonstructuralMass);
    return DesignFieldLookup::refused(refusalOfField("RodProp", field, DESIGNABLE));
}

double readField(const RodPropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Area:
        return card.area;
    case Torsion:
        return card.torsion;
    case StressCoefficient:
        return card.stressCoefficient;
    default:
        return card.nonStructMass;
    }
}

void writeFields(RodPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Area:
            card.area = item.value;
            break;
        case Torsion:
            card.torsion = item.value;
            break;
        case StressCoefficient:
            card.stressCoefficient = item.value;
            break;
        default:
            card.nonStructMass = item.value;
            break;
        }
    }
}

// The contract columns: a, j, c, nsm.
DesignColumn columnOf(const RodPropCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Area:
        return {"area"};
    case Torsion:
        return {"torsionConstant"};
    case StressCoefficient:
        return {"torsionStressCoefficient"};
    default:
        return {"nonStructMass"};
    }
}

} // namespace

const DesignableCard& rodPropDesignFields()
{
    static const TableDesignFields<RodPropTable> FIELDS("RodProp", {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
