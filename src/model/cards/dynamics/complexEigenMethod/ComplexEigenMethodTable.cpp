#include "oor/model/cards/dynamics/complexEigenMethod/ComplexEigenMethodTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_REGION_FIELD = 9;
constexpr int REGION_FIELDS = 8;

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

std::optional<ComplexMethod> parseMethod(std::string_view text)
{
    if ((text == "InverseIteration"))
        return ComplexMethod::Inv;
    if ((text == "HessenbergMethod"))
        return ComplexMethod::Hess;
    if ((text == "ComplexLanczos"))
        return ComplexMethod::Clan;
    if ((text == "ImplicitRestartedArnoldi"))
        return ComplexMethod::Iram;
    if ((text == "DeterminantMethod"))
        return ComplexMethod::Det;
    return std::nullopt;
}

const char* methodWord(ComplexMethod method)
{
    switch (method)
    {
    case ComplexMethod::Inv:
        return "InverseIteration";
    case ComplexMethod::Hess:
        return "HessenbergMethod";
    case ComplexMethod::Clan:
        return "ComplexLanczos";
    case ComplexMethod::Iram:
        return "ImplicitRestartedArnoldi";
    case ComplexMethod::Det:
        return "DeterminantMethod";
    }
    return "InverseIteration";
}

// One continuation line, in the layout of the method (see ComplexEigenMethodRegion). `first` is the number of its first
// field.
ComplexEigenMethodRegion readRegion(FieldReader& reader, ComplexMethod method, int first)
{
    ComplexEigenMethodRegion region;
    region.alphaA = reader.real(first, "alphaA", 0.0);
    region.omegaA = reader.real(first + 1, "omegaA", 0.0);
    if (method == ComplexMethod::Clan)
    {
        region.maximumBlockSize = reader.real(first + 2, "maximumBlockSize", 7.0);
        region.initialBlockSize = reader.real(first + 3, "initialBlockSize", 2.0);
        region.solveInterval = reader.integer(first + 4, "solveInterval", 5);
        if (!reader.isBlank(first + 5))
            reader.error("card.bad-value", "the sixth field of a block Lanczos line of the card is not used",
                         "shiftModeCount");
    }
    else
    {
        region.alphaB = reader.real(first + 2, "alphaB", 0.0);
        region.omegaB = reader.real(first + 3, "omegaB", 0.0);
        region.regionWidth = reader.real(first + 4, "regionWidth", 1.0);
        region.estimatedModeCount = reader.integer(first + 5, "estimatedModeCount", 0);
    }
    // A blank NDJ is 3 * NEJ for the inverse power method: stored resolved.
    region.desiredModeCount =
        reader.integer(first + 6, "desiredModeCount", method == ComplexMethod::Inv ? 3 * region.estimatedModeCount : 0);
    if (!reader.isBlank(first + 7))
        reader.error("card.bad-value", "the eighth field of a search region of the card is not used",
                     "desiredModeCount");
    return region;
}

} // namespace

std::optional<ComplexEigenMethodCard> ComplexEigenMethodFormat::read(FieldReader& reader)
{
    ComplexEigenMethodCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view methodText = reader.text(2);
    if (methodText.empty())
        reader.error("card.missing-field", std::string("methodName") + " is required", "methodName");
    else if (const auto method = parseMethod(methodText))
        card.methodName = *method;
    else
        reader.error("card.bad-value",
                     std::string("methodName") + " must be INV, HESS, CLAN, IRAM or DET, found \"" +
                         std::string(methodText) + "\"",
                     "methodName");
    const std::string_view normText = reader.text(3);
    if (normText.empty() || (normText == "Maximum"))
        card.norm = ComplexNorm::Max;
    else if ((normText == "Point"))
        card.norm = ComplexNorm::Point;
    else
        reader.error("card.bad-value",
                     std::string("norm") + " must be MAX or POINT, found \"" + std::string(normText) + "\"", "norm");
    card.normNode = reader.integer(4, "normNode", 0);
    card.normComponent = reader.integer(5, "normComponent", 0);
    card.convergenceTolerance = reader.optionalReal(6, "convergenceTolerance");
    card.desiredModeCount0 = reader.integer(7, "desiredModeCount0", 0);
    if (!reader.isBlank(8))
        reader.error("card.bad-value", "field 8 of the card is not used", "desiredModeCount0");
    const int last = reader.lastUsedField();
    for (int first = FIRST_REGION_FIELD; first <= last && !reader.failed(); first += REGION_FIELDS)
        card.regions.push_back(readRegion(reader, card.methodName, first));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ComplexEigenMethodFormat::check(const ComplexEigenMethodCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the method ID must be a positive integer", "id"};
    if (card.norm == ComplexNorm::Point && card.normNode <= 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required when " + CardProblem::fieldNamed("norm") + " is POINT",
            "normNode"};
    if (card.norm == ComplexNorm::Max && (card.normNode != 0 || card.normComponent != 0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("normComponent") +
                               " are for " + CardProblem::fieldNamed("norm") + " = POINT only",
                           "normNode"};
    if (card.normNode < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "normNode"};
    if (card.normComponent < 0 || card.normComponent > 6)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a component from 1 to 6, or blank",
                           "normComponent"};
    if (card.convergenceTolerance && !(*card.convergenceTolerance > 0.0 && std::isfinite(*card.convergenceTolerance)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "convergenceTolerance"};
    if (card.desiredModeCount0 < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "desiredModeCount0"};
    const ComplexEigenMethodRegion defaults;
    for (const ComplexEigenMethodRegion& region : card.regions)
    {
        if (!std::isfinite(region.alphaA) || !std::isfinite(region.omegaA) || !std::isfinite(region.alphaB) ||
            !std::isfinite(region.omegaB))
            return CardProblem{"card.bad-value", "the coordinates of a search region must be finite numbers", "alphaA"};
        if (region.desiredModeCount < 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                               "desiredModeCount"};
        if (card.methodName == ComplexMethod::Clan)
        {
            if (region.alphaB != defaults.alphaB || region.omegaB != defaults.omegaB ||
                region.regionWidth != defaults.regionWidth || region.estimatedModeCount != defaults.estimatedModeCount)
                return CardProblem{
                    "card.bad-value",
                    "a block Lanczos (CLAN) line has no " + CardProblem::fieldNamed("alphaB") + ", " +
                        CardProblem::fieldNamed("omegaB") + ", " + CardProblem::fieldNamed("regionWidth") + " or " +
                        CardProblem::fieldNamed("estimatedModeCount") + ": it has " + std::string(CardProblem::FIELD) +
                        ", " + CardProblem::fieldNamed("initialBlockSize") + " and " +
                        CardProblem::fieldNamed("solveInterval"),
                    "maximumBlockSize"};
            if (!(region.maximumBlockSize > 0.0))
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                                   "maximumBlockSize"};
            if (!(region.initialBlockSize > 0.0))
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                                   "initialBlockSize"};
            if (region.solveInterval < 1)
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                                   "solveInterval"};
        }
        else
        {
            if (region.maximumBlockSize != defaults.maximumBlockSize ||
                region.initialBlockSize != defaults.initialBlockSize || region.solveInterval != defaults.solveInterval)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + ", " +
                                       CardProblem::fieldNamed("initialBlockSize") + " and " +
                                       CardProblem::fieldNamed("solveInterval") + " are for " +
                                       CardProblem::fieldNamed("methodName") + " = CLAN only",
                                   "maximumBlockSize"};
            if (!(region.regionWidth > 0.0))
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                                   "regionWidth"};
            if (region.estimatedModeCount < 0)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                                   "estimatedModeCount"};
        }
    }
    return std::nullopt;
}

void ComplexEigenMethodFormat::write(const ComplexEigenMethodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, methodWord(card.methodName));
    writer.textUnlessDefault(3, card.norm == ComplexNorm::Point ? "Point" : "Maximum", "Maximum");
    if (card.normNode != 0)
        writer.integer(4, card.normNode);
    if (card.normComponent != 0)
        writer.integer(5, card.normComponent);
    if (card.convergenceTolerance)
        writer.real(6, *card.convergenceTolerance);
    if (card.desiredModeCount0 != 0)
        writer.integer(7, card.desiredModeCount0);
    int first = FIRST_REGION_FIELD;
    for (const ComplexEigenMethodRegion& region : card.regions)
    {
        // The first two fields are always written, so that a region whose values are all defaults is still a line.
        writer.real(first, region.alphaA);
        writer.real(first + 1, region.omegaA);
        if (card.methodName == ComplexMethod::Clan)
        {
            writer.realUnlessDefault(first + 2, region.maximumBlockSize, 7.0);
            writer.realUnlessDefault(first + 3, region.initialBlockSize, 2.0);
            writer.integerUnlessDefault(first + 4, region.solveInterval, 5);
        }
        else
        {
            writer.realUnlessDefault(first + 2, region.alphaB, 0.0);
            writer.realUnlessDefault(first + 3, region.omegaB, 0.0);
            writer.realUnlessDefault(first + 4, region.regionWidth, 1.0);
            if (region.estimatedModeCount != 0)
                writer.integer(first + 5, region.estimatedModeCount);
        }
        if (region.desiredModeCount !=
            0) // 3 * NEJ for the inverse power method when blank: written whenever it has a value
            writer.integer(first + 6, region.desiredModeCount);
        first += REGION_FIELDS;
    }
}

void ComplexEigenMethodTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // G is a grid or a scalar point, which may be named without an SPOINT.
    if (m_normNode.get(row) != 0)
        m_normNode.set(row, visitor.visit("normNode", ReferenceTarget::ScalarOrGridPoint, m_normNode.get(row)));
}

std::uint32_t ComplexEigenMethodTable::append(const ComplexEigenMethodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_methodName.push(static_cast<std::uint8_t>(card.methodName));
    m_norm.push(static_cast<std::uint8_t>(card.norm));
    m_normNode.push(card.normNode);
    m_normComponent.push(card.normComponent);
    m_convergenceTolerance.push(blankIfNone(card.convergenceTolerance));
    m_desiredModeCount0.push(card.desiredModeCount0);
    std::vector<double> alphaA, omegaA, alphaB, omegaB, regionWidth, maximumBlockSize, initialBlockSize;
    std::vector<std::int32_t> estimatedModeCount, solveInterval, desiredModeCount;
    for (const ComplexEigenMethodRegion& region : card.regions)
    {
        alphaA.push_back(region.alphaA);
        omegaA.push_back(region.omegaA);
        alphaB.push_back(region.alphaB);
        omegaB.push_back(region.omegaB);
        regionWidth.push_back(region.regionWidth);
        estimatedModeCount.push_back(region.estimatedModeCount);
        maximumBlockSize.push_back(region.maximumBlockSize);
        initialBlockSize.push_back(region.initialBlockSize);
        solveInterval.push_back(region.solveInterval);
        desiredModeCount.push_back(region.desiredModeCount);
    }
    m_alphaA.push(alphaA);
    m_omegaA.push(omegaA);
    m_alphaB.push(alphaB);
    m_omegaB.push(omegaB);
    m_regionWidth.push(regionWidth);
    m_estimatedModeCount.push(estimatedModeCount);
    m_maximumBlockSize.push(maximumBlockSize);
    m_initialBlockSize.push(initialBlockSize);
    m_solveInterval.push(solveInterval);
    m_desiredModeCount.push(desiredModeCount);
    return row;
}

ComplexEigenMethodCard ComplexEigenMethodTable::card(std::uint32_t row) const
{
    ComplexEigenMethodCard card;
    card.id = m_id.get(row);
    card.methodName = static_cast<ComplexMethod>(m_methodName.get(row));
    card.norm = static_cast<ComplexNorm>(m_norm.get(row));
    card.normNode = m_normNode.get(row);
    card.normComponent = m_normComponent.get(row);
    card.convergenceTolerance = noneIfBlank(m_convergenceTolerance.get(row));
    card.desiredModeCount0 = m_desiredModeCount0.get(row);
    const auto alphaA = m_alphaA.get(row);
    const auto omegaA = m_omegaA.get(row);
    const auto alphaB = m_alphaB.get(row);
    const auto omegaB = m_omegaB.get(row);
    const auto regionWidth = m_regionWidth.get(row);
    const auto estimatedModeCount = m_estimatedModeCount.get(row);
    const auto maximumBlockSize = m_maximumBlockSize.get(row);
    const auto initialBlockSize = m_initialBlockSize.get(row);
    const auto solveInterval = m_solveInterval.get(row);
    const auto desiredModeCount = m_desiredModeCount.get(row);
    for (std::size_t index = 0; index < alphaA.size(); ++index)
    {
        ComplexEigenMethodRegion region;
        region.alphaA = alphaA[index];
        region.omegaA = omegaA[index];
        region.alphaB = alphaB[index];
        region.omegaB = omegaB[index];
        region.regionWidth = regionWidth[index];
        region.estimatedModeCount = estimatedModeCount[index];
        region.maximumBlockSize = maximumBlockSize[index];
        region.initialBlockSize = initialBlockSize[index];
        region.solveInterval = solveInterval[index];
        region.desiredModeCount = desiredModeCount[index];
        card.regions.push_back(region);
    }
    return card;
}

void ComplexEigenMethodTable::store(std::uint32_t row, const ComplexEigenMethodCard& card)
{
    m_id.set(row, card.id);
    m_methodName.set(row, static_cast<std::uint8_t>(card.methodName));
    m_norm.set(row, static_cast<std::uint8_t>(card.norm));
    m_normNode.set(row, card.normNode);
    m_normComponent.set(row, card.normComponent);
    m_convergenceTolerance.set(row, blankIfNone(card.convergenceTolerance));
    m_desiredModeCount0.set(row, card.desiredModeCount0);
    std::vector<double> alphaA, omegaA, alphaB, omegaB, regionWidth, maximumBlockSize, initialBlockSize;
    std::vector<std::int32_t> estimatedModeCount, solveInterval, desiredModeCount;
    for (const ComplexEigenMethodRegion& region : card.regions)
    {
        alphaA.push_back(region.alphaA);
        omegaA.push_back(region.omegaA);
        alphaB.push_back(region.alphaB);
        omegaB.push_back(region.omegaB);
        regionWidth.push_back(region.regionWidth);
        estimatedModeCount.push_back(region.estimatedModeCount);
        maximumBlockSize.push_back(region.maximumBlockSize);
        initialBlockSize.push_back(region.initialBlockSize);
        solveInterval.push_back(region.solveInterval);
        desiredModeCount.push_back(region.desiredModeCount);
    }
    m_alphaA.set(row, alphaA);
    m_omegaA.set(row, omegaA);
    m_alphaB.set(row, alphaB);
    m_omegaB.set(row, omegaB);
    m_regionWidth.set(row, regionWidth);
    m_estimatedModeCount.set(row, estimatedModeCount);
    m_maximumBlockSize.set(row, maximumBlockSize);
    m_initialBlockSize.set(row, initialBlockSize);
    m_solveInterval.set(row, solveInterval);
    m_desiredModeCount.set(row, desiredModeCount);
}

void ComplexEigenMethodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_methodName.keepRows(rows);
    m_norm.keepRows(rows);
    m_normNode.keepRows(rows);
    m_normComponent.keepRows(rows);
    m_convergenceTolerance.keepRows(rows);
    m_desiredModeCount0.keepRows(rows);
    m_alphaA.keepRows(rows);
    m_omegaA.keepRows(rows);
    m_alphaB.keepRows(rows);
    m_omegaB.keepRows(rows);
    m_regionWidth.keepRows(rows);
    m_estimatedModeCount.keepRows(rows);
    m_maximumBlockSize.keepRows(rows);
    m_initialBlockSize.keepRows(rows);
    m_solveInterval.keepRows(rows);
    m_desiredModeCount.keepRows(rows);
}

} // namespace oor::model
