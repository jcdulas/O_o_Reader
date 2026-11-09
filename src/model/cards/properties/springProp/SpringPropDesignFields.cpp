#include "oor/model/cards/properties/springProp/SpringPropDesignFields.hpp"

#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum SpringPropKind : std::int32_t
{
    Stiffness,
    Damping,
    StressCoefficient,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "stiffness, structuralDamping or stressCoefficient";

DesignFieldLookup foundKind(std::int32_t kind)
{
    using enum DesignLowerLimit;
    switch (kind)
    {
    case Stiffness:
        return DesignFieldLookup::found(designFacts(kind, "stiffness", Signed));
    case Damping:
        return DesignFieldLookup::found(designFacts(kind, "structuralDamping", Signed));
    default:
        return DesignFieldLookup::found(designFacts(kind, "stressCoefficient", Signed));
    }
}

// Each row describes one property, regardless of its position in the record.
DesignFieldLookup findField(const SpringPropCard&, const DesignField& field)
{
    if (field.name == "stiffness")
        return foundKind(Stiffness);
    if (field.name == "structuralDamping")
        return foundKind(Damping);
    if (field.name == "stressCoefficient")
        return foundKind(StressCoefficient);
    return DesignFieldLookup::refused(refusalOfField("SpringProp", field, DESIGNABLE));
}

double readField(const SpringPropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Stiffness:
        return card.stiffness;
    case Damping:
        return card.structuralDamping;
    default:
        return card.stressCoefficient;
    }
}

void writeFields(SpringPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Stiffness:
            card.stiffness = item.value;
            break;
        case Damping:
            card.structuralDamping = item.value;
            break;
        default:
            card.stressCoefficient = item.value;
            break;
        }
    }
}

// The corresponding table columns.
DesignColumn columnOf(const SpringPropCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Stiffness:
        return {"stiffness"};
    case Damping:
        return {"structuralDamping"};
    default:
        return {"stressCoefficient"};
    }
}

} // namespace

const DesignableCard& springPropDesignFields()
{
    static const TableDesignFields<SpringPropTable> FIELDS("SpringProp",
                                                           {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
