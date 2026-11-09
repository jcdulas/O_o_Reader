#include "oor/model/cards/properties/barShapeProp/BarShapePropDesignFields.hpp"

#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum BarShapePropKind : std::int32_t
{
    Dimension, // `index` is the number of the dimension minus one
    NonstructuralMass,
};

DesignFieldLookup foundDimension(const BarShapePropCard& card, int index, const DesignField& field)
{
    if (index < 0 || static_cast<std::size_t>(index) >= card.dims.size())
        return DesignFieldLookup::refused("a BarShapeProp of type " + card.kind + " has " +
                                          std::to_string(card.dims.size()) +
                                          " dimensions: " + describeDesignField(field) + " is not one of them");
    return DesignFieldLookup::found(designFacts(Dimension, "dimension(" + std::to_string(index + 1) + ")",
                                                DesignLowerLimit::Positive, DesignMassEffect::Nonlinear, index));
}

DesignFieldLookup foundNonstructuralMass()
{
    return DesignFieldLookup::found(
        designFacts(NonstructuralMass, "nonStructMass", DesignLowerLimit::Positive, DesignMassEffect::Linear));
}

DesignFieldLookup findField(const BarShapePropCard& card, const DesignField& field)
{
    if (field.name == "nonStructMass")
        return foundNonstructuralMass();
    const NumberedName named = splitTrailingNumber(field.name);
    if (named.word == "dimension" && named.number >= 1)
        return foundDimension(card, named.number - 1, field);
    return DesignFieldLookup::refused(
        refusalOfField("BarShapeProp", field, "dimension(1), dimension(2), ... and nonStructMass"));
}

double readField(const BarShapePropCard& card, const DesignFieldFacts& facts)
{
    if (facts.kind == Dimension)
        return card.dims[static_cast<std::size_t>(facts.index)];
    return card.nonStructMass;
}

void writeFields(BarShapePropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        if (item.facts->kind == Dimension)
            card.dims[static_cast<std::size_t>(item.facts->index)] = item.value;
        else
            card.nonStructMass = item.value;
    }
}

// The contract columns: the list dims (DIM1, DIM2, ...) and nsm.
DesignColumn columnOf(const BarShapePropCard&, const DesignFieldFacts& facts)
{
    if (facts.kind == Dimension)
        return {"dims", facts.index};
    return {"nonStructMass"};
}

} // namespace

const DesignableCard& barShapePropDesignFields()
{
    static const TableDesignFields<BarShapePropTable> FIELDS("BarShapeProp",
                                                             {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
