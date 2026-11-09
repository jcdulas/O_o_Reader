#include "oor/model/cards/materials/isoMat/IsoMatDesignFields.hpp"

#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum IsoMatKind : std::int32_t
{
    Young,
    Shear,
    Poisson,
    Density,
    ThermalExpansion,
    ReferenceTemperature,
    Damping,
    TensionLimit,
    CompressionLimit,
    ShearLimit,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "youngsModulus, shearModulus, poissonRatio, density, thermalExpansion, "
    "referenceTemperature, structuralDamping, tensionAllowable, compressionAllowable, "
    "shearAllowable";

DesignFieldLookup findField(const IsoMatCard&, const DesignField& field)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    if (field.name == "youngsModulus")
        return DesignFieldLookup::found(designFacts(Young, "youngsModulus", Positive));
    if (field.name == "shearModulus")
        return DesignFieldLookup::found(designFacts(Shear, "shearModulus", Positive));
    if (field.name == "poissonRatio")
        return DesignFieldLookup::found(designFacts(Poisson, "poissonRatio", Signed));
    if (field.name == "density")
        return DesignFieldLookup::found(designFacts(Density, "density", Positive, Linear));
    if (field.name == "thermalExpansion")
        return DesignFieldLookup::found(designFacts(ThermalExpansion, "thermalExpansion", Signed));
    if (field.name == "referenceTemperature")
        return DesignFieldLookup::found(designFacts(ReferenceTemperature, "referenceTemperature", Signed));
    if (field.name == "structuralDamping")
        return DesignFieldLookup::found(designFacts(Damping, "structuralDamping", Signed));
    if (field.name == "tensionAllowable")
        return DesignFieldLookup::found(designFacts(TensionLimit, "tensionAllowable", Positive));
    if (field.name == "compressionAllowable")
        return DesignFieldLookup::found(designFacts(CompressionLimit, "compressionAllowable", Positive));
    if (field.name == "shearAllowable")
        return DesignFieldLookup::found(designFacts(ShearLimit, "shearAllowable", Positive));
    return DesignFieldLookup::refused(refusalOfField("IsoMat", field, DESIGNABLE));
}

double readField(const IsoMatCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Young:
        return card.youngsModulus;
    case Shear:
        return card.shearModulus;
    case Poisson:
        return card.poissonRatio;
    case Density:
        return card.density;
    case ThermalExpansion:
        return card.thermalExpansion;
    case ReferenceTemperature:
        return card.referenceTemperature;
    case Damping:
        return card.structuralDamping;
    case TensionLimit:
        return card.tensionAllowable;
    case CompressionLimit:
        return card.compressionAllowable;
    default:
        return card.shearAllowable;
    }
}

// The one of E, G and NU that the card left blank, if exactly one was and it still holds the value the other two give
// (G = E/(2(1+NU)), E = 2(1+NU)G, NU = E/(2G) - 1); none when the card gave all three,
// or only one of E and G (NU is then 0 and nothing is derived), or when the value was edited since.
std::uint8_t derivedField(const IsoMatCard& card)
{
    switch (card.blankFields)
    {
    case IsoMatCard::BLANK_SHEAR_MODULUS:
        return holdsDefault(card.shearModulus, card.youngsModulus / (2.0 * (1.0 + card.poissonRatio)))
                   ? IsoMatCard::BLANK_SHEAR_MODULUS
                   : 0;
    case IsoMatCard::BLANK_YOUNGS_MODULUS:
        return holdsDefault(card.youngsModulus, 2.0 * (1.0 + card.poissonRatio) * card.shearModulus)
                   ? IsoMatCard::BLANK_YOUNGS_MODULUS
                   : 0;
    case IsoMatCard::BLANK_POISSON_RATIO:
        return card.shearModulus != 0.0 &&
                       holdsDefault(card.poissonRatio, card.youngsModulus / (2.0 * card.shearModulus) - 1.0)
                   ? IsoMatCard::BLANK_POISSON_RATIO
                   : 0;
    default:
        return 0;
    }
}

// The card stores E, G and NU resolved, so the one the card left blank follows the other two when they are designed,
// as the solver derives it again (a field that is designed itself is left as the relation gives it).
void writeFields(IsoMatCard& card, std::span<const DesignFieldValue> values)
{
    const std::uint8_t derived = derivedField(card);
    const double oldYoung = card.youngsModulus;
    const double oldShear = card.shearModulus;
    const double oldPoisson = card.poissonRatio;
    std::uint8_t designed = 0;
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Young:
            card.youngsModulus = item.value;
            designed |= IsoMatCard::BLANK_YOUNGS_MODULUS;
            break;
        case Shear:
            card.shearModulus = item.value;
            designed |= IsoMatCard::BLANK_SHEAR_MODULUS;
            break;
        case Poisson:
            card.poissonRatio = item.value;
            designed |= IsoMatCard::BLANK_POISSON_RATIO;
            break;
        case Density:
            card.density = item.value;
            break;
        case ThermalExpansion:
            card.thermalExpansion = item.value;
            break;
        case ReferenceTemperature:
            card.referenceTemperature = item.value;
            break;
        case Damping:
            card.structuralDamping = item.value;
            break;
        case TensionLimit:
            card.tensionAllowable = item.value;
            break;
        case CompressionLimit:
            card.compressionAllowable = item.value;
            break;
        default:
            card.shearAllowable = item.value;
            break;
        }
    }
    const bool modulusChanged =
        card.youngsModulus != oldYoung || card.shearModulus != oldShear || card.poissonRatio != oldPoisson;
    if (derived == 0 || (designed & derived) != 0 || !modulusChanged)
        return;
    if (derived == IsoMatCard::BLANK_SHEAR_MODULUS)
        card.shearModulus = card.youngsModulus / (2.0 * (1.0 + card.poissonRatio));
    else if (derived == IsoMatCard::BLANK_YOUNGS_MODULUS)
        card.youngsModulus = 2.0 * card.shearModulus *
                             (1.0 + card.poissonRatio); // the expression of IsoMatFormat::resolve, to the last bit
    else if (card.shearModulus != 0.0)
        card.poissonRatio = card.youngsModulus / (2.0 * card.shearModulus) - 1.0;
}

// The contract columns have the names of the fields, in lower case.
DesignColumn columnOf(const IsoMatCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Young:
        return {"youngsModulus"};
    case Shear:
        return {"shearModulus"};
    case Poisson:
        return {"poissonRatio"};
    case Density:
        return {"density"};
    case ThermalExpansion:
        return {"thermalExpansion"};
    case ReferenceTemperature:
        return {"referenceTemperature"};
    case Damping:
        return {"structuralDamping"};
    case TensionLimit:
        return {"tensionAllowable"};
    case CompressionLimit:
        return {"compressionAllowable"};
    default:
        return {"shearAllowable"};
    }
}

} // namespace

const DesignableCard& isoMatDesignFields()
{
    static const TableDesignFields<IsoMatTable> FIELDS("IsoMat", {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
