#include "oor/model/cards/loads/acousticPressureLoadMatrices/AcousticPressureLoadMatricesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr double DEFAULT_SCALE_REAL = 1.0;
constexpr double DEFAULT_SCALE_IMAGINARY = 0.0;
constexpr base::EntityId DEFAULT_FILE_LOAD_CASE_ID = 1;

} // namespace

AcousticPressureLoadMatricesTable::AcousticPressureLoadMatricesTable()
    : m_scaleReal(DEFAULT_SCALE_REAL),
      m_scaleImaginary(DEFAULT_SCALE_IMAGINARY),
      m_fileLoadCaseId(DEFAULT_FILE_LOAD_CASE_ID)
{
}

std::optional<AcousticPressureLoadMatricesCard> AcousticPressureLoadMatricesFormat::read(FieldReader& reader)
{
    AcousticPressureLoadMatricesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dataFileUnit = reader.requiredInteger(2, "dataFileUnit");
    card.propertyFileUnit = reader.requiredInteger(3, "propertyFileUnit");
    card.scaleReal = reader.real(4, "scaleReal", DEFAULT_SCALE_REAL);
    card.scaleImaginary = reader.real(5, "scaleImaginary", DEFAULT_SCALE_IMAGINARY);
    card.fileLoadCaseId = reader.integer(6, "fileLoadCaseId", DEFAULT_FILE_LOAD_CASE_ID);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("fileLoadCaseId"),
                     "fileLoadCaseId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticPressureLoadMatricesFormat::check(const AcousticPressureLoadMatricesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.dataFileUnit, "dataFileUnit"))
        return problem;
    if (const auto problem = checkRequiredId(card.propertyFileUnit, "propertyFileUnit"))
        return problem;
    if (const auto problem = checkReal(card.scaleReal, "scaleReal", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.scaleImaginary, "scaleImaginary", RealRule::Finite))
        return problem;
    return checkRequiredId(card.fileLoadCaseId, "fileLoadCaseId");
}

void AcousticPressureLoadMatricesFormat::write(const AcousticPressureLoadMatricesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.dataFileUnit);
    writer.integer(3, card.propertyFileUnit);
    writer.realUnlessDefault(4, card.scaleReal, DEFAULT_SCALE_REAL);
    writer.realUnlessDefault(5, card.scaleImaginary, DEFAULT_SCALE_IMAGINARY);
    writer.integerUnlessDefault(6, card.fileLoadCaseId, DEFAULT_FILE_LOAD_CASE_ID);
}

std::uint32_t AcousticPressureLoadMatricesTable::append(const AcousticPressureLoadMatricesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_dataFileUnit.push(card.dataFileUnit);
    m_propertyFileUnit.push(card.propertyFileUnit);
    m_scaleReal.push(card.scaleReal);
    m_scaleImaginary.push(card.scaleImaginary);
    m_fileLoadCaseId.push(card.fileLoadCaseId);
    return row;
}

AcousticPressureLoadMatricesCard AcousticPressureLoadMatricesTable::card(std::uint32_t row) const
{
    AcousticPressureLoadMatricesCard card;
    card.id = m_id.get(row);
    card.dataFileUnit = m_dataFileUnit.get(row);
    card.propertyFileUnit = m_propertyFileUnit.get(row);
    card.scaleReal = m_scaleReal.get(row);
    card.scaleImaginary = m_scaleImaginary.get(row);
    card.fileLoadCaseId = m_fileLoadCaseId.get(row);
    return card;
}

void AcousticPressureLoadMatricesTable::store(std::uint32_t row, const AcousticPressureLoadMatricesCard& card)
{
    m_id.set(row, card.id);
    m_dataFileUnit.set(row, card.dataFileUnit);
    m_propertyFileUnit.set(row, card.propertyFileUnit);
    m_scaleReal.set(row, card.scaleReal);
    m_scaleImaginary.set(row, card.scaleImaginary);
    m_fileLoadCaseId.set(row, card.fileLoadCaseId);
}

void AcousticPressureLoadMatricesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_dataFileUnit.keepRows(rows);
    m_propertyFileUnit.keepRows(rows);
    m_scaleReal.keepRows(rows);
    m_scaleImaginary.keepRows(rows);
    m_fileLoadCaseId.keepRows(rows);
}

} // namespace oor::model
