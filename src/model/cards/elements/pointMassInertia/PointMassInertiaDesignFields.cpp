#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaDesignFields.hpp"

#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum PointMassInertiaKind : std::int32_t
{
    Mass,
    Offset,  // X1, X2, X3: `index` 0 to 2
    Inertia, // I11, I21, I22, I31, I32, I33: `index` 0 to 5
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "mass, offset1, offset2, offset3, inertia11, inertia21, "
    "inertia22, inertia31, inertia32, inertia33";
constexpr const char* INERTIA_NAMES[6] = {"inertia11", "inertia21", "inertia22", "inertia31", "inertia32", "inertia33"};

DesignFieldLookup findField(const PointMassInertiaCard&, const DesignField& field)
{
    using enum DesignLowerLimit;
    using enum DesignMassEffect;
    if (field.name == "mass")
        return DesignFieldLookup::found(designFacts(Mass, "mass", Positive, Linear));
    if (field.name == "offset1" || field.name == "offset2" || field.name == "offset3")
    {
        const int index = field.name.back() - '1';
        return DesignFieldLookup::found(designFacts(Offset, field.name, Signed, None, index));
    }
    for (int index = 0; index < 6; ++index)
    {
        if (field.name != INERTIA_NAMES[index])
            continue;
        // I11, I22 and I33 are moments of inertia, which cannot be negative; the others are products.
        const bool isMoment = index == 0 || index == 2 || index == 5;
        return DesignFieldLookup::found(designFacts(Inertia, field.name, isMoment ? Positive : Signed, None, index));
    }
    return DesignFieldLookup::refused(refusalOfField("PointMassInertia", field, DESIGNABLE));
}

double readField(const PointMassInertiaCard& card, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Mass:
        return card.mass;
    case Offset:
        return card.offset[static_cast<std::size_t>(facts.index)];
    default:
        return card.inertia[static_cast<std::size_t>(facts.index)];
    }
}

void writeFields(PointMassInertiaCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        switch (item.facts->kind)
        {
        case Mass:
            card.mass = item.value;
            break;
        case Offset:
            card.offset[static_cast<std::size_t>(item.facts->index)] = item.value;
            break;
        default:
            card.inertia[static_cast<std::size_t>(item.facts->index)] = item.value;
            break;
        }
    }
}

// The contract columns: mass, and the vectors offset (X1 to X3) and inertia (I11, I21, I22, I31, I32, I33), in the
// order of `index`.
DesignColumn columnOf(const PointMassInertiaCard&, const DesignFieldFacts& facts)
{
    switch (facts.kind)
    {
    case Mass:
        return {"mass"};
    case Offset:
        return {"offset", facts.index};
    default:
        return {"inertia", facts.index};
    }
}

} // namespace

const DesignableCard& pointMassInertiaDesignFields()
{
    static const TableDesignFields<PointMassInertiaTable> FIELDS("PointMassInertia",
                                                           {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
