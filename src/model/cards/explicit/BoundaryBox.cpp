#include "oor/model/cards/explicit/BoundaryBox.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The range of one axis from its two fields: a MIN left blank is the lowest value, a MAX left blank is the MIN when the
// MIN is given and the highest value when it is not.
void resolveAxis(const std::optional<double>& givenMin, const std::optional<double>& givenMax, double& low,
                 double& high)
{
    low = givenMin.value_or(BoundaryBox::LOWEST);
    high = givenMax.value_or(givenMin.value_or(BoundaryBox::HIGHEST));
}

std::optional<CardProblem> checkAxis(double low, double high, const char* lowName, const char* highName)
{
    if (const auto problem = checkReal(low, lowName, RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(high, highName, RealRule::Finite))
        return problem;
    if (high < low)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed(lowName),
                           highName};
    return std::nullopt;
}

} // namespace

BoundaryBox resolveBoundaryBox(const BoundaryBoxFields& fields) noexcept
{
    BoundaryBox box;
    resolveAxis(fields.minX, fields.maxX, box.minX, box.maxX);
    resolveAxis(fields.minY, fields.maxY, box.minY, box.maxY);
    resolveAxis(fields.minZ, fields.maxZ, box.minZ, box.maxZ);
    return box;
}

BoundaryBox readBoundaryBox(FieldReader& reader, int firstField)
{
    BoundaryBoxFields fields;
    fields.minX = reader.optionalReal(firstField, "minX");
    fields.maxX = reader.optionalReal(firstField + 1, "maxX");
    fields.minY = reader.optionalReal(firstField + 2, "minY");
    fields.maxY = reader.optionalReal(firstField + 3, "maxY");
    fields.minZ = reader.optionalReal(firstField + 4, "minZ");
    fields.maxZ = reader.optionalReal(firstField + 5, "maxZ");
    return resolveBoundaryBox(fields);
}

void writeBoundaryBox(FieldWriter& writer, int firstField, const BoundaryBox& box)
{
    if (writer.omitsDefaults() && box == BoundaryBox{})
        return;
    writer.real(firstField, box.minX);
    writer.real(firstField + 1, box.maxX);
    writer.real(firstField + 2, box.minY);
    writer.real(firstField + 3, box.maxY);
    writer.real(firstField + 4, box.minZ);
    writer.real(firstField + 5, box.maxZ);
}

std::optional<CardProblem> checkBoundaryBox(const BoundaryBox& box)
{
    if (const auto problem = checkAxis(box.minX, box.maxX, "minX", "maxX"))
        return problem;
    if (const auto problem = checkAxis(box.minY, box.maxY, "minY", "maxY"))
        return problem;
    return checkAxis(box.minZ, box.maxZ, "minZ", "maxZ");
}

} // namespace oor::model
