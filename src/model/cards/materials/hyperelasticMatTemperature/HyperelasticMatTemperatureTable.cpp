#include "oor/model/cards/materials/hyperelasticMatTemperature/HyperelasticMatTemperatureTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// Where the tables sit on the continuation lines, in the order the card keeps them.
constexpr int COEFFICIENT_C_FIELDS[20] = {9,  10, 17, 18, 19, 25, 26, 27, 28, 33,
                                          34, 35, 36, 37, 41, 42, 43, 44, 45, 46};
constexpr const char* COEFFICIENT_C_NAMES[20] = {
    "coefficientC10TableId", "coefficientC01TableId", "coefficientC20TableId", "coefficientC11TableId",
    "coefficientC02TableId", "coefficientC30TableId", "coefficientC21TableId", "coefficientC12TableId",
    "coefficientC03TableId", "coefficientC40TableId", "coefficientC31TableId", "coefficientC22TableId",
    "coefficientC13TableId", "coefficientC04TableId", "coefficientC50TableId", "coefficientC41TableId",
    "coefficientC32TableId", "coefficientC23TableId", "coefficientC14TableId", "coefficientC05TableId"};
constexpr int MOONEY_COEFFICIENT_D_FIELDS[5] = {11, 20, 29, 38, 47};
constexpr int OGDEN_COEFFICIENT_D_FIELDS[5] = {9, 10, 11, 12, 13};
constexpr int OGDEN_EXAMPLE_COEFFICIENT_D_FIELDS[5] = {33, 34, 35, 36,
                                                       37}; // where a common deck of the OGDEN form has them
constexpr int BOYCE_COEFFICIENT_D_FIELDS[5] = {17, 18, 19, 20, 21};
constexpr const char* COEFFICIENT_D_NAMES[5] = {"coefficientD1TableId", "coefficientD2TableId", "coefficientD3TableId",
                                                "coefficientD4TableId", "coefficientD5TableId"};
constexpr int RUBBER_MODULUS_FIELD = 9;
constexpr int EXPONENT_OR_MODULUS_FIELD = 10;
constexpr int INVARIANT_LIMIT_FIELD = 11;

bool isOgdenOrFoam(HyperelasticMatTemperatureModel model)
{
    return model == HyperelasticMatTemperatureModel::Ogden || model == HyperelasticMatTemperatureModel::Foam;
}

bool isBoyceOrGent(HyperelasticMatTemperatureModel model)
{
    return model == HyperelasticMatTemperatureModel::Aboyce || model == HyperelasticMatTemperatureModel::Gent;
}

template <std::size_t N>
bool allZero(const std::array<base::EntityId, N>& ids)
{
    for (const base::EntityId id : ids)
    {
        if (id != 0)
            return false;
    }
    return true;
}

// True when one of the fields `first` to `last` has a value.
bool anyGiven(const FieldReader& reader, int first, int last)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

} // namespace

const char* HyperelasticMatTemperatureFormat::modelWord(HyperelasticMatTemperatureModel model) noexcept
{
    switch (model)
    {
    case HyperelasticMatTemperatureModel::Mooney:
        return "MooneyRivlin";
    case HyperelasticMatTemperatureModel::Ogden:
        return "Ogden";
    case HyperelasticMatTemperatureModel::Foam:
        return "Foam";
    case HyperelasticMatTemperatureModel::Aboyce:
        return "ArrudaBoyce";
    case HyperelasticMatTemperatureModel::Gent:
        return "Gent";
    }
    return "MooneyRivlin";
}

std::optional<HyperelasticMatTemperatureModel> HyperelasticMatTemperatureFormat::parseModel(
    std::string_view text) noexcept
{
    for (const HyperelasticMatTemperatureModel model :
         {HyperelasticMatTemperatureModel::Mooney, HyperelasticMatTemperatureModel::Ogden,
          HyperelasticMatTemperatureModel::Foam, HyperelasticMatTemperatureModel::Aboyce,
          HyperelasticMatTemperatureModel::Gent})
    {
        if ((text == modelWord(model)))
            return model;
    }
    return std::nullopt;
}

std::optional<HyperelasticMatTemperatureCard> HyperelasticMatTemperatureFormat::read(FieldReader& reader)
{
    HyperelasticMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
    {
        if (const auto model = parseModel(word))
            card.modelName = *model;
        else
        {
            reader.error("card.bad-value",
                         std::string("modelName") + " must be MOONEY, OGDEN, FOAM, ABOYCE or GENT, not \"" +
                             std::string(word) + "\"",
                         "modelName");
            return card;
        }
    }
    card.bulkModulusTableId = reader.integer(4, "bulkModulusTableId", 0);
    card.densityTableId = reader.integer(5, "densityTableId", 0);
    card.thermalExpansionTableId = reader.integer(6, "thermalExpansionTableId", 0);
    card.structuralDampingTableId = reader.integer(8, "structuralDampingTableId", 0);

    std::vector<int> used = {1, 2, 4, 5, 6, 8};
    if (card.modelName == HyperelasticMatTemperatureModel::Mooney)
    {
        for (std::size_t index = 0; index < card.coefficientCTableIds.size(); ++index)
        {
            card.coefficientCTableIds[index] =
                reader.integer(COEFFICIENT_C_FIELDS[index], COEFFICIENT_C_NAMES[index], 0);
            used.push_back(COEFFICIENT_C_FIELDS[index]);
        }
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
        {
            card.coefficientDTableIds[index] =
                reader.integer(MOONEY_COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0);
            used.push_back(MOONEY_COEFFICIENT_D_FIELDS[index]);
        }
    }
    else if (isOgdenOrFoam(card.modelName))
    {
        const bool inFormat = anyGiven(reader, OGDEN_COEFFICIENT_D_FIELDS[0], OGDEN_COEFFICIENT_D_FIELDS[4]);
        const bool inExample =
            anyGiven(reader, OGDEN_EXAMPLE_COEFFICIENT_D_FIELDS[0], OGDEN_EXAMPLE_COEFFICIENT_D_FIELDS[4]);
        if (inFormat && inExample)
        {
            reader.error("card.bad-value",
                         "the tables " + std::string("coefficientD1TableId") + " to " +
                             std::string("coefficientD5TableId") +
                             " are in fields 9 to 13 or in fields 33 to 37, not both",
                         "coefficientD1TableId");
            return card;
        }
        const int* fields = inExample ? OGDEN_EXAMPLE_COEFFICIENT_D_FIELDS : OGDEN_COEFFICIENT_D_FIELDS;
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
        {
            card.coefficientDTableIds[index] = reader.integer(fields[index], COEFFICIENT_D_NAMES[index], 0);
            used.push_back(fields[index]);
        }
    }
    else
    {
        card.rubberModulusTableId = reader.integer(RUBBER_MODULUS_FIELD, "rubberModulusTableId", 0);
        card.exponentOrModulusTableId = reader.integer(EXPONENT_OR_MODULUS_FIELD, "exponentOrModulusTableId", 0);
        used.push_back(RUBBER_MODULUS_FIELD);
        used.push_back(EXPONENT_OR_MODULUS_FIELD);
        if (card.modelName == HyperelasticMatTemperatureModel::Gent)
        {
            card.invariantLimitTableId = reader.integer(INVARIANT_LIMIT_FIELD, "invariantLimitTableId", 0);
            used.push_back(INVARIANT_LIMIT_FIELD);
        }
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
        {
            card.coefficientDTableIds[index] =
                reader.integer(BOYCE_COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0);
            used.push_back(BOYCE_COEFFICIENT_D_FIELDS[index]);
        }
    }
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HyperelasticMatTemperatureFormat::check(const HyperelasticMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.bulkModulusTableId, "bulkModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansionTableId, "thermalExpansionTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.rubberModulusTableId, "rubberModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.exponentOrModulusTableId, "exponentOrModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.invariantLimitTableId, "invariantLimitTableId"))
        return problem;
    for (std::size_t index = 0; index < card.coefficientCTableIds.size(); ++index)
    {
        if (const auto problem = checkTableReference(card.coefficientCTableIds[index], COEFFICIENT_C_NAMES[index]))
            return problem;
    }
    for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
    {
        if (const auto problem = checkTableReference(card.coefficientDTableIds[index], COEFFICIENT_D_NAMES[index]))
            return problem;
    }
    // A model has the tables of its own coefficients only.
    if (card.modelName != HyperelasticMatTemperatureModel::Mooney && !allZero(card.coefficientCTableIds))
        return CardProblem{
            "card.bad-value",
            "the tables of the Mooney-Rivlin constants Cij need " + std::string(CardProblem::FIELD) + " = MOONEY",
            "modelName"};
    if (!isBoyceOrGent(card.modelName) && (card.rubberModulusTableId != 0 || card.exponentOrModulusTableId != 0))
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("rubberModulusTableId") + " and " +
                               CardProblem::fieldNamed("exponentOrModulusTableId") + " need " +
                               std::string(CardProblem::FIELD) + " = ABOYCE or GENT",
                           "modelName"};
    if (card.modelName != HyperelasticMatTemperatureModel::Gent && card.invariantLimitTableId != 0)
        return CardProblem{
            "card.bad-value",
            CardProblem::fieldNamed("invariantLimitTableId") + " needs " + std::string(CardProblem::FIELD) + " = GENT",
            "modelName"};
    return std::nullopt;
}

void HyperelasticMatTemperatureFormat::write(const HyperelasticMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.modelName != HyperelasticMatTemperatureModel::Mooney)
        writer.text(2, modelWord(card.modelName));
    writer.integerUnlessDefault(4, card.bulkModulusTableId, 0);
    writer.integerUnlessDefault(5, card.densityTableId, 0);
    writer.integerUnlessDefault(6, card.thermalExpansionTableId, 0);
    writer.integerUnlessDefault(8, card.structuralDampingTableId, 0);
    if (card.modelName == HyperelasticMatTemperatureModel::Mooney)
    {
        for (std::size_t index = 0; index < card.coefficientCTableIds.size(); ++index)
            writer.integerUnlessDefault(COEFFICIENT_C_FIELDS[index], card.coefficientCTableIds[index], 0);
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
            writer.integerUnlessDefault(MOONEY_COEFFICIENT_D_FIELDS[index], card.coefficientDTableIds[index], 0);
    }
    else if (isOgdenOrFoam(card.modelName))
    {
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
            writer.integerUnlessDefault(OGDEN_COEFFICIENT_D_FIELDS[index], card.coefficientDTableIds[index], 0);
    }
    else
    {
        writer.integerUnlessDefault(RUBBER_MODULUS_FIELD, card.rubberModulusTableId, 0);
        writer.integerUnlessDefault(EXPONENT_OR_MODULUS_FIELD, card.exponentOrModulusTableId, 0);
        if (card.modelName == HyperelasticMatTemperatureModel::Gent)
            writer.integerUnlessDefault(INVARIANT_LIMIT_FIELD, card.invariantLimitTableId, 0);
        for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
            writer.integerUnlessDefault(BOYCE_COEFFICIENT_D_FIELDS[index], card.coefficientDTableIds[index], 0);
    }
}

std::uint32_t HyperelasticMatTemperatureTable::append(const HyperelasticMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HyperelasticMatTemperatureTable::store(std::uint32_t row, const HyperelasticMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HyperelasticMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HyperelasticMatTemperatureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HyperelasticMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    HyperelasticMatTemperatureCard& card = m_cards.edit(row);
    // The ID is the one of the MATHE that the card extends; the tables are TABLEMi cards.
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::MaterialTable, table);
    };
    visitTable(card.bulkModulusTableId, "bulkModulusTableId");
    visitTable(card.densityTableId, "densityTableId");
    visitTable(card.thermalExpansionTableId, "thermalExpansionTableId");
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
    for (std::size_t index = 0; index < card.coefficientCTableIds.size(); ++index)
        visitTable(card.coefficientCTableIds[index], COEFFICIENT_C_NAMES[index]);
    for (std::size_t index = 0; index < card.coefficientDTableIds.size(); ++index)
        visitTable(card.coefficientDTableIds[index], COEFFICIENT_D_NAMES[index]);
    visitTable(card.rubberModulusTableId, "rubberModulusTableId");
    visitTable(card.exponentOrModulusTableId, "exponentOrModulusTableId");
    visitTable(card.invariantLimitTableId, "invariantLimitTableId");
}

} // namespace oor::model
