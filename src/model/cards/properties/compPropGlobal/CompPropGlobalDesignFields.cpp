#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalDesignFields.hpp"

#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum CompPropGlobalKind : std::int32_t
{
    ReferencePlane, // Z0
    NonstructuralMass,
    BondShearAllowable,
    ReferenceTemperature,
    Damping,
    PlyThickness, // `index` is the ply, from 0
    PlyAngle,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "bottomOffset, nonStructMass, bondAllowable, referenceTemperature, "
    "structuralDamping, plyThickness(1), plyThickness(2), ..., plyAngle(1), "
    "plyAngle(2), ...";

DesignFieldLookup foundWhole(std::int32_t kind)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    switch (kind)
    {
    case ReferencePlane:
        return DesignFieldLookup::found(designFacts(kind, "bottomOffset", StressRecovery));
    case NonstructuralMass:
        return DesignFieldLookup::found(designFacts(kind, "nonStructMass", Positive, Linear));
    case BondShearAllowable:
        return DesignFieldLookup::found(designFacts(kind, "bondAllowable", Positive));
    case ReferenceTemperature:
        return DesignFieldLookup::found(designFacts(kind, "referenceTemperature", Signed));
    default:
        return DesignFieldLookup::found(designFacts(kind, "structuralDamping", Signed));
    }
}

DesignFieldLookup foundPly(const CompPropGlobalCard& card, std::int32_t kind, int ply, const DesignField& field)
{
    if (ply < 0 || static_cast<std::size_t>(ply) >= card.plies.size())
        return DesignFieldLookup::refused("this CompPropGlobal has " + std::to_string(card.plies.size()) +
                                          " plies, so " + describeDesignField(field) + " is not one of them");
    if (kind == PlyThickness)
        return DesignFieldLookup::found(designFacts(kind, "plyThickness(" + std::to_string(ply + 1) + ")",
                                                    DesignLowerLimit::Positive, DesignMassEffect::Linear, ply));
    return DesignFieldLookup::found(designFacts(kind, "plyAngle(" + std::to_string(ply + 1) + ")",
                                                DesignLowerLimit::Signed, DesignMassEffect::None, ply));
}

DesignFieldLookup findField(const CompPropGlobalCard& card, const DesignField& field)
{
    if (field.name == "bottomOffset")
        return foundWhole(ReferencePlane);
    if (field.name == "nonStructMass")
        return foundWhole(NonstructuralMass);
    if (field.name == "bondAllowable")
        return foundWhole(BondShearAllowable);
    if (field.name == "referenceTemperature")
        return foundWhole(ReferenceTemperature);
    if (field.name == "structuralDamping")
        return foundWhole(Damping);
    const NumberedName named = splitTrailingNumber(field.name);
    if (named.number >= 1 && named.word == "plyThickness")
        return foundPly(card, PlyThickness, named.number - 1, field);
    if (named.number >= 1 && named.word == "plyAngle")
        return foundPly(card, PlyAngle, named.number - 1, field);
    return DesignFieldLookup::refused(refusalOfField("CompPropGlobal", field, DESIGNABLE));
}

double readField(const CompPropGlobalCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case ReferencePlane:
        return card.bottomOffset;
    case NonstructuralMass:
        return card.nonStructMass;
    case BondShearAllowable:
        return card.bondShearAllowable;
    case ReferenceTemperature:
        return card.referenceTemperature;
    case Damping:
        return card.structuralDamping;
    case PlyThickness:
        return card.plies[static_cast<std::size_t>(facts.index)].thickness;
    default:
        return card.plies[static_cast<std::size_t>(facts.index)].thetaInDegrees;
    }
}

// Z0 blank is minus half the thickness of the laminate; the card stores it resolved, so it follows a thickness that is
// designed when it still held that default (as for a PCOMP).
void writeFields(CompPropGlobalCard& card, std::span<const DesignFieldValue> values)
{
    const double oldTotal = card.totalThickness();
    const bool z0AtDefault = !card.bottomOffsetGiven; // a blank Z0, not a value equal to it
    bool designsZ0 = false;
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case ReferencePlane:
            card.bottomOffset = item.value;
            card.bottomOffsetGiven = true;
            designsZ0 = true;
            break;
        case NonstructuralMass:
            card.nonStructMass = item.value;
            break;
        case BondShearAllowable:
            card.bondShearAllowable = item.value;
            break;
        case ReferenceTemperature:
            card.referenceTemperature = item.value;
            break;
        case Damping:
            card.structuralDamping = item.value;
            break;
        case PlyThickness:
            card.plies[static_cast<std::size_t>(item.facts->index)].thickness = item.value;
            break;
        default:
            card.plies[static_cast<std::size_t>(item.facts->index)].thetaInDegrees = item.value;
            break;
        }
    }
    if (z0AtDefault && !designsZ0 && card.totalThickness() != oldTotal)
        card.bottomOffset = -0.5 * card.totalThickness();
}

// The contract columns: bottomOffset, nsm, sb, tref, ge, and the ply lists ply_thickness and ply_theta.
DesignColumn columnOf(const CompPropGlobalCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case ReferencePlane:
        return {"bottomOffset"};
    case NonstructuralMass:
        return {"nonStructMass"};
    case BondShearAllowable:
        return {"bondAllowable"};
    case ReferenceTemperature:
        return {"referenceTemperature"};
    case Damping:
        return {"structuralDamping"};
    case PlyThickness:
        return {"plyThickness", facts.index};
    default:
        return {"plyTheta", facts.index};
    }
}

} // namespace

const DesignableCard& compPropGlobalDesignFields()
{
    static const TableDesignFields<CompPropGlobalTable> FIELDS("CompPropGlobal",
                                                               {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
