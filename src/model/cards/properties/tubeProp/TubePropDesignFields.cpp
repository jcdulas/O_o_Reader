#include "oor/model/cards/properties/tubeProp/TubePropDesignFields.hpp"

#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum TubePropKind : std::int32_t
{
    OuterDiameter,
    WallThickness,
    NonstructuralMass,
    OuterDiameter2,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "outerDiameter, thickness, nonStructMass, outerDiameter2";

DesignFieldLookup foundKind(std::int32_t kind)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    switch (kind)
    {
    case OuterDiameter:
        return DesignFieldLookup::found(designFacts(kind, "outerDiameter", Positive, Nonlinear));
    case WallThickness:
        return DesignFieldLookup::found(designFacts(kind, "thickness", Positive, Nonlinear));
    case NonstructuralMass:
        return DesignFieldLookup::found(designFacts(kind, "nonStructMass", Positive, Linear));
    default:
        return DesignFieldLookup::found(designFacts(kind, "outerDiameter2", Positive));
    }
}

DesignFieldLookup findField(const TubePropCard&, const DesignField& field)
{
    if (field.name == "outerDiameter")
        return foundKind(OuterDiameter);
    if (field.name == "thickness")
        return foundKind(WallThickness);
    if (field.name == "nonStructMass")
        return foundKind(NonstructuralMass);
    if (field.name == "outerDiameter2")
        return foundKind(OuterDiameter2);
    return DesignFieldLookup::refused(refusalOfField("TubeProp", field, DESIGNABLE));
}

double readField(const TubePropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case OuterDiameter:
        return card.outerDiameter;
    case WallThickness:
        return card.thickness;
    case NonstructuralMass:
        return card.nonStructMass;
    default:
        return card.outerDiameter2;
    }
}

// A blank T is half the outer diameter (a solid rod) and a blank OD2 is OD. The card stores them resolved, so a
// diameter that is designed takes them along unless they were given: a T that is OD/2 for the old diameter is a blank,
// and follows.
void writeFields(TubePropCard& card, std::span<const DesignFieldValue> values)
{
    const double oldDiameter = card.outerDiameter;
    // blanks, not values equal to them
    const bool thicknessAtDefault = !card.thicknessGiven;
    const bool diameter2AtDefault = !card.outerDiameter2Given;
    bool designsThickness = false;
    bool designsDiameter2 = false;
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case OuterDiameter:
            card.outerDiameter = item.value;
            break;
        case WallThickness:
            card.thickness = item.value;
            card.thicknessGiven = true;
            designsThickness = true;
            break;
        case NonstructuralMass:
            card.nonStructMass = item.value;
            break;
        default:
            card.outerDiameter2 = item.value;
            card.outerDiameter2Given = true;
            designsDiameter2 = true;
            break;
        }
    }
    if (card.outerDiameter != oldDiameter)
    {
        if (thicknessAtDefault && !designsThickness)
            card.thickness = 0.5 * card.outerDiameter;
        if (diameter2AtDefault && !designsDiameter2)
            card.outerDiameter2 = card.outerDiameter;
    }
}

// The contract columns: od, t, nsm, od2.
DesignColumn columnOf(const TubePropCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case OuterDiameter:
        return {"outerDiameter"};
    case WallThickness:
        return {"thickness"};
    case NonstructuralMass:
        return {"nonStructMass"};
    default:
        return {"outerDiameter2"};
    }
}

} // namespace

const DesignableCard& tubePropDesignFields()
{
    static const TableDesignFields<TubePropTable> FIELDS("TubeProp", {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
