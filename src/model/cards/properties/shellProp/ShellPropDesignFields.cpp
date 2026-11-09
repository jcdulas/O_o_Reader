#include "oor/model/cards/properties/shellProp/ShellPropDesignFields.hpp"

#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum ShellPropKind : std::int32_t
{
    Thickness,
    BendingRatio,
    ShearRatio,
    NonstructuralMass,
    FiberDistance1,
    FiberDistance2,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "thickness, shearThicknessRatio, nonStructMass, fiberDistance1, "
    "fiberDistance2 and bendingStiffnessRatio";

DesignFieldLookup findField(const ShellPropCard&, const DesignField& field)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    if (field.name == "thickness")
        return DesignFieldLookup::found(designFacts(Thickness, "thickness", Positive, Linear));
    if (field.name == "bendingStiffnessRatio")
        return DesignFieldLookup::found(designFacts(BendingRatio, "bendingStiffnessRatio", Positive));
    if (field.name == "shearThicknessRatio")
        return DesignFieldLookup::found(designFacts(ShearRatio, "shearThicknessRatio", Positive));
    if (field.name == "nonStructMass")
        return DesignFieldLookup::found(designFacts(NonstructuralMass, "nonStructMass", Positive, Linear));
    if (field.name == "fiberDistance1")
        return DesignFieldLookup::found(designFacts(FiberDistance1, "fiberDistance1", StressRecovery));
    if (field.name == "fiberDistance2")
        return DesignFieldLookup::found(designFacts(FiberDistance2, "fiberDistance2", StressRecovery));
    return DesignFieldLookup::refused(refusalOfField("ShellProp", field, DESIGNABLE));
}

double readField(const ShellPropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Thickness:
        return card.thickness;
    case BendingRatio:
        return card.bendingStiffnessRatio;
    case ShearRatio:
        return card.shearThicknessRatio;
    case NonstructuralMass:
        return card.nonStructMass;
    case FiberDistance1: // a blank fibre distance is half the thickness of the property
        return card.fiberDistance1.value_or(-0.5 * card.thickness);
    default:
        return card.fiberDistance2.value_or(0.5 * card.thickness);
    }
}

// Z1 and Z2 blank are -T/2 and +T/2 for the thickness the solver uses. They stay blank when the thickness is designed,
// and so follow it; a Z1 that was given stays as given.
void writeFields(ShellPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Thickness:
            card.thickness = item.value;
            break;
        case BendingRatio:
            card.bendingStiffnessRatio = item.value;
            break;
        case ShearRatio:
            card.shearThicknessRatio = item.value;
            break;
        case NonstructuralMass:
            card.nonStructMass = item.value;
            break;
        case FiberDistance1:
            card.fiberDistance1 = item.value;
            break;
        default:
            card.fiberDistance2 = item.value;
            break;
        }
    }
}

// The contract columns: thickness (T), bendingStiffnessRatio (12I/T**3), shearThicknessRatio (TS/T), nonStructMass
// (NSM), fiberDistance1 (Z1), fiberDistance2 (Z2).
DesignColumn columnOf(const ShellPropCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Thickness:
        return {"thickness"};
    case BendingRatio:
        return {"bendingStiffnessRatio"};
    case ShearRatio:
        return {"shearThicknessRatio"};
    case NonstructuralMass:
        return {"nonStructMass"};
    case FiberDistance1:
        return {"fiberDistance1"};
    default:
        return {"fiberDistance2"};
    }
}

} // namespace

const DesignableCard& shellPropDesignFields()
{
    static const TableDesignFields<ShellPropTable> FIELDS("ShellProp",
                                                          {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
