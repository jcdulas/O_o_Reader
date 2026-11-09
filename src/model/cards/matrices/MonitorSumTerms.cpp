#include "oor/model/cards/matrices/MonitorSumTerms.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <bit>
#include <cmath>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int TRIPLES_PER_LINE = 2;
constexpr int FIELDS_PER_TRIPLE = 3;
constexpr int FIRST_NAME_OFFSET = 3; // the NAME of the first triple is the third field of a line

bool lineIsBlank(const FieldReader& reader, int lineBase)
{
    for (int field = lineBase + 1; field <= lineBase + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// The digits of a set of components as the bits 1 to 6 of a mask (0 for a value that is not a set of components).
unsigned digitMask(std::int32_t components) noexcept
{
    unsigned mask = 0;
    for (std::int32_t rest = components; rest > 0; rest /= 10)
        mask |= 1U << static_cast<unsigned>(rest % 10);
    return mask;
}

// The word of a type in a message: a placeholder for the one that is a card (CardProblem::cardNamed).
std::string messageWordOf(MonitorResultType type)
{
    const std::string_view word = MONITOR_RESULT_WORDS[static_cast<std::size_t>(type)];
    return type == MonitorResultType::LoadPoint ? CardProblem::cardNamed(word) : std::string(word);
}

} // namespace

std::string monitorTypeWord(MonitorResultType type)
{
    return std::string(MONITOR_RESULT_WORDS[static_cast<std::size_t>(type)]);
}

std::string acceptedMonitorTypes(std::uint32_t accepted)
{
    std::string phrase;
    int remaining = std::popcount(accepted & ALL_MONITOR_TYPES);
    for (std::size_t index = 0; index < MONITOR_RESULT_WORDS.size(); ++index)
    {
        if ((accepted & (1U << index)) == 0)
            continue;
        --remaining;
        phrase += messageWordOf(static_cast<MonitorResultType>(index));
        if (remaining > 1)
            phrase += ", ";
        else if (remaining == 1)
            phrase += " or ";
    }
    return phrase;
}

std::optional<MonitorResultType> readMonitorType(FieldReader& reader, int field, const char* name,
                                                 std::uint32_t accepted)
{
    const std::string_view text = reader.text(field);
    const std::optional<MonitorResultType> type = parseKeyword<MonitorResultType>(MONITOR_RESULT_WORDS, text);
    if (!type || (accepted & monitorTypeBit(*type)) == 0)
    {
        reader.report(CardProblem{"card.bad-value",
                                  std::string(CardProblem::FIELD) + " must be " + acceptedMonitorTypes(accepted) +
                                      ", not \"" + std::string(text) + "\"",
                                  name});
        return std::nullopt;
    }
    return type;
}

std::optional<CardProblem> monitorTypeProblem(MonitorResultType type, std::uint32_t accepted, const char* field)
{
    if ((accepted & monitorTypeBit(type)) != 0)
        return std::nullopt;
    return CardProblem{
        "card.bad-value",
        std::string(CardProblem::FIELD) + " must be " + acceptedMonitorTypes(accepted) + ", not " + messageWordOf(type),
        field};
}

std::optional<CardProblem> monitoredNameProblem(std::string_view name, const char* field)
{
    return checkNameWord(name, field);
}

// The names of the fields of a term, as the code gives them (a diagnostic shows their keywords).
constexpr const char* TYPE_NAME = "termType";
constexpr const char* TERM_NAME = "termNames";
constexpr const char* AXES_NAME = "termAxes";
constexpr const char* COEFFICIENT_NAME = "termCoefficient";

MonitorSumBody readMonitorSumTerms(FieldReader& reader, int firstLine, std::uint32_t accepted, const char* newAxesName)
{
    const std::string newAxesLabel = std::string(newAxesName);
    const std::string typeLabel = std::string(TYPE_NAME);
    const std::string nameLabel = std::string(TERM_NAME);
    MonitorSumBody body;
    const int last = reader.lastUsedField();
    bool started = false;
    bool inBlock = false;      // a NEWAXISj was read
    bool blockHasTerms = true; // the block that was started has at least one term
    std::int32_t blockAxes = 0;
    std::optional<MonitorResultType> sectionType; // the type of the terms being read: it carries over from line to line
    for (int line = firstLine; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        if (lineIsBlank(reader, lineBase))
            continue;
        const int firstField = lineBase + 1;
        const int secondField = lineBase + 2;
        const bool firstLineOfCard = !started;
        if (firstLineOfCard)
        {
            started = true;
            // The first field of the first line is a number (a NEWAXISj) in the first layout and a word (an MTYPE) in
            // the alternate one, which has the two fields the other way round.
            body.alternate =
                !reader.isBlank(firstField) && !base::NumberParser::parseInteger(reader.text(firstField)).has_value();
        }
        const int newAxesField = body.alternate ? secondField : firstField;
        const int typeField = body.alternate ? firstField : secondField;

        std::optional<MonitorResultType> typeOnLine;
        if (!reader.isBlank(typeField))
        {
            typeOnLine = readMonitorType(reader, typeField, TYPE_NAME, accepted);
            if (!typeOnLine)
                break;
        }
        if (!reader.isBlank(newAxesField))
        {
            if (!blockHasTerms)
            {
                reader.error("card.missing-field", newAxesLabel + " has no " + nameLabel + " to sum", TERM_NAME);
                break;
            }
            blockAxes = base::sortedDigits(reader.requiredInteger(newAxesField, newAxesName));
            inBlock = true;
            blockHasTerms = false;
            if (typeOnLine)
                sectionType = typeOnLine; // otherwise the block goes on with the type of the line above
            if (!sectionType && !reader.failed())
            {
                reader.error("card.missing-field", typeLabel + " is required on the line that starts a " + newAxesLabel,
                             TYPE_NAME);
                break;
            }
        }
        else if (typeOnLine)
        {
            sectionType = typeOnLine; // a new type inside the block
        }
        else if (firstLineOfCard)
        {
            reader.error("card.missing-field", newAxesLabel + " is required on the first line", newAxesName);
            break;
        }

        for (int slot = 0; slot < TRIPLES_PER_LINE; ++slot)
        {
            const int nameField = lineBase + FIRST_NAME_OFFSET + slot * FIELDS_PER_TRIPLE;
            const int axesField = nameField + 1;
            const int coefficientField = nameField + 2;
            if (reader.isBlank(nameField) && reader.isBlank(axesField) && reader.isBlank(coefficientField))
                continue;
            if (!inBlock || !sectionType)
            {
                reader.error("card.missing-field",
                             "a " + nameLabel + " needs a " + newAxesLabel + " and a " + typeLabel + " above it",
                             TERM_NAME);
                break;
            }
            if (reader.isBlank(nameField))
            {
                reader.error("card.missing-field",
                             std::string(AXES_NAME) + " and " + std::string(COEFFICIENT_NAME) + " need the " +
                                 nameLabel + " they belong to",
                             TERM_NAME);
                break;
            }
            MonitorSumTerm term;
            term.newAxes = blockAxes;
            term.type = *sectionType;
            term.name = std::string(reader.text(nameField));
            // A blank AXES is the components of the NEWAXISj, kept resolved so that editing the NEWAXISj
            // does not move it.
            term.axes = reader.isBlank(axesField) ? blockAxes
                                                  : base::sortedDigits(reader.requiredInteger(axesField, AXES_NAME));
            term.coefficient = reader.real(coefficientField, COEFFICIENT_NAME, 1.0);
            body.terms.push_back(std::move(term));
            blockHasTerms = true;
        }
    }
    if (inBlock && !blockHasTerms && !reader.failed())
        reader.error("card.missing-field", newAxesLabel + " has no " + nameLabel + " to sum", TERM_NAME);
    return body;
}

void writeMonitorSumTerms(FieldWriter& writer, int firstLine, const MonitorSumBody& body)
{
    const std::vector<MonitorSumTerm>& terms = body.terms;
    int line = firstLine;
    std::optional<MonitorResultType> lastType; // the type the lines above ended with
    std::size_t index = 0;
    while (index < terms.size())
    {
        // A block: the terms of one NEWAXISj in a row.
        const std::int32_t blockAxes = terms[index].newAxes;
        std::size_t blockEnd = index;
        while (blockEnd < terms.size() && terms[blockEnd].newAxes == blockAxes)
            ++blockEnd;
        bool firstLineOfBlock = true;
        while (index < blockEnd)
        {
            // A section: the terms of one type in a row, two to a line.
            const MonitorResultType type = terms[index].type;
            std::size_t sectionEnd = index;
            while (sectionEnd < blockEnd && terms[sectionEnd].type == type)
                ++sectionEnd;
            bool firstLineOfSection = true;
            while (index < sectionEnd)
            {
                const int lineBase = line * FIELDS_PER_LINE;
                if (body.alternate)
                {
                    // The type is written when it is not the one the lines above ended with.
                    if (firstLineOfSection && lastType != type)
                        writer.text(lineBase + 1, monitorTypeWord(type));
                    if (firstLineOfBlock)
                        writer.integer(lineBase + 2, blockAxes);
                }
                else
                {
                    if (firstLineOfBlock)
                        writer.integer(lineBase + 1, blockAxes);
                    if (firstLineOfSection)
                        writer.text(lineBase + 2, monitorTypeWord(type));
                }
                for (int slot = 0; slot < TRIPLES_PER_LINE && index < sectionEnd; ++slot, ++index)
                {
                    const MonitorSumTerm& term = terms[index];
                    const int nameField = lineBase + FIRST_NAME_OFFSET + slot * FIELDS_PER_TRIPLE;
                    writer.text(nameField, term.name);
                    writer.integer(nameField + 1, term.axes); // resolved: its default is the NEWAXISj
                    writer.realUnlessDefault(nameField + 2, term.coefficient, 1.0);
                }
                lastType = type;
                firstLineOfBlock = false;
                firstLineOfSection = false;
                ++line;
            }
        }
    }
}

std::optional<CardProblem> checkMonitorSumTerms(const MonitorSumBody& body, std::uint32_t accepted,
                                                const char* newAxesName)
{
    const std::vector<MonitorSumTerm>& terms = body.terms;
    if (terms.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " to sum is required", TERM_NAME};
    const std::string digitSet = " must be a set of the digits 1 to 6, each at most once";
    for (const MonitorSumTerm& term : terms)
    {
        if (term.newAxes <= 0 || !base::isComponentSet(term.newAxes))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + digitSet, newAxesName};
        if (const auto problem = monitorTypeProblem(term.type, accepted, TYPE_NAME))
            return problem;
        if (const auto problem = monitoredNameProblem(term.name, TERM_NAME))
            return problem;
        if (term.axes <= 0 || !base::isComponentSet(term.axes))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + digitSet, AXES_NAME};
        if (!std::isfinite(term.coefficient))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               COEFFICIENT_NAME};
    }
    // The results that are added must be of one family: forces and moments, or
    // displacements.
    for (const MonitorSumTerm& term : terms)
    {
        if (isDisplacementType(term.type) != isDisplacementType(terms.front().type))
            return CardProblem{"card.bad-value",
                               "the monitor results of a sum must be all forces and moments or all displacements",
                               TYPE_NAME};
    }
    // The blocks: no component is filled twice, and what a block sums is what it fills.
    unsigned filled = 0;
    std::size_t index = 0;
    while (index < terms.size())
    {
        const std::int32_t blockAxes = terms[index].newAxes;
        const unsigned blockMask = digitMask(blockAxes);
        if ((filled & blockMask) != 0)
            return CardProblem{"card.bad-value",
                               "a component can be filled by one " + std::string(CardProblem::FIELD) + " only",
                               newAxesName};
        filled |= blockMask;
        unsigned summed = 0;
        bool singleInputs = true;
        for (; index < terms.size() && terms[index].newAxes == blockAxes; ++index)
        {
            const unsigned mask = digitMask(terms[index].axes);
            summed |= mask;
            singleInputs = singleInputs && std::popcount(mask) == 1;
        }
        if (std::popcount(blockMask) == 1)
        {
            if (!singleInputs)
                return CardProblem{"card.bad-value",
                                   CardProblem::fieldNamed(newAxesName) + " fills one component, so each " +
                                       std::string(CardProblem::FIELD) + " is one component",
                                   AXES_NAME};
        }
        else if (summed != blockMask)
        {
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed(newAxesName) + " must be the union of the " +
                                   std::string(CardProblem::FIELD) + " that are summed into it",
                               AXES_NAME};
        }
    }
    return std::nullopt;
}

} // namespace oor::model
