#include "oor/model/cards/elements/springDofInline/SpringDofInlineDesignFields.hpp"

#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum SpringDofInlineKind : std::int32_t
{
    Stiffness,
    Damping,
    StressCoefficient,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "stiffness, structuralDamping, stressCoefficient";

DesignFieldLookup findField(const SpringDofInlineCard&, const DesignField& field)
{
    using enum DesignLowerLimit;
    if (field.name == "stiffness")
        return DesignFieldLookup::found(designFacts(Stiffness, "stiffness", Signed));
    if (field.name == "structuralDamping")
        return DesignFieldLookup::found(designFacts(Damping, "structuralDamping", Signed));
    if (field.name == "stressCoefficient")
        return DesignFieldLookup::found(designFacts(StressCoefficient, "stressCoefficient", Signed));
    return DesignFieldLookup::refused(refusalOfField("SpringDofInline", field, DESIGNABLE));
}

double readField(const SpringDofInlineCard& card, const DesignFieldFacts& facts)
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

void writeFields(SpringDofInlineCard& card, std::span<const DesignFieldValue> values)
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

// The contract columns: k, ge, s.
DesignColumn columnOf(const SpringDofInlineCard&, const DesignFieldFacts& facts)
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

const DesignableCard& springDofInlineDesignFields()
{
    static const TableDesignFields<SpringDofInlineTable> FIELDS("SpringDofInline",
                                                            {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
