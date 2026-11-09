#include "oor/model/cards/materials/creepMat/CreepMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr const char* COEFFICIENT_NAMES[7] = {"coefficientA", "coefficientB", "coefficientC", "coefficientD",
                                              "coefficientE", "coefficientF", "coefficientG"};

bool isLawType(int type)
{
    switch (type)
    {
    case 111:
    case 112:
    case 121:
    case 122:
    case 211:
    case 212:
    case 221:
    case 222:
    case 300:
        return true;
    default:
        return false;
    }
}

std::array<double, 7> blankCoefficients()
{
    return std::array<double, 7>{CreepMatCard::BLANK, CreepMatCard::BLANK, CreepMatCard::BLANK, CreepMatCard::BLANK,
                                 CreepMatCard::BLANK, CreepMatCard::BLANK, CreepMatCard::BLANK};
}

} // namespace

const char* CreepMatFormat::formWord(CreepMatForm form) noexcept
{
    switch (form)
    {
    case CreepMatForm::CrLaw:
        return "CreepLaw";
    case CreepMatForm::Table:
        return "Table";
    case CreepMatForm::Blank:
        break;
    }
    return "";
}

std::optional<CreepMatForm> CreepMatFormat::parseForm(std::string_view text) noexcept
{
    if ((text == "CreepLaw"))
        return CreepMatForm::CrLaw;
    if ((text == "Table"))
        return CreepMatForm::Table;
    return std::nullopt;
}

CreepMatTable::CreepMatTable()
    : m_exponent(CreepMatCard::DEFAULT_EXPONENT),
      m_threshold(CreepMatCard::DEFAULT_THRESHOLD),
      m_coefficients(blankCoefficients())
{
}

std::optional<CreepMatCard> CreepMatFormat::read(FieldReader& reader)
{
    CreepMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceTemperature = reader.real(2, "referenceTemperature", 0.0);
    card.exponent = reader.real(3, "exponent", CreepMatCard::DEFAULT_EXPONENT);
    const std::string_view word = reader.text(4);
    if (!word.empty())
    {
        if (const auto form = parseForm(word))
            card.form = *form;
        else
            reader.error("card.bad-value",
                         std::string("form") + " must be CRLAW or TABLE, found \"" + std::string(word) + "\"", "form");
    }
    card.primaryStiffnessTableId = reader.integer(5, "primaryStiffnessTableId", 0);
    card.primaryDampingTableId = reader.integer(6, "primaryDampingTableId", 0);
    card.secondaryDampingTableId = reader.integer(7, "secondaryDampingTableId", 0);
    card.threshold = reader.real(8, "threshold", CreepMatCard::DEFAULT_THRESHOLD);
    card.kind = reader.integer(9, "kind", 0);
    for (std::size_t index = 0; index < 7; ++index)
    {
        const std::optional<double> value = reader.optionalReal(10 + static_cast<int>(index), COEFFICIENT_NAMES[index]);
        card.coefficients[index] = value.value_or(CreepMatCard::BLANK);
    }
    requireBlankFields(reader, {}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CreepMatFormat::check(const CreepMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.exponent > 0.0 && card.exponent < 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 and less than 1.0", "exponent"};
    if (!(card.threshold > 0.0 && card.threshold < 1.0e-3))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 and less than 1.0E-3",
                           "threshold"};
    if (!std::isfinite(card.referenceTemperature))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "referenceTemperature"};
    if (const auto problem = checkTableReference(card.primaryStiffnessTableId, "primaryStiffnessTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.primaryDampingTableId, "primaryDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.secondaryDampingTableId, "secondaryDampingTableId"))
        return problem;
    bool anyCoefficient = false;
    for (const double value : card.coefficients)
        anyCoefficient = anyCoefficient || !std::isnan(value);
    if (card.form == CreepMatForm::Table)
    {
        // The parameters come from three tables and the second line stays blank.
        if (card.primaryStiffnessTableId <= 0)
            return CardProblem{
                "card.missing-field",
                std::string(CardProblem::FIELD) + " is required for " + CardProblem::fieldNamed("form") + " = TABLE",
                "primaryStiffnessTableId"};
        if (card.primaryDampingTableId <= 0)
            return CardProblem{
                "card.missing-field",
                std::string(CardProblem::FIELD) + " is required for " + CardProblem::fieldNamed("form") + " = TABLE",
                "primaryDampingTableId"};
        if (card.secondaryDampingTableId <= 0)
            return CardProblem{
                "card.missing-field",
                std::string(CardProblem::FIELD) + " is required for " + CardProblem::fieldNamed("form") + " = TABLE",
                "secondaryDampingTableId"};
        if (card.kind != 0 || anyCoefficient)
            return CardProblem{"card.bad-value",
                               "the creep law (" + std::string(CardProblem::FIELD) + " and " +
                                   CardProblem::fieldNamed("coefficientA") + " to " +
                                   CardProblem::fieldNamed("coefficientG") + ") must be blank for " +
                                   CardProblem::fieldNamed("form") + " = TABLE",
                               "kind"};
        return std::nullopt;
    }
    // An empirical creep law.
    if (card.primaryStiffnessTableId != 0 || card.primaryDampingTableId != 0 || card.secondaryDampingTableId != 0)
        return CardProblem{"card.bad-value",
                           "the tables " + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("primaryDampingTableId") + " and " +
                               CardProblem::fieldNamed("secondaryDampingTableId") + " belong to " +
                               CardProblem::fieldNamed("form") + " = TABLE",
                           "primaryStiffnessTableId"};
    if (!isLawType(card.kind))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 111, 112, 121, 122, 211, 212, 221, 222 or 300",
                           "kind"};
    // g is blank for the types ending in 2, and c, e, f and g are blank for 300.
    const bool endsInTwo = card.kind != 300 && card.kind % 10 == 2;
    if ((endsInTwo || card.kind == 300) && !std::isnan(card.coefficients[6]))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be blank for this " + CardProblem::fieldNamed("kind"),
            "coefficientG"};
    if (card.kind == 300)
    {
        for (const std::size_t index : {std::size_t{2}, std::size_t{4}, std::size_t{5}})
        {
            if (!std::isnan(card.coefficients[index]))
                return CardProblem{
                    "card.bad-value",
                    std::string(CardProblem::FIELD) + " must be blank for " + CardProblem::fieldNamed("kind") + " 300",
                    COEFFICIENT_NAMES[index]};
        }
    }
    return std::nullopt;
}

void CreepMatFormat::write(const CreepMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(3, card.exponent, CreepMatCard::DEFAULT_EXPONENT);
    if (card.form != CreepMatForm::Blank)
        writer.text(4, formWord(card.form));
    writer.integerUnlessDefault(5, card.primaryStiffnessTableId, 0);
    writer.integerUnlessDefault(6, card.primaryDampingTableId, 0);
    writer.integerUnlessDefault(7, card.secondaryDampingTableId, 0);
    writer.realUnlessDefault(8, card.threshold, CreepMatCard::DEFAULT_THRESHOLD);
    writer.integerUnlessDefault(9, card.kind, 0);
    for (std::size_t index = 0; index < 7; ++index)
    {
        if (!std::isnan(card.coefficients[index]))
            writer.real(10 + static_cast<int>(index), card.coefficients[index]);
    }
}

std::uint32_t CreepMatTable::append(const CreepMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_referenceTemperature.push(card.referenceTemperature);
    m_exponent.push(card.exponent);
    m_form.push(static_cast<std::uint8_t>(card.form));
    m_primaryStiffnessTableId.push(card.primaryStiffnessTableId);
    m_primaryDampingTableId.push(card.primaryDampingTableId);
    m_secondaryDampingTableId.push(card.secondaryDampingTableId);
    m_threshold.push(card.threshold);
    m_kind.push(card.kind);
    m_coefficients.push(card.coefficients);
    return row;
}

CreepMatCard CreepMatTable::card(std::uint32_t row) const
{
    CreepMatCard card;
    card.id = m_id.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.exponent = m_exponent.get(row);
    card.form = static_cast<CreepMatForm>(m_form.get(row));
    card.primaryStiffnessTableId = m_primaryStiffnessTableId.get(row);
    card.primaryDampingTableId = m_primaryDampingTableId.get(row);
    card.secondaryDampingTableId = m_secondaryDampingTableId.get(row);
    card.threshold = m_threshold.get(row);
    card.kind = m_kind.get(row);
    card.coefficients = m_coefficients.get(row);
    return card;
}

void CreepMatTable::store(std::uint32_t row, const CreepMatCard& card)
{
    m_id.set(row, card.id);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_exponent.set(row, card.exponent);
    m_form.set(row, static_cast<std::uint8_t>(card.form));
    m_primaryStiffnessTableId.set(row, card.primaryStiffnessTableId);
    m_primaryDampingTableId.set(row, card.primaryDampingTableId);
    m_secondaryDampingTableId.set(row, card.secondaryDampingTableId);
    m_threshold.set(row, card.threshold);
    m_kind.set(row, card.kind);
    m_coefficients.set(row, card.coefficients);
}

void CreepMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_exponent.keepRows(rows);
    m_form.keepRows(rows);
    m_primaryStiffnessTableId.keepRows(rows);
    m_primaryDampingTableId.keepRows(rows);
    m_secondaryDampingTableId.keepRows(rows);
    m_threshold.keepRows(rows);
    m_kind.keepRows(rows);
    m_coefficients.keepRows(rows);
}

void CreepMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1, MAT2 or MAT9 that the card extends; the tables are TABLES1 cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId id = m_primaryStiffnessTableId.get(row); id > 0)
        m_primaryStiffnessTableId.set(row, visitor.visit("primaryStiffnessTableId", ReferenceTarget::StressTable, id));
    if (const base::EntityId id = m_primaryDampingTableId.get(row); id > 0)
        m_primaryDampingTableId.set(row, visitor.visit("primaryDampingTableId", ReferenceTarget::StressTable, id));
    if (const base::EntityId id = m_secondaryDampingTableId.get(row); id > 0)
        m_secondaryDampingTableId.set(row, visitor.visit("secondaryDampingTableId", ReferenceTarget::StressTable, id));
}

} // namespace oor::model
