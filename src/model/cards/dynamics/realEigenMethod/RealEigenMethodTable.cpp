#include "oor/model/cards/dynamics/realEigenMethod/RealEigenMethodTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

struct NamedMethod
{
    const char* name;
    RealEigenMethodMethod method;
};
constexpr NamedMethod METHODS[] = {{"Lanczos", RealEigenMethodMethod::Lanczos},
                                   {"AutomaticHouseholder", RealEigenMethodMethod::AutoHouseholder},
                                   {"InverseIteration", RealEigenMethodMethod::InversePower},
                                   {"SturmInverse", RealEigenMethodMethod::InversePowerEnhanced},
                                   {"Givens", RealEigenMethodMethod::Givens},
                                   {"ModifiedGivens", RealEigenMethodMethod::ModifiedGivens},
                                   {"Householder", RealEigenMethodMethod::Householder},
                                   {"ModifiedHouseholder", RealEigenMethodMethod::ModifiedHouseholder},
                                   {"AutomaticGivens", RealEigenMethodMethod::AutoGivens}};

struct NamedNorm
{
    const char* name;
    RealEigenMethodNorm norm;
};
constexpr NamedNorm NORMS[] = {
    {"Mass", RealEigenMethodNorm::Mass}, {"Maximum", RealEigenMethodNorm::Max}, {"Point", RealEigenMethodNorm::Point}};

const char* methodName(RealEigenMethodMethod method)
{
    for (const NamedMethod& entry : METHODS)
        if (entry.method == method)
            return entry.name;
    return "";
}

const char* normName(RealEigenMethodNorm norm)
{
    for (const NamedNorm& entry : NORMS)
        if (entry.norm == norm)
            return entry.name;
    return "";
}

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

std::optional<std::int32_t> noneIfNoNd(std::int32_t value)
{
    if (value == RealEigenMethodTable::NO_DESIRED_MODE_COUNT)
        return std::nullopt;
    return value;
}

} // namespace

std::optional<RealEigenMethodCard> RealEigenMethodFormat::read(FieldReader& reader)
{
    RealEigenMethodCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view method = reader.text(2); !method.empty())
    {
        bool found = false;
        for (const NamedMethod& entry : METHODS)
        {
            if ((method == entry.name))
            {
                card.method = entry.method;
                found = true;
            }
        }
        if (!found)
            reader.error("card.bad-value",
                         std::string("method") +
                             " must be LAN, AHOU, INV, SINV, GIV, MGIV, HOU, MHOU or AGIV, found \"" +
                             std::string(method) + "\"",
                         "method");
    }
    card.frequency1 = reader.optionalReal(3, "frequency1");
    card.frequency2 = reader.optionalReal(4, "frequency2");
    card.estimatedModeCount = reader.integer(5, "estimatedModeCount", 0);
    // What a blank ND stands for depends on the method; it stays blank.
    card.desiredModeCount = reader.optionalInteger(6, "desiredModeCount");
    if (!reader.isBlank(7) || !reader.isBlank(8))
        reader.error("card.bad-value", "fields 7 and 8 of the card are not used", "normalization");
    if (const std::string_view norm = reader.text(9); !norm.empty())
    {
        bool found = false;
        for (const NamedNorm& entry : NORMS)
        {
            if ((norm == entry.name))
            {
                card.normalization = entry.norm;
                found = true;
            }
        }
        if (!found)
            reader.error(
                "card.bad-value",
                std::string("normalization") + " must be MASS, MAX or POINT, found \"" + std::string(norm) + "\"",
                "normalization");
    }
    card.normNode = reader.integer(10, "normNode", 0);
    card.normComponent = reader.integer(11, "normComponent", 0);
    if (reader.lastUsedField() > 11)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("normComponent"), "normComponent");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RealEigenMethodFormat::check(const RealEigenMethodCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the method ID must be a positive integer", "id"};
    if (card.frequency1 && card.frequency2 && !(*card.frequency1 < *card.frequency2))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("frequency2"),
                           "frequency1"};
    if (card.estimatedModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "estimatedModeCount"};
    if (card.desiredModeCount && *card.desiredModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "desiredModeCount"};
    if (card.normalization == RealEigenMethodNorm::Point && card.normNode <= 0)
        return CardProblem{
            "card.missing-field",
            CardProblem::fieldNamed("normalization") + " = POINT needs the point " + std::string(CardProblem::FIELD),
            "normNode"};
    if (card.normNode < 0)
        return CardProblem{"card.bad-id",
                           "the point " + std::string(CardProblem::FIELD) + " must be a positive integer", "normNode"};
    // Without POINT, G and C are not used; the card keeps them if they are there.
    if (card.normComponent < 0 || card.normComponent > 6)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a component from 1 to 6, or blank",
                           "normComponent"};
    return std::nullopt;
}

void RealEigenMethodFormat::write(const RealEigenMethodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, methodName(card.method), "Lanczos");
    if (card.frequency1)
        writer.real(3, *card.frequency1);
    if (card.frequency2)
        writer.real(4, *card.frequency2);
    writer.integerUnlessDefault(5, card.estimatedModeCount, 0);
    if (card.desiredModeCount)
        writer.integer(6, *card.desiredModeCount);
    writer.textUnlessDefault(9, normName(card.normalization), "Mass");
    if (card.normNode != 0)
        writer.integer(10, card.normNode);
    if (card.normComponent != 0)
        writer.integer(11, card.normComponent);
}

std::uint32_t RealEigenMethodTable::append(const RealEigenMethodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_frequency1.push(blankIfNone(card.frequency1));
    m_frequency2.push(blankIfNone(card.frequency2));
    m_estimatedModeCount.push(card.estimatedModeCount);
    m_desiredModeCount.push(card.desiredModeCount.value_or(RealEigenMethodTable::NO_DESIRED_MODE_COUNT));
    m_normalization.push(static_cast<std::uint8_t>(card.normalization));
    m_normNode.push(card.normNode);
    m_normComponent.push(card.normComponent);
    return row;
}

RealEigenMethodCard RealEigenMethodTable::card(std::uint32_t row) const
{
    RealEigenMethodCard card;
    card.id = m_id.get(row);
    card.method = static_cast<RealEigenMethodMethod>(m_method.get(row));
    card.frequency1 = noneIfBlank(m_frequency1.get(row));
    card.frequency2 = noneIfBlank(m_frequency2.get(row));
    card.estimatedModeCount = m_estimatedModeCount.get(row);
    card.desiredModeCount = noneIfNoNd(m_desiredModeCount.get(row));
    card.normalization = static_cast<RealEigenMethodNorm>(m_normalization.get(row));
    card.normNode = m_normNode.get(row);
    card.normComponent = m_normComponent.get(row);
    return card;
}

void RealEigenMethodTable::store(std::uint32_t row, const RealEigenMethodCard& card)
{
    m_id.set(row, card.id);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_frequency1.set(row, blankIfNone(card.frequency1));
    m_frequency2.set(row, blankIfNone(card.frequency2));
    m_estimatedModeCount.set(row, card.estimatedModeCount);
    m_desiredModeCount.set(row, card.desiredModeCount.value_or(RealEigenMethodTable::NO_DESIRED_MODE_COUNT));
    m_normalization.set(row, static_cast<std::uint8_t>(card.normalization));
    m_normNode.set(row, card.normNode);
    m_normComponent.set(row, card.normComponent);
}

void RealEigenMethodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_method.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequency2.keepRows(rows);
    m_estimatedModeCount.keepRows(rows);
    m_desiredModeCount.keepRows(rows);
    m_normalization.keepRows(rows);
    m_normNode.keepRows(rows);
    m_normComponent.keepRows(rows);
}

void RealEigenMethodTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const base::EntityId g = m_normNode.get(row);
    if (g > 0)
        m_normNode.set(row, visitor.visit("normNode", ReferenceTarget::Node, g));
}

} // namespace oor::model
