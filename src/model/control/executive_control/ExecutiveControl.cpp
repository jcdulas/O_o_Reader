#include "oor/model/control/executive_control/ExecutiveControl.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/control/executive_control/SolutionSequences.hpp"
#include "oor/model/control/statement/StatementDefinition.hpp"

namespace oor::model
{

namespace
{

// The keyword of a statement, for a message: its word in the standard dictionary (an edit has no dictionary of a deck).
std::string statementLabel(std::string_view name)
{
    return std::string(name);
}

// The order of the statements the format's examples use; everything else goes after DIAG.
int rankOf(std::string_view name) noexcept
{
    if (name == "JobIdentifier")
        return 0;
    if (name == "SolutionSequence")
        return 1;
    if (name == "TimeLimit")
        return 2;
    if (name == "DiagnosticSelection")
        return 3;
    return 4;
}

// Statements whose place matters: the DMAP boundaries, and ECHOOFF and ECHOON, which act on the statements after them.
// A new one goes after the last statement.
bool isOrdered(std::string_view name) noexcept
{
    return name == "ExecutiveEchoOff" || name == "ExecutiveEchoOn";
}

// COMPILE, ALTER and MALTER are followed by DMAP text.

void requireWord(std::string_view what, std::string_view word)
{
    if (word.empty())
        throw base::UsageError(std::string(what) + " cannot be empty");
    if (word.find_first_of(" \t,'\"$()=") != std::string_view::npos)
        throw base::UsageError(std::string(what) + ": \"" + std::string(word) + "\" is not one word");
}

// Free text, as an ID holds: it may have blanks, not commas, quotes or a comment.
void requireText(std::string_view what, std::string_view text)
{
    if (base::trimAscii(text).empty())
        throw base::UsageError(std::string(what) + " cannot be empty");
}

void requireNumber(std::string_view what, std::string_view word, bool integerOnly)
{
    const bool fits =
        base::NumberParser::parseInteger(word) || (!integerOnly && base::NumberParser::parseReal(word).has_value());
    if (!fits)
        throw base::UsageError(std::string(what) + ": \"" + std::string(word) + "\" is not " +
                               (integerOnly ? "an integer" : "a number"));
}

bool isAllDigits(std::string_view text) noexcept
{
    if (text.empty())
        return false;
    for (const char character : text)
    {
        if (character < '0' || character > '9')
            return false;
    }
    return true;
}

// A solution as a SOL statement holds it: a number, a neutral name of SolutionSequences, or another word in upper
// case (the words of a deck are not case sensitive).
std::string solutionWord(const std::string& word)
{
    if (SolutionSequences::numberOf(word))
        return word;
    return word;
}

} // namespace

std::optional<std::int64_t> ExecutiveControl::solutionNumber() const
{
    const ControlStatement* solution = findLast("SolutionSequence");
    if (solution == nullptr || solution->words.empty())
        return std::nullopt;
    const std::string& word = solution->words.front();
    if (isAllDigits(word))
        return base::NumberParser::parseInteger(word);
    return SolutionSequences::numberOf(word);
}

std::vector<std::int64_t> ExecutiveControl::diagnosticSelectionNumbers() const
{
    std::vector<std::int64_t> numbers;
    for (const ControlStatement* statement : findAll("DiagnosticSelection"))
    {
        for (const std::string& word : statement->words)
        {
            if (const auto number = base::NumberParser::parseInteger(word))
                numbers.push_back(*number);
        }
    }
    return numbers;
}

std::size_t ExecutiveControl::insertionIndex(std::string_view name) const
{
    const std::vector<ControlStatement>& list = entries();
    const bool boundary = isOrdered(name);
    const int rank = rankOf(name);
    int depth = 0;
    std::size_t candidate = 0;
    for (std::size_t index = 0; index < list.size(); ++index)
    {
        const ControlStatement& statement = list[index];
        if (statement.kind == ControlKind::IncludeBegin)
        {
            ++depth;
            continue;
        }
        if (statement.kind == ControlKind::IncludeEnd)
        {
            --depth;
            if (depth == 0 && !boundary)
                candidate = index + 1;
            continue;
        }
        if (depth == 0 && statement.isTyped() && !boundary && rankOf(statement.name) <= rank)
            candidate = index + 1;
    }
    return boundary ? list.size() : candidate;
}

ControlStatement& ExecutiveControl::setJobIdentifier(std::vector<std::string> names)
{
    if (names.empty() || names.size() > 2)
        throw base::UsageError(statementLabel("JobIdentifier") + " takes one or two names");
    for (std::string& name : names)
        requireText("a name of " + statementLabel("JobIdentifier"), name);
    ControlStatement* existing = findLast("JobIdentifier");
    if (existing == nullptr)
    {
        ControlStatement fresh;
        fresh.kind = ControlKind::JobIdentifier;
        fresh.name = "JobIdentifier";
        existing = &insertAt(insertionIndex("JobIdentifier"), std::move(fresh));
        existing->words = std::move(names);
        return *existing;
    }
    if (existing->words != names)
    {
        existing->words = std::move(names);
        existing->markChanged();
    }
    return *existing;
}

ControlStatement& ExecutiveControl::setSolution(std::string solution, std::string second,
                                                std::vector<StatementParameter> options)
{
    requireWord("the solution", solution);
    if (!second.empty())
        requireWord("the second value of " + statementLabel("SolutionSequence"), second);
    std::vector<std::string> words{solutionWord(solution)};
    if (!second.empty())
        words.push_back(solutionWord(second));
    ControlStatement* existing = findLast("SolutionSequence");
    if (existing == nullptr)
    {
        ControlStatement fresh;
        fresh.kind = ControlKind::SolutionSequence;
        fresh.name = "SolutionSequence";
        existing = &insertAt(insertionIndex("SolutionSequence"), std::move(fresh));
        existing->words = std::move(words);
        existing->parameters = std::move(options);
        return *existing;
    }
    if (existing->words != words || existing->parameters != options)
    {
        existing->words = std::move(words);
        existing->parameters = std::move(options);
        existing->markChanged();
    }
    return *existing;
}

ControlStatement& ExecutiveControl::setTimeLimit(std::vector<std::string> numbers)
{
    if (numbers.empty() || numbers.size() > 2)
        throw base::UsageError(statementLabel("TimeLimit") + " takes one or two numbers");
    for (const std::string& number : numbers)
        requireNumber(statementLabel("TimeLimit"), number, false);
    ControlStatement* existing = findLast("TimeLimit");
    if (existing == nullptr)
    {
        ControlStatement fresh;
        fresh.kind = ControlKind::TimeLimit;
        fresh.name = "TimeLimit";
        existing = &insertAt(insertionIndex("TimeLimit"), std::move(fresh));
        existing->words = std::move(numbers);
        return *existing;
    }
    if (existing->words != numbers)
    {
        existing->words = std::move(numbers);
        existing->markChanged();
    }
    return *existing;
}

void ExecutiveControl::setDiagnosticSelection(const std::vector<std::int64_t>& numbers)
{
    if (numbers.empty())
    {
        removeAll("DiagnosticSelection");
        return;
    }
    std::vector<std::string> words;
    for (const std::int64_t number : numbers)
        words.push_back(std::to_string(number));
    const std::vector<const ControlStatement*> existing = findAll("DiagnosticSelection");
    if (existing.size() == 1 && existing.front()->words == words)
        return;
    std::uint32_t keep = 0;
    std::vector<std::uint32_t> surplus;
    for (const ControlStatement* statement : existing)
    {
        if (keep == 0)
            keep = statement->key;
        else
            surplus.push_back(statement->key);
    }
    for (const std::uint32_t key : surplus)
        remove(key);
    ControlStatement* target = keep != 0 ? find(keep) : nullptr;
    if (target == nullptr)
    {
        ControlStatement fresh;
        fresh.kind = ControlKind::DiagnosticSelection;
        fresh.name = "DiagnosticSelection";
        target = &insertAt(insertionIndex("DiagnosticSelection"), std::move(fresh));
    }
    target->words = std::move(words);
    target->markChanged();
}

ControlStatement& ExecutiveControl::addKeyword(std::string_view name, std::vector<StatementParameter> parameters)
{
    const StatementDefinition* definition = executiveStatements().findByName(name);
    if (definition == nullptr)
        throw base::UsageError(std::string(name) + " is not an Executive Control statement");
    if (definition->shape != ControlKind::Keyword)
        throw base::UsageError(statementLabel(definition->name) + " has its own editing functions");
    ControlStatement fresh;
    fresh.kind = ControlKind::Keyword;
    fresh.name = definition->name;
    fresh.parameters = std::move(parameters);
    return insertAt(insertionIndex(definition->name), std::move(fresh));
}

bool ExecutiveControl::removeStatement(std::uint32_t key)
{
    const auto index = indexOf(key);
    if (!index)
        return false;
    return remove(key);
}

} // namespace oor::model
