#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_FLAG_FIELD = 2;
constexpr int FIELDS_PER_GROUP = 8; // a group is a line: the field before its flag is blank, the flag and six values

// The values of a group of `count` fields behind the flag at `flagField`; the fields after them must be blank.
void requireBlankAfter(FieldReader& reader, int flagField, int count, const char* groupName)
{
    for (int field = flagField + 1 + count; field < flagField + FIELDS_PER_GROUP; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error(
                "card.bad-value",
                std::string("too many values in the ") + groupName + " group of a " + std::string(reader.cardName()),
                groupName);
            return;
        }
    }
}

} // namespace

std::optional<BushPropCard> BushPropFormat::read(FieldReader& reader)
{
    static constexpr const char* K_NAMES[6] = {"stiffness1", "stiffness2", "stiffness3",
                                               "stiffness4", "stiffness5", "stiffness6"};
    static constexpr const char* B_NAMES[6] = {"damping1", "damping2", "damping3", "damping4", "damping5", "damping6"};
    static constexpr const char* STRUCTURAL_DAMPING_NAMES[6] = {"structuralDamping1", "structuralDamping2",
                                                                "structuralDamping3", "structuralDamping4",
                                                                "structuralDamping5", "structuralDamping6"};
    static constexpr const char* STRESS_FACTOR_NAMES[4] = {"stressFactorTranslational", "stressFactorRotational",
                                                           "strainFactorTranslational", "strainFactorRotational"};
    static constexpr const char* T_NAMES[3] = {"alphaCoefficient", "referenceTemperature", "coincidentLength"};

    BushPropCard card;
    card.id = reader.requiredInteger(1, "id");

    unsigned seen = 0; // bit per group already read
    // A group is checked as soon as a field of it (or the blank field before its flag) holds something.
    for (int flagField = FIRST_FLAG_FIELD; flagField - 1 <= reader.lastUsedField(); flagField += FIELDS_PER_GROUP)
    {
        if (flagField > FIRST_FLAG_FIELD && !reader.isBlank(flagField - 1))
            reader.error(
                "card.bad-value",
                "a group of a " + std::string(reader.cardName()) + " starts with its flag on a line of its own", "id");
        const std::string_view flag = reader.text(flagField);
        if (flag.empty())
        {
            requireBlankAfter(reader, flagField, 0, "flag");
            continue;
        }
        const auto is = [&](const char* name) { return (flag == name); };
        unsigned bit = 0;
        if (is("Stiffness"))
        {
            bit = 1;
            for (int index = 0; index < 6; ++index)
                card.stiffness[static_cast<std::size_t>(index)] =
                    reader.real(flagField + 1 + index, K_NAMES[index], 0.0);
        }
        else if (is("Damping"))
        {
            bit = 2;
            for (int index = 0; index < 6; ++index)
                card.damping[static_cast<std::size_t>(index)] = reader.real(flagField + 1 + index, B_NAMES[index], 0.0);
        }
        else if (is("StructuralDamping"))
        {
            bit = 4;
            std::optional<double> given[6];
            bool othersBlank = true;
            for (int index = 0; index < 6; ++index)
            {
                given[index] = reader.optionalReal(flagField + 1 + index, STRUCTURAL_DAMPING_NAMES[index]);
                othersBlank = othersBlank && (index == 0 || !given[index]);
            }
            // GE1 alone, with GE2 to GE6 blank, is the damping of all six directions. With any of GE2 to
            // GE6 given (0.0 included) every blank one is 0.0.
            for (std::size_t index = 0; index < 6; ++index)
                card.structuralDamping[index] = othersBlank ? given[0].value_or(0.0) : given[index].value_or(0.0);
        }
        else if (is("Recovery"))
        {
            bit = 8;
            double* const values[4] = {&card.stressFactorTranslational, &card.stressFactorRotational,
                                       &card.strainFactorTranslational, &card.strainFactorRotational};
            for (int index = 0; index < 4; ++index)
                *values[index] = reader.real(flagField + 1 + index, STRESS_FACTOR_NAMES[index], 1.0);
            requireBlankAfter(reader, flagField, 4, "Recovery");
        }
        else if (is("Mass"))
        {
            bit = 16;
            card.mass = reader.real(flagField + 1, "mass", 0.0);
            requireBlankAfter(reader, flagField, 1, "Mass");
        }
        else if (is("Thermal"))
        {
            bit = 32;
            double* const values[3] = {&card.alphaCoefficient, &card.referenceTemperature, &card.coincidentLength};
            for (int index = 0; index < 3; ++index)
                *values[index] = reader.real(flagField + 1 + index, T_NAMES[index], 0.0);
            requireBlankAfter(reader, flagField, 3, "Thermal");
        }
        else
        {
            reader.error("card.bad-value",
                         "a group of a " + std::string(reader.cardName()) +
                             " starts with K, B, GE, RCV, M or T, found \"" + std::string(flag) + "\"",
                         "id");
            continue;
        }
        if ((seen & bit) != 0)
            reader.error("card.bad-value",
                         "a group of a " + std::string(reader.cardName()) + " may be given only once: \"" +
                             std::string(flag) + "\"",
                         "id");
        seen |= bit;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushPropFormat::check(const BushPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.mass < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "mass"};
    if (card.coincidentLength < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "coincidentLength"};
    return std::nullopt;
}

void BushPropFormat::write(const BushPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    const auto anyNonZero = [](const std::array<double, 6>& values)
    {
        for (const double value : values)
        {
            if (value != 0.0)
                return true;
        }
        return false;
    };
    // The groups that have something in them, in the usual order, one line each.
    int flagField = FIRST_FLAG_FIELD;
    if (anyNonZero(card.stiffness))
    {
        writer.text(flagField, "Stiffness");
        for (int index = 0; index < 6; ++index)
            writer.realUnlessDefault(flagField + 1 + index, card.stiffness[static_cast<std::size_t>(index)], 0.0);
        flagField += FIELDS_PER_GROUP;
    }
    if (anyNonZero(card.damping))
    {
        writer.text(flagField, "Damping");
        for (int index = 0; index < 6; ++index)
            writer.realUnlessDefault(flagField + 1 + index, card.damping[static_cast<std::size_t>(index)], 0.0);
        flagField += FIELDS_PER_GROUP;
    }
    if (anyNonZero(card.structuralDamping))
    {
        writer.text(flagField, "StructuralDamping");
        // GE2 to GE6 all blank would mean "GE1 for all": they are always written.
        for (int index = 0; index < 6; ++index)
            writer.real(flagField + 1 + index, card.structuralDamping[static_cast<std::size_t>(index)]);
        flagField += FIELDS_PER_GROUP;
    }
    if (card.stressFactorTranslational != 1.0 || card.stressFactorRotational != 1.0 ||
        card.strainFactorTranslational != 1.0 || card.strainFactorRotational != 1.0)
    {
        writer.text(flagField, "Recovery");
        writer.realUnlessDefault(flagField + 1, card.stressFactorTranslational, 1.0);
        writer.realUnlessDefault(flagField + 2, card.stressFactorRotational, 1.0);
        writer.realUnlessDefault(flagField + 3, card.strainFactorTranslational, 1.0);
        writer.realUnlessDefault(flagField + 4, card.strainFactorRotational, 1.0);
        flagField += FIELDS_PER_GROUP;
    }
    if (card.mass != 0.0)
    {
        writer.text(flagField, "Mass");
        writer.real(flagField + 1, card.mass);
        flagField += FIELDS_PER_GROUP;
    }
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0 || card.coincidentLength != 0.0)
    {
        writer.text(flagField, "Thermal");
        writer.realUnlessDefault(flagField + 1, card.alphaCoefficient, 0.0);
        writer.realUnlessDefault(flagField + 2, card.referenceTemperature, 0.0);
        writer.realUnlessDefault(flagField + 3, card.coincidentLength, 0.0);
    }
}

std::uint32_t BushPropTable::append(const BushPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_damping.push(card.damping);
    m_structuralDamping.push(card.structuralDamping);
    m_recovery.push({card.stressFactorTranslational, card.stressFactorRotational, card.strainFactorTranslational,
                     card.strainFactorRotational});
    m_mass.push(card.mass);
    m_thermal.push({card.alphaCoefficient, card.referenceTemperature, card.coincidentLength});
    return row;
}

BushPropCard BushPropTable::card(std::uint32_t row) const
{
    BushPropCard card;
    card.id = m_id.get(row);
    card.stiffness = m_stiffness.get(row);
    card.damping = m_damping.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    const std::array<double, 4> recovery = m_recovery.get(row);
    card.stressFactorTranslational = recovery[0];
    card.stressFactorRotational = recovery[1];
    card.strainFactorTranslational = recovery[2];
    card.strainFactorRotational = recovery[3];
    card.mass = m_mass.get(row);
    const std::array<double, 3> thermal = m_thermal.get(row);
    card.alphaCoefficient = thermal[0];
    card.referenceTemperature = thermal[1];
    card.coincidentLength = thermal[2];
    return card;
}

void BushPropTable::store(std::uint32_t row, const BushPropCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_damping.set(row, card.damping);
    m_structuralDamping.set(row, card.structuralDamping);
    m_recovery.set(row, {card.stressFactorTranslational, card.stressFactorRotational, card.strainFactorTranslational,
                         card.strainFactorRotational});
    m_mass.set(row, card.mass);
    m_thermal.set(row, {card.alphaCoefficient, card.referenceTemperature, card.coincidentLength});
}

void BushPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_damping.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_recovery.keepRows(rows);
    m_mass.keepRows(rows);
    m_thermal.keepRows(rows);
}

void BushPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PBUSH refers to no other card (the tables of a PBUSHT are not modelled).
}

} // namespace oor::model
