#include "oor/model/cards/materials/elastoPlasticMatTemperature/ElastoPlasticMatTemperatureTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line
constexpr int FIELD_YIELD_STRESS_TABLE = 3;
constexpr int FIELD_STRESS_STRAIN_CURVES_TABLE = 4;
constexpr int FIELD_HARDENING_SLOPE_TABLE = 8;
// The CHABOCHE keyword is on the line the format reference draws it on, the third; the second is a line of unused
// cells.
constexpr int FIELD_CHABOCHE = 17;

constexpr const char* CHABOCHE_NAMES[ElastoPlasticMatTemperatureCard::CHABOCHE_TABLES] = {
    "chabocheYieldStressTableId",       "saturatedIsotropicHardeningTableId", "isotropicHardeningRateTableId",
    "kinematicHardeningModulusTableId", "kinematicRecallRateTableId",         "viscousCoefficientTableId",
    "viscousExponentTableId",           "memoryHardeningLimitTableId",        "memoryHardeningCoefficientTableId",
    "memoryEvolutionFactorTableId"};

// True when the line that starts at `first` has any field that is not blank.
bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

// The field that holds the table `index` of the CHABOCHE group when the keyword is at `keyword`: the first seven follow
// it, the other three are on the next line after one blank field.
int chabocheField(int keyword, std::size_t index)
{
    if (index < 7)
        return keyword + 1 + static_cast<int>(index);
    return keyword + LINE + 1 + static_cast<int>(index - 7);
}

} // namespace

std::optional<ElastoPlasticMatTemperatureCard> ElastoPlasticMatTemperatureFormat::read(FieldReader& reader)
{
    ElastoPlasticMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.yieldStressTableId = reader.integer(FIELD_YIELD_STRESS_TABLE, "yieldStressTableId", 0);
    card.stressStrainCurvesTableId = reader.integer(FIELD_STRESS_STRAIN_CURVES_TABLE, "stressStrainCurvesTableId", 0);
    card.hardeningSlopeTableId = reader.integer(FIELD_HARDENING_SLOPE_TABLE, "hardeningSlopeTableId", 0);
    for (const int field : {2, 5, 6, 7})
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "id");
            return card;
        }
    }

    // The CHABOCHE group: its keyword starts the line, on the second line or the third (a line of unused cells before
    // it).
    const int last = reader.lastUsedField();
    int keyword = 0;
    for (int line = 9; line <= last && keyword == 0; line += LINE)
    {
        if (!reader.isBlank(line))
            keyword = line;
        else if (lineHasValues(reader, line))
        {
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " after the first starts with the keyword " +
                             std::string("chabocheHardening") + ", or is blank",
                         "");
            return card;
        }
    }
    if (keyword != 0)
    {
        if (!(reader.text(keyword) == "ChabocheHardening"))
        {
            reader.error("card.bad-value",
                         "\"" + std::string(reader.text(keyword)) + "\" is not a group of a " +
                             std::string(reader.cardName()) + ": " + std::string("chabocheHardening"),
                         "");
            return card;
        }
        std::array<base::EntityId, ElastoPlasticMatTemperatureCard::CHABOCHE_TABLES> tables{};
        for (std::size_t index = 0; index < tables.size(); ++index)
            tables[index] = reader.integer(chabocheField(keyword, index), CHABOCHE_NAMES[index], 0);
        // The cells the format reference leaves empty: the first field of the second line, the end of that line, and
        // what follows.
        for (const int field :
             {keyword + LINE, keyword + LINE + 4, keyword + LINE + 5, keyword + LINE + 6, keyword + LINE + 7})
        {
            if (!reader.isBlank(field))
            {
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                                 " is not used and must be blank",
                             "chabocheHardening");
                return card;
            }
        }
        if (reader.lastUsedField() > keyword + 2 * LINE - 1)
        {
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " has no field after the second line of the " +
                             std::string("chabocheHardening") + " group",
                         "chabocheHardening");
            return card;
        }
        card.chabocheHardening = tables;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ElastoPlasticMatTemperatureFormat::check(const ElastoPlasticMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.yieldStressTableId, "yieldStressTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.stressStrainCurvesTableId, "stressStrainCurvesTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.hardeningSlopeTableId, "hardeningSlopeTableId"))
        return problem;
    // T(FID) is used alone; SOL 400 blanks T(Y0) and T(H) when it is given.
    if (card.stressStrainCurvesTableId != 0 && (card.yieldStressTableId != 0 || card.hardeningSlopeTableId != 0))
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("yieldStressTableId") + " and " +
                               CardProblem::fieldNamed("hardeningSlopeTableId") + " are blank when " +
                               std::string(CardProblem::FIELD) + " is given",
                           "stressStrainCurvesTableId"};
    if (card.chabocheHardening)
    {
        for (std::size_t index = 0; index < card.chabocheHardening->size(); ++index)
        {
            if (const auto problem = checkTableReference((*card.chabocheHardening)[index], CHABOCHE_NAMES[index]))
                return problem;
        }
    }
    return std::nullopt;
}

void ElastoPlasticMatTemperatureFormat::write(const ElastoPlasticMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(FIELD_YIELD_STRESS_TABLE, card.yieldStressTableId, 0);
    writer.integerUnlessDefault(FIELD_STRESS_STRAIN_CURVES_TABLE, card.stressStrainCurvesTableId, 0);
    writer.integerUnlessDefault(FIELD_HARDENING_SLOPE_TABLE, card.hardeningSlopeTableId, 0);
    if (!card.chabocheHardening)
        return;
    writer.text(FIELD_CHABOCHE, "ChabocheHardening");
    for (std::size_t index = 0; index < card.chabocheHardening->size(); ++index)
        writer.integerUnlessDefault(chabocheField(FIELD_CHABOCHE, index), (*card.chabocheHardening)[index], 0);
}

std::uint32_t ElastoPlasticMatTemperatureTable::append(const ElastoPlasticMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ElastoPlasticMatTemperatureTable::store(std::uint32_t row, const ElastoPlasticMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ElastoPlasticMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ElastoPlasticMatTemperatureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ElastoPlasticMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ElastoPlasticMatTemperatureCard& card = m_cards.edit(row);
    // The ID is the one of the MATEP that the card extends; the tables are TABLEMi and TABLEST cards.
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    if (card.yieldStressTableId != 0)
        card.yieldStressTableId =
            visitor.visit("yieldStressTableId", ReferenceTarget::MaterialTable, card.yieldStressTableId);
    if (card.stressStrainCurvesTableId != 0)
        card.stressStrainCurvesTableId =
            visitor.visit("stressStrainCurvesTableId", ReferenceTarget::StressTable, card.stressStrainCurvesTableId);
    if (card.hardeningSlopeTableId != 0)
        card.hardeningSlopeTableId =
            visitor.visit("hardeningSlopeTableId", ReferenceTarget::MaterialTable, card.hardeningSlopeTableId);
    if (card.chabocheHardening)
    {
        for (base::EntityId& table : *card.chabocheHardening)
        {
            if (table != 0)
                table = visitor.visit("chabocheHardening", ReferenceTarget::MaterialTable, table);
        }
    }
}

} // namespace oor::model
