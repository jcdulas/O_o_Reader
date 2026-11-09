#include "oor/model/cards/properties/barProp/BarPropDesignFields.hpp"

#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

#include <array>

namespace oor::model
{

namespace
{

enum BarPropKind : std::int32_t
{
    Area,
    MomentOfInertia1,
    MomentOfInertia2,
    Torsion,
    NonstructuralMass,
    StressCoefficient, // C1, C2, D1, D2, E1, E2, F1, F2: `index` 0 to 7
    ShearFactor1,
    ShearFactor2,
    ProductOfInertia,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "area, bendingInertia1, bendingInertia2, torsionConstant, nonStructMass, "
    "stressPointCy, stressPointCz, stressPointDy, stressPointDz, "
    "stressPointEy, stressPointEz, stressPointFy, stressPointFz, shearFactor1, "
    "shearFactor2, productInertia12";
constexpr const char* STRESS_NAMES[8] = {"stressPointCy", "stressPointCz", "stressPointDy", "stressPointDz",
                                         "stressPointEy", "stressPointEz", "stressPointFy", "stressPointFz"};

DesignFieldLookup foundKind(std::int32_t kind, std::int32_t index = 0)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    switch (kind)
    {
    case Area:
        return DesignFieldLookup::found(designFacts(kind, "area", Positive, Linear));
    case MomentOfInertia1:
        return DesignFieldLookup::found(designFacts(kind, "bendingInertia1", Positive));
    case MomentOfInertia2:
        return DesignFieldLookup::found(designFacts(kind, "bendingInertia2", Positive));
    case Torsion:
        return DesignFieldLookup::found(designFacts(kind, "torsionConstant", Positive));
    case NonstructuralMass:
        return DesignFieldLookup::found(designFacts(kind, "nonStructMass", Positive, Linear));
    case StressCoefficient:
        return DesignFieldLookup::found(designFacts(kind, STRESS_NAMES[index], StressRecovery, None, index));
    case ShearFactor1:
        return DesignFieldLookup::found(designFacts(kind, "shearFactor1", Positive));
    case ShearFactor2:
        return DesignFieldLookup::found(designFacts(kind, "shearFactor2", Positive));
    default:
        return DesignFieldLookup::found(designFacts(kind, "productInertia12", Signed));
    }
}

// The field of a position on the card: the name of the card is field 1, so PID is 2, MID 3, A 4, I1 5, I2 6, J 7, NSM
// 8; the second line holds C1 to F2 in 12 to 19, the third K1, K2 and I12 in 22, 23 and 24.

// A negative FID is the word of the element property table of the solver: PID 1, MID 2, A 3, I1 4, I2 5, J 6, NSM 7, a
// word of its own for the stress recovery flag 8, C1 to F2 in 9 to 16, K1 17, K2 18, I12 19 (the format reference: the
// area of a PBAR is FID -3).

DesignFieldLookup findField(const BarPropCard&, const DesignField& field)
{
    if (field.name == "area")
        return foundKind(Area);
    if (field.name == "bendingInertia1")
        return foundKind(MomentOfInertia1);
    if (field.name == "bendingInertia2")
        return foundKind(MomentOfInertia2);
    if (field.name == "torsionConstant")
        return foundKind(Torsion);
    if (field.name == "nonStructMass")
        return foundKind(NonstructuralMass);
    if (field.name == "shearFactor1")
        return foundKind(ShearFactor1);
    if (field.name == "shearFactor2")
        return foundKind(ShearFactor2);
    if (field.name == "productInertia12")
        return foundKind(ProductOfInertia);
    for (int index = 0; index < 8; ++index)
    {
        if (field.name == STRESS_NAMES[index])
            return foundKind(StressCoefficient, index);
    }
    return DesignFieldLookup::refused(refusalOfField("BarProp", field, DESIGNABLE));
}

double& stressCoefficient(BarPropCard& card, int index)
{
    std::array<double, 2>* const pairs[4] = {&card.stressPointC, &card.stressPointD, &card.stressPointE,
                                             &card.stressPointF};
    return (*pairs[index / 2])[static_cast<std::size_t>(index % 2)];
}

double stressCoefficientOf(const BarPropCard& card, int index)
{
    const std::array<double, 2>* const pairs[4] = {&card.stressPointC, &card.stressPointD, &card.stressPointE,
                                                   &card.stressPointF};
    return (*pairs[index / 2])[static_cast<std::size_t>(index % 2)];
}

double readField(const BarPropCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Area:
        return card.area;
    case MomentOfInertia1:
        return card.bendingInertia1;
    case MomentOfInertia2:
        return card.bendingInertia2;
    case Torsion:
        return card.torsionConstant;
    case NonstructuralMass:
        return card.nonStructMass;
    case StressCoefficient:
        return stressCoefficientOf(card, facts.index);
    case ShearFactor1:
        return card.shearFactor1;
    case ShearFactor2:
        return card.shearFactor2;
    default:
        return card.productInertia12;
    }
}

void writeFields(BarPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Area:
            card.area = item.value;
            break;
        case MomentOfInertia1:
            card.bendingInertia1 = item.value;
            break;
        case MomentOfInertia2:
            card.bendingInertia2 = item.value;
            break;
        case Torsion:
            card.torsionConstant = item.value;
            break;
        case NonstructuralMass:
            card.nonStructMass = item.value;
            break;
        case StressCoefficient:
            stressCoefficient(card, item.facts->index) = item.value;
            break;
        case ShearFactor1:
            card.shearFactor1 = item.value;
            break;
        case ShearFactor2:
            card.shearFactor2 = item.value;
            break;
        default:
            card.productInertia12 = item.value;
            break;
        }
    }
}

// The contract columns: a, i1, i2, j, nsm, i12, the pairs c, d, e, f (C1 C2, D1 D2, ...) and the pair k (K1 K2).
DesignColumn columnOf(const BarPropCard&, const DesignFieldFacts& facts)
{
    static constexpr const char* PAIRS[4] = {"stressPointC", "stressPointD", "stressPointE", "stressPointF"};
    switch (facts.kind)
    {
    case Area:
        return {"area"};
    case MomentOfInertia1:
        return {"bendingInertia1"};
    case MomentOfInertia2:
        return {"bendingInertia2"};
    case Torsion:
        return {"torsionConstant"};
    case NonstructuralMass:
        return {"nonStructMass"};
    case StressCoefficient:
        return {PAIRS[facts.index / 2], facts.index % 2};
    case ShearFactor1:
        return {"shearFactor", 0};
    case ShearFactor2:
        return {"shearFactor", 1};
    default:
        return {"productInertia12"};
    }
}

} // namespace

const DesignableCard& barPropDesignFields()
{
    static const TableDesignFields<BarPropTable> FIELDS("BarProp", {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
