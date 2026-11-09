#include "oor/model/cards/materials/failureMat/FailureMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

// The values of the PF line: A1, the residual stiffness, then A2 to A5.
constexpr const char* PROGRESSIVE_FAILURE_NAMES[FailureMatCard::PROGRESSIVE_FAILURE_VALUES] = {
    "residualStiffness", "degradationParameter2", "degradationParameter3", "degradationParameter4",
    "degradationParameter5"};
constexpr int LINE = 8; // fields to a line
constexpr int FIRST_BLOCK_FIELD = 9;
constexpr int BLOCK_FIELDS = 3 * LINE; // the three lines of a criterion

constexpr int CRITERIA[] = {1, 2, 3, 4, 5, 7, 8, 10, 11, 13};

// The SOL 700 criteria (the SOL 700 form of MATF in the format reference): 101 maximum equivalent stress, 102
// Johnson-Cook, 103 maximum plastic strain, 104 maximum pressure, 105 CAP.
constexpr int FIRST_EXPLICIT_CRITERION = 101;
constexpr int LAST_EXPLICIT_CRITERION = 105;
constexpr int JOHNSON_COOK = 102;
constexpr int CAP = 105;

// The V values (by index) that the format of the SOL 700 form requires: MES (101), MPS (103), PRS (104), and ALPHA,
// THETA, GAMMA, BETA, R, D and W (105). The others have defaults.
constexpr std::size_t CAP_REQUIRED[] = {0, 1, 2, 3, 4, 5, 8};

bool is(std::string_view text, std::string_view word)
{
    return (text == word);
}

bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

bool isCriterion(int code)
{
    return std::find(std::begin(CRITERIA), std::end(CRITERIA), code) != std::end(CRITERIA);
}

bool isExplicitCriterion(int code)
{
    return code >= FIRST_EXPLICIT_CRITERION && code <= LAST_EXPLICIT_CRITERION;
}

// The cell of a SOL 700 criterion that holds a word instead of a number, as the index into FailureMatCriterion::words,
// or -1: MTH sits in the cell of V11 for 102; TYPE in the cell of V12 and ITER in that of W1 for 105.
int vWordIndex(int criterion, std::size_t vIndex)
{
    if (criterion == JOHNSON_COOK && vIndex == 10)
        return 0;
    if (criterion == CAP && vIndex == 11)
        return 0;
    return -1;
}

int wWordIndex(int criterion, std::size_t wIndex)
{
    return criterion == CAP && wIndex == 0 ? 1 : -1;
}

// The words a SOL 700 criterion takes in its word cells (index 0 then 1).
std::span<const std::string_view> explicitWords(int criterion, int wordIndex)
{
    static constexpr std::string_view FAILURE_METHODS[] = {"Continue", "Discard"};
    static constexpr std::string_view CAP_TYPES[] = {"SoilModel", "RockModel"};
    static constexpr std::string_view CAP_ITERATIONS[] = {"VectorizedIterations", "FullIterations"};
    if (criterion == JOHNSON_COOK)
        return FAILURE_METHODS;
    if (wordIndex == 0)
        return CAP_TYPES;
    return CAP_ITERATIONS;
}

template <std::size_t N>
bool allFinite(const std::array<std::optional<double>, N>& values)
{
    for (const std::optional<double>& value : values)
    {
        if (value && !std::isfinite(*value))
            return false;
    }
    return true;
}

// The fields of the criterion that starts at `first` (the field of "CRI"): the V values of the first line, then V7 to
// V9, FIND, V10 to V12 and W1 of the second, then W2 to W9 of the third.
int vField(int first, std::size_t index)
{
    if (index < 6)
        return first + 2 + static_cast<int>(index);
    if (index < 9)
        return first + LINE + static_cast<int>(index - 6);
    return first + LINE + 4 + static_cast<int>(index - 9);
}

int findField(int first)
{
    return first + LINE + 3;
}

int wField(int first, std::size_t index)
{
    if (index == 0)
        return first + LINE + 7;
    return first + 2 * LINE + static_cast<int>(index - 1);
}

} // namespace

std::optional<FailureMatCard> FailureMatFormat::read(FieldReader& reader)
{
    FailureMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.degradationKind = reader.integer(2, "degradationKind", 0);
    for (int field = 3; field < FIRST_BLOCK_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return card;
        }
    }
    const int last = reader.lastUsedField();
    int position = FIRST_BLOCK_FIELD;
    while (position <= last && !reader.failed())
    {
        const std::string_view keyword = reader.text(position);
        if (keyword.empty())
        {
            if (lineHasValues(reader, position))
            {
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) + " after the first starts with " +
                                 fieldNameList({"criteria", "progressiveFailure"}),
                             "");
                return card;
            }
            position += LINE;
        }
        else if (is(keyword, "Criterion"))
        {
            FailureMatCriterion criterion;
            criterion.criterion = reader.requiredInteger(position + 1, "criterion");
            for (std::size_t index = 0; index < criterion.values.size(); ++index)
            {
                if (const int word = vWordIndex(criterion.criterion, index); word >= 0)
                    criterion.words[static_cast<std::size_t>(word)] = std::string(reader.word(vField(position, index)));
                else
                    criterion.values[index] = reader.optionalReal(vField(position, index), "criterionValue");
            }
            criterion.failureIndexScale = reader.optionalReal(findField(position), "failureIndexScale");
            for (std::size_t index = 0; index < criterion.strainValues.size(); ++index)
            {
                if (const int word = wWordIndex(criterion.criterion, index); word >= 0)
                    criterion.words[static_cast<std::size_t>(word)] = std::string(reader.word(wField(position, index)));
                else
                    criterion.strainValues[index] = reader.optionalReal(wField(position, index), "strainLimit");
            }
            card.criteria.push_back(criterion);
            position += BLOCK_FIELDS;
        }
        else if (is(keyword, "ProgressiveFailure"))
        {
            if (card.progressiveFailure)
            {
                reader.error(
                    "card.bad-value",
                    "a " + std::string(reader.cardName()) + " has one " + std::string("progressiveFailure") + " line",
                    "progressiveFailure");
                return card;
            }
            std::array<std::optional<double>, FailureMatCard::PROGRESSIVE_FAILURE_VALUES> values{};
            for (std::size_t index = 0; index < values.size(); ++index)
                values[index] =
                    reader.optionalReal(position + 1 + static_cast<int>(index), PROGRESSIVE_FAILURE_NAMES[index]);
            for (const int gap : {position + 6, position + 7})
            {
                if (!reader.isBlank(gap))
                {
                    reader.error("card.bad-value",
                                 "field " + std::to_string(gap) + " of the " + std::string("progressiveFailure") +
                                     " line of a " + std::string(reader.cardName()) + " must be blank",
                                 "progressiveFailure");
                    return card;
                }
            }
            card.progressiveFailure = values;
            position += LINE;
        }
        else
        {
            reader.error("card.bad-value",
                         "\"" + std::string(keyword) + "\" does not start a line of a " +
                             std::string(reader.cardName()) + ": " + fieldNameList({"criteria", "progressiveFailure"}),
                         "");
            return card;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

namespace
{

// The rules of the SOL 700 form: one criterion only, no ITYPE and no PF line, the values the format requires, and the
// words of the word cells. Nothing for the other criteria.
std::optional<CardProblem> checkExplicitForm(const FailureMatCard& card)
{
    const bool anyExplicit =
        std::any_of(card.criteria.begin(), card.criteria.end(),
                    [](const FailureMatCriterion& criterion) { return isExplicitCriterion(criterion.criterion); });
    if (!anyExplicit)
    {
        for (const FailureMatCriterion& criterion : card.criteria)
        {
            if (!criterion.words[0].empty() || !criterion.words[1].empty())
                return CardProblem{"card.bad-value", "only the SOL 700 criteria 102 and 105 have words", "criteria"};
        }
        return std::nullopt;
    }
    if (card.criteria.size() != 1 || card.degradationKind != 0 || card.progressiveFailure)
        return CardProblem{"card.bad-value",
                           "a SOL 700 criterion (101 to 105) is the only one of its card, with no " +
                               CardProblem::fieldNamed("degradationKind") + " and no " +
                               CardProblem::fieldNamed("progressiveFailure") + " line",
                           "criteria"};
    const FailureMatCriterion& criterion = card.criteria.front();
    const auto isGiven = [&](std::size_t index) { return criterion.values[index].has_value(); };
    switch (criterion.criterion)
    {
    case 101:
        if (!isGiven(0))
            return CardProblem{"card.missing-field",
                               "criterion 101 needs " + CardProblem::fieldNamed("maxEquivalentStress"),
                               "criterionValue"};
        break;
    case 103:
        if (!isGiven(0))
            return CardProblem{"card.missing-field",
                               "criterion 103 needs " + CardProblem::fieldNamed("maxPlasticStrain"), "criterionValue"};
        break;
    case 104:
        if (!isGiven(0))
            return CardProblem{"card.missing-field", "criterion 104 needs " + CardProblem::fieldNamed("maxPressure"),
                               "criterionValue"};
        break;
    case CAP:
        for (const std::size_t index : CAP_REQUIRED)
        {
            if (!isGiven(index))
                return CardProblem{"card.missing-field",
                                   "criterion 105 needs " + CardProblem::fieldNamed("capAlpha") + ", " +
                                       CardProblem::fieldNamed("capTheta") + ", " +
                                       CardProblem::fieldNamed("capGamma") + ", " + CardProblem::fieldNamed("capBeta") +
                                       ", " + CardProblem::fieldNamed("capShapeRatio") + ", " +
                                       CardProblem::fieldNamed("capHardeningExponent") + " and " +
                                       CardProblem::fieldNamed("strainLimit"),
                                   "criterionValue"};
        }
        break;
    default:
        break;
    }
    for (std::size_t word = 0; word < criterion.words.size(); ++word)
    {
        const std::string& value = criterion.words[word];
        if (value.empty())
            continue;
        const bool hasCell = (criterion.criterion == JOHNSON_COOK && word == 0) || criterion.criterion == CAP;
        if (!hasCell)
            return CardProblem{"card.bad-value", "criterion " + std::to_string(criterion.criterion) + " has no word",
                               "criteria"};
        const auto allowed = explicitWords(criterion.criterion, static_cast<int>(word));
        if (std::find(allowed.begin(), allowed.end(), value) == allowed.end())
            return CardProblem{"card.bad-value",
                               "\"" + value + "\" is not a word of criterion " + std::to_string(criterion.criterion),
                               "criteria"};
    }
    return std::nullopt;
}

} // namespace

std::optional<CardProblem> FailureMatFormat::check(const FailureMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.degradationKind != 0 && card.degradationKind != 2 && card.degradationKind != 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 2 or 3", "degradationKind"};
    if (card.criteria.empty() || card.criteria.size() > FailureMatCard::MAX_CRITERIA)
        return CardProblem{"card.bad-value", "the card has from one to three criteria", "criteria"};
    if (const auto problem = checkExplicitForm(card))
        return problem;
    for (const FailureMatCriterion& criterion : card.criteria)
    {
        if (!isCriterion(criterion.criterion) && !isExplicitCriterion(criterion.criterion))
            return CardProblem{"card.bad-value",
                               "the criterion must be 1, 2, 3, 4, 5, 7, 8, 10, 11 or 13, or 101 to 105 (SOL 700)",
                               "criterion"};
        if (!allFinite(criterion.values) || !allFinite(criterion.strainValues) ||
            (criterion.failureIndexScale && !std::isfinite(*criterion.failureIndexScale)))
            return CardProblem{"card.bad-value", "the values of a criterion must be finite numbers", "criterionValue"};
    }
    if (card.progressiveFailure)
    {
        if (card.degradationKind == 0)
            return CardProblem{"card.bad-value",
                               "the " + std::string(CardProblem::FIELD) + " line needs " +
                                   CardProblem::fieldNamed("degradationKind") + " of 2 or 3",
                               "progressiveFailure"};
        const auto& values = *card.progressiveFailure;
        if (!allFinite(values))
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("residualStiffness") + " to " +
                                   CardProblem::fieldNamed("degradationParameter5") + " must be finite numbers",
                               "progressiveFailure"};
        if (values[0] && !(*values[0] > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "residualStiffness"};
        for (std::size_t index = 1; index < values.size(); ++index)
        {
            if (values[index] && !(*values[index] >= 0.0 && *values[index] <= 1.0))
                return CardProblem{"card.bad-value",
                                   CardProblem::fieldNamed("degradationParameter2") + " to " +
                                       CardProblem::fieldNamed("degradationParameter5") + " must be from 0.0 to 1.0",
                                   "progressiveFailure"};
        }
    }
    return std::nullopt;
}

void FailureMatFormat::write(const FailureMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.degradationKind, 0);
    int position = FIRST_BLOCK_FIELD;
    for (std::size_t block = 0; block < card.criteria.size(); ++block)
    {
        const FailureMatCriterion& criterion = card.criteria[block];
        writer.text(position, "Criterion");
        writer.integer(position + 1, criterion.criterion);
        for (std::size_t index = 0; index < criterion.values.size(); ++index)
        {
            if (const int word = vWordIndex(criterion.criterion, index); word >= 0)
            {
                if (!criterion.words[static_cast<std::size_t>(word)].empty())
                    writer.text(vField(position, index), criterion.words[static_cast<std::size_t>(word)]);
            }
            else if (criterion.values[index])
                writer.real(vField(position, index), *criterion.values[index]);
        }
        if (criterion.failureIndexScale)
            writer.real(findField(position), *criterion.failureIndexScale);
        for (std::size_t index = 0; index < criterion.strainValues.size(); ++index)
        {
            if (const int word = wWordIndex(criterion.criterion, index); word >= 0)
            {
                if (!criterion.words[static_cast<std::size_t>(word)].empty())
                    writer.text(wField(position, index), criterion.words[static_cast<std::size_t>(word)]);
            }
            else if (criterion.strainValues[index])
                writer.real(wField(position, index), *criterion.strainValues[index]);
        }
        position += BLOCK_FIELDS;
        // The progressive failure line follows the lines of the first criterion.
        if (block == 0 && card.progressiveFailure)
        {
            writer.text(position, "ProgressiveFailure");
            for (std::size_t index = 0; index < card.progressiveFailure->size(); ++index)
            {
                if ((*card.progressiveFailure)[index])
                    writer.real(position + 1 + static_cast<int>(index), *(*card.progressiveFailure)[index]);
            }
            position += LINE;
        }
    }
}

std::uint32_t FailureMatTable::append(const FailureMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FailureMatTable::store(std::uint32_t row, const FailureMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FailureMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FailureMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FailureMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card extends.
    FailureMatCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
