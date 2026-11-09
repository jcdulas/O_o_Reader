#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DDesignFields.hpp"

#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

#include <limits>

namespace oor::model
{

namespace
{

enum OrthoMat2DKind : std::int32_t
{
    YoungFiber,
    YoungMatrix,
    Poisson12,
    Shear12,
    ShearTransverse1,
    ShearTransverse2,
    Density,
    Expansion1,
    Expansion2,
    ReferenceTemperature,
    TensionFiber,
    CompressionFiber,
    TensionMatrix,
    CompressionMatrix,
    ShearLimit,
    Damping,
    Interaction12,
    StrainFlag,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "youngsModulus1, youngsModulus2, poissonRatio12, shearModulus12, "
    "shearModulus1Z, shearModulus2Z, density, thermalExpansion1, "
    "thermalExpansion2, referenceTemperature, tensionStrength1, compressionStrength1, "
    "tensionStrength2, compressionStrength2, shearStrength, structuralDamping, "
    "interactionTerm12, strainAllowables";

DesignFieldLookup findField(const OrthoMat2DCard&, const DesignField& field)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    if (field.name == "youngsModulus1")
        return DesignFieldLookup::found(designFacts(YoungFiber, "youngsModulus1", Positive));
    if (field.name == "youngsModulus2")
        return DesignFieldLookup::found(designFacts(YoungMatrix, "youngsModulus2", Positive));
    if (field.name == "poissonRatio12")
        return DesignFieldLookup::found(designFacts(Poisson12, "poissonRatio12", Signed));
    if (field.name == "shearModulus12")
        return DesignFieldLookup::found(designFacts(Shear12, "shearModulus12", Positive));
    if (field.name == "shearModulus1Z")
        return DesignFieldLookup::found(designFacts(ShearTransverse1, "shearModulus1Z", Positive));
    if (field.name == "shearModulus2Z")
        return DesignFieldLookup::found(designFacts(ShearTransverse2, "shearModulus2Z", Positive));
    if (field.name == "density")
        return DesignFieldLookup::found(designFacts(Density, "density", Positive, Linear));
    if (field.name == "thermalExpansion1")
        return DesignFieldLookup::found(designFacts(Expansion1, "thermalExpansion1", Signed));
    if (field.name == "thermalExpansion2")
        return DesignFieldLookup::found(designFacts(Expansion2, "thermalExpansion2", Signed));
    if (field.name == "referenceTemperature")
        return DesignFieldLookup::found(designFacts(ReferenceTemperature, "referenceTemperature", Signed));
    if (field.name == "tensionStrength1")
        return DesignFieldLookup::found(designFacts(TensionFiber, "tensionStrength1", Positive));
    if (field.name == "compressionStrength1")
        return DesignFieldLookup::found(designFacts(CompressionFiber, "compressionStrength1", Positive));
    if (field.name == "tensionStrength2")
        return DesignFieldLookup::found(designFacts(TensionMatrix, "tensionStrength2", Positive));
    if (field.name == "compressionStrength2")
        return DesignFieldLookup::found(designFacts(CompressionMatrix, "compressionStrength2", Positive));
    if (field.name == "shearStrength")
        return DesignFieldLookup::found(designFacts(ShearLimit, "shearStrength", Positive));
    if (field.name == "structuralDamping")
        return DesignFieldLookup::found(designFacts(Damping, "structuralDamping", Signed));
    if (field.name == "interactionTerm12")
        return DesignFieldLookup::found(designFacts(Interaction12, "interactionTerm12", Signed));
    if (field.name == "strainAllowables")
        return DesignFieldLookup::found(designFacts(StrainFlag, "strainAllowables", Signed));
    return DesignFieldLookup::refused(refusalOfField("OrthoMat2D", field, DESIGNABLE));
}

double readField(const OrthoMat2DCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case YoungFiber:
        return card.youngsModulus1;
    case YoungMatrix:
        return card.youngsModulus2;
    case Poisson12:
        return card.poissonRatio12;
    case Shear12:
        return card.shearModulus12;
    case ShearTransverse1: // a blank transverse shear modulus is infinite
        return card.shearModulus1Z.value_or(std::numeric_limits<double>::infinity());
    case ShearTransverse2:
        return card.shearModulus2Z.value_or(std::numeric_limits<double>::infinity());
    case Density:
        return card.density;
    case Expansion1:
        return card.thermalExpansion1;
    case Expansion2:
        return card.thermalExpansion2;
    case ReferenceTemperature:
        return card.referenceTemperature;
    case TensionFiber:
        return card.tensionStrength1;
    case CompressionFiber:
        return card.compressionStrength1;
    case TensionMatrix:
        return card.tensionStrength2;
    case CompressionMatrix:
        return card.compressionStrength2;
    case ShearLimit:
        return card.shearStrength;
    case Damping:
        return card.structuralDamping;
    case Interaction12:
        return card.interactionTerm12;
    default:
        return card.strainAllowables;
    }
}

void writeFields(OrthoMat2DCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case YoungFiber:
            card.youngsModulus1 = item.value;
            break;
        case YoungMatrix:
            card.youngsModulus2 = item.value;
            break;
        case Poisson12:
            card.poissonRatio12 = item.value;
            break;
        case Shear12:
            card.shearModulus12 = item.value;
            break;
        case ShearTransverse1:
            card.shearModulus1Z = item.value;
            break;
        case ShearTransverse2:
            card.shearModulus2Z = item.value;
            break;
        case Density:
            card.density = item.value;
            break;
        case Expansion1:
            card.thermalExpansion1 = item.value;
            break;
        case Expansion2:
            card.thermalExpansion2 = item.value;
            break;
        case ReferenceTemperature:
            card.referenceTemperature = item.value;
            break;
        case TensionFiber:
            card.tensionStrength1 = item.value;
            break;
        case CompressionFiber:
            card.compressionStrength1 = item.value;
            break;
        case TensionMatrix:
            card.tensionStrength2 = item.value;
            break;
        case CompressionMatrix:
            card.compressionStrength2 = item.value;
            break;
        case ShearLimit:
            card.shearStrength = item.value;
            break;
        case Damping:
            card.structuralDamping = item.value;
            break;
        case Interaction12:
            card.interactionTerm12 = item.value;
            break;
        default:
            card.strainAllowables = item.value;
            break;
        }
    }
}

// The contract columns have the names of the fields, in lower case.
DesignColumn columnOf(const OrthoMat2DCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case YoungFiber:
        return {"youngsModulus1"};
    case YoungMatrix:
        return {"youngsModulus2"};
    case Poisson12:
        return {"poissonRatio12"};
    case Shear12:
        return {"shearModulus12"};
    case ShearTransverse1:
        return {"shearModulus1Z"};
    case ShearTransverse2:
        return {"shearModulus2Z"};
    case Density:
        return {"density"};
    case Expansion1:
        return {"thermalExpansion1"};
    case Expansion2:
        return {"thermalExpansion2"};
    case ReferenceTemperature:
        return {"referenceTemperature"};
    case TensionFiber:
        return {"tensionStrength1"};
    case CompressionFiber:
        return {"compressionStrength1"};
    case TensionMatrix:
        return {"tensionStrength2"};
    case CompressionMatrix:
        return {"compressionStrength2"};
    case ShearLimit:
        return {"shearStrength"};
    case Damping:
        return {"structuralDamping"};
    case Interaction12:
        return {"interactionTerm12"};
    default:
        return {"strainAllowables"};
    }
}

} // namespace

const DesignableCard& orthoMat2DDesignFields()
{
    static const TableDesignFields<OrthoMat2DTable> FIELDS("OrthoMat2D",
                                                           {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
