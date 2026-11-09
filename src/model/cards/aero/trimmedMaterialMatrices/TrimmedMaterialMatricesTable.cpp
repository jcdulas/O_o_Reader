#include "oor/model/cards/aero/trimmedMaterialMatrices/TrimmedMaterialMatricesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<TrimmedMaterialMatricesCard> TrimmedMaterialMatricesFormat::read(FieldReader& reader)
{
    TrimmedMaterialMatricesCard card;
    if (reader.isBlank(1))
        reader.report(CardProblem{"card.missing-field", "{field} is required", "name"});
    card.name = std::string(reader.text(1));
    card.mappedDataUnit = reader.requiredInteger(2, "mappedDataUnit");
    card.propertyMatrixUnit = reader.requiredInteger(3, "propertyMatrixUnit");
    card.scaleReal = reader.real(4, "scaleReal", TrimmedMaterialMatricesCard::DEFAULT_SCALE_REAL);
    card.scaleImaginary = reader.real(5, "scaleImaginary", TrimmedMaterialMatricesCard::DEFAULT_SCALE_IMAGINARY);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has five fields: {field:name}, {field:mappedDataUnit}, "
                                      "{field:propertyMatrixUnit}, {field:scaleReal} and {field:scaleImaginary}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TrimmedMaterialMatricesFormat::check(const TrimmedMaterialMatricesCard& card)
{
    if (!isMatrixName(card.name))
        return CardProblem{"card.bad-value", "{field} must be a nonempty matrix name without control characters",
                           "name"};
    if (const auto problem = checkRequiredId(card.mappedDataUnit, "mappedDataUnit"))
        return problem;
    if (const auto problem = checkRequiredId(card.propertyMatrixUnit, "propertyMatrixUnit"))
        return problem;
    // Each unit number is assigned once, and the two files are different ones.
    if (card.mappedDataUnit == card.propertyMatrixUnit)
        return CardProblem{"card.bad-value", "{field:mappedDataUnit} and {field} must be different unit numbers",
                           "propertyMatrixUnit"};
    if (const auto problem = checkReal(card.scaleReal, "scaleReal", RealRule::Finite))
        return problem;
    return checkReal(card.scaleImaginary, "scaleImaginary", RealRule::Finite);
}

void TrimmedMaterialMatricesFormat::write(const TrimmedMaterialMatricesCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.integer(2, card.mappedDataUnit);
    writer.integer(3, card.propertyMatrixUnit);
    writer.realUnlessDefault(4, card.scaleReal, TrimmedMaterialMatricesCard::DEFAULT_SCALE_REAL);
    writer.realUnlessDefault(5, card.scaleImaginary, TrimmedMaterialMatricesCard::DEFAULT_SCALE_IMAGINARY);
}

std::uint32_t TrimmedMaterialMatricesTable::append(const TrimmedMaterialMatricesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void TrimmedMaterialMatricesTable::store(std::uint32_t row, const TrimmedMaterialMatricesCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void TrimmedMaterialMatricesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
