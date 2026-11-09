#include "oor/model/cards/elements/shellTria3/ShellTria3DesignFields.hpp"

#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum ShellTria3Kind : std::int32_t
{
    Offset,
    CornerThickness, // T1 to T3: `index` 0 to 2
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE = "offset, cornerThickness1, cornerThickness2, cornerThickness3";

DesignFieldLookup findField(const ShellTria3Card&, const DesignField& field)
{
    if (field.name == "offset")
        return DesignFieldLookup::found(designFacts(Offset, "offset", DesignLowerLimit::Signed));
    if (field.name == "cornerThickness1" || field.name == "cornerThickness2" || field.name == "cornerThickness3")
        return DesignFieldLookup::found(designFacts(CornerThickness, field.name, DesignLowerLimit::Positive,
                                                    DesignMassEffect::Nonlinear, field.name.back() - '1'));
    return DesignFieldLookup::refused(refusalOfField("ShellTria3", field, DESIGNABLE));
}

double readField(const ShellTria3Card& card, const DesignFieldFacts& facts)
{
    return facts.kind == Offset ? card.offset : card.cornerThickness[static_cast<std::size_t>(facts.index)];
}

void writeFields(ShellTria3Card& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        if (item.facts->kind == Offset)
            card.offset = item.value;
        else
            card.cornerThickness[static_cast<std::size_t>(item.facts->index)] = item.value;
    }
}

// The contract columns: offset (ZOFFS), and the vector cornerThickness (T1 to T3).
DesignColumn columnOf(const ShellTria3Card&, const DesignFieldFacts& facts)
{
    if (facts.kind == Offset)
        return {"offset"};
    return {"cornerThickness", facts.index};
}

} // namespace

const DesignableCard& shellTria3DesignFields()
{
    static const TableDesignFields<ShellTria3Table> FIELDS("ShellTria3",
                                                          {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
