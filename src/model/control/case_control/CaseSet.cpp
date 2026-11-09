#include "oor/model/control/case_control/CaseSet.hpp"

#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <unordered_set>

namespace oor::model
{

namespace
{

// The words of the set: Through, By and Except between IDs, All for every ID.
constexpr std::string_view THROUGH = "Through";
constexpr std::string_view BY = "By";
constexpr std::string_view EXCEPT = "Except";
constexpr std::string_view ALL = "All";

// The words of the text, split on commas and blanks.
std::vector<std::string> splitWords(std::string_view text)
{
    std::vector<std::string> words;
    std::string current;
    for (const char character : text)
    {
        if (character == ',' || base::isAsciiSpace(character))
        {
            if (!current.empty())
                words.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(character);
        }
    }
    if (!current.empty())
        words.push_back(current);
    return words;
}

bool isStructureWord(const std::string& word)
{
    return word == THROUGH || word == BY || word == EXCEPT;
}

// Reads the members of an integer set with Through, By and Except.
class IntegerSetReader
{
public:
    explicit IntegerSetReader(const std::vector<std::string>& words) : m_words(words) {}

    std::optional<std::vector<std::int64_t>> read(std::string& problem)
    {
        while (m_next < m_words.size())
        {
            const std::string& word = m_words[m_next];
            if (word == EXCEPT)
            {
                if (m_members.empty())
                {
                    problem = "Except has nothing to exclude from";
                    return std::nullopt;
                }
                applyExclusions();
                m_excepting = true;
                m_exceptLimit = m_hasRange ? m_rangeEnd : *std::max_element(m_members.begin(), m_members.end());
                m_exceptFloor = std::numeric_limits<std::int64_t>::min();
                ++m_next;
                continue;
            }
            if (isStructureWord(word))
            {
                problem = word + " without a first ID";
                return std::nullopt;
            }
            if (!readItem(problem))
                return std::nullopt;
        }
        applyExclusions();
        removeRepeats();
        return std::move(m_members);
    }

private:
    std::optional<std::int64_t> integerAt(std::size_t index, std::string& problem, const char* what) const
    {
        if (index >= m_words.size())
        {
            problem = std::string("a ID is missing after ") + what;
            return std::nullopt;
        }
        const auto value = base::NumberParser::parseInteger(m_words[index]);
        if (!value)
            problem = "\"" + m_words[index] + "\" is not an integer";
        return value;
    }

    // One ID or one "a Through b [By c]" starting at the next word.
    bool readItem(std::string& problem)
    {
        const auto low = integerAt(m_next, problem, "a comma");
        if (!low)
            return false;
        ++m_next;
        std::int64_t high = *low;
        std::int64_t step = 1;
        const bool isRange = m_next < m_words.size() && m_words[m_next] == THROUGH;
        if (isRange)
        {
            const auto end = integerAt(m_next + 1, problem, "Through");
            if (!end)
                return false;
            high = *end;
            m_next += 2;
            if (m_next < m_words.size() && m_words[m_next] == BY)
            {
                const auto by = integerAt(m_next + 1, problem, "By");
                if (!by)
                    return false;
                step = *by;
                m_next += 2;
            }
            if (high < *low || step <= 0)
            {
                problem = std::to_string(*low) + " Through " + std::to_string(high) + " is not an ascending range";
                return false;
            }
            const std::uint64_t count = base::idRangeCount(*low, high, step);
            if (count > CaseSet::MAX_MEMBERS || m_members.size() + count > CaseSet::MAX_MEMBERS)
            {
                problem = "the set has more than " + std::to_string(CaseSet::MAX_MEMBERS) + " members";
                return false;
            }
        }
        if (m_excepting)
        {
            if (*low >= m_exceptFloor && high <= m_exceptLimit)
            {
                m_exclusions.push_back(Exclusion{*low, high, step});
                m_exceptFloor = high;
                return true;
            }
            m_excepting = false; // past the range: the IDs are members again
            applyExclusions();
        }
        for (std::int64_t id = *low; id <= high; id += step)
        {
            m_members.push_back(id);
            // the last one: the next step would pass the end (unsigned, so that no end can overflow)
            if (static_cast<std::uint64_t>(high) - static_cast<std::uint64_t>(id) < static_cast<std::uint64_t>(step))
                break;
        }
        if (isRange)
        {
            m_rangeEnd = high;
            m_hasRange = true;
        }
        return true;
    }

    // Removes the members the IDs after an EXCEPT name, in one pass once the EXCEPT is over: removing them one item at
    // a time would cost the size of the set for each ("1 THRU 5000000 EXCEPT 2, 3, 4, ..." would never end). The
    // exclusions come in ascending order, each starting where the one before ends or later, so the one that may hold a
    // member is the last that starts at or below it, or the one before when the member is their shared end.
    void applyExclusions()
    {
        if (m_exclusions.empty())
            return;
        const auto holds = [](const Exclusion& exclusion, std::int64_t id)
        {
            return id >= exclusion.low && id <= exclusion.high &&
                   (static_cast<std::uint64_t>(id) - static_cast<std::uint64_t>(exclusion.low)) %
                           static_cast<std::uint64_t>(exclusion.step) ==
                       0;
        };
        const auto excluded = [&](std::int64_t id)
        {
            auto after =
                std::upper_bound(m_exclusions.begin(), m_exclusions.end(), id,
                                 [](std::int64_t value, const Exclusion& exclusion) { return value < exclusion.low; });
            if (after == m_exclusions.begin())
                return false;
            --after;
            return holds(*after, id) || (after != m_exclusions.begin() && holds(*std::prev(after), id));
        };
        m_members.erase(std::remove_if(m_members.begin(), m_members.end(), excluded), m_members.end());
        m_exclusions.clear();
    }

    // A member is listed once, where it first appears. IDs usually come in ascending order, which needs no work.
    void removeRepeats()
    {
        if (std::adjacent_find(m_members.begin(), m_members.end(), std::greater_equal<>()) == m_members.end())
            return;
        std::unordered_set<std::int64_t> seen;
        seen.reserve(m_members.size());
        m_members.erase(std::remove_if(m_members.begin(), m_members.end(),
                                       [&seen](std::int64_t id) { return !seen.insert(id).second; }),
                        m_members.end());
    }

    const std::vector<std::string>& m_words;
    std::size_t m_next = 0;
    std::vector<std::int64_t> m_members;
    struct Exclusion
    {
        std::int64_t low = 0;
        std::int64_t high = 0;
        std::int64_t step = 1;
    };
    std::vector<Exclusion> m_exclusions; // what the current EXCEPT names, applied when it ends
    bool m_excepting = false;
    bool m_hasRange = false;
    std::int64_t m_rangeEnd = 0;
    std::int64_t m_exceptLimit = 0;
    std::int64_t m_exceptFloor = 0;
};

} // namespace

CaseSet CaseSet::ofIntegers(std::vector<std::int64_t> values)
{
    CaseSet set;
    set.kind = CaseSetKind::Integers;
    set.integers = std::move(values);
    return set;
}

CaseSet CaseSet::ofReals(std::vector<double> values)
{
    CaseSet set;
    set.kind = CaseSetKind::Reals;
    set.reals = std::move(values);
    return set;
}

CaseSet CaseSet::ofWords(std::vector<std::string> values)
{
    CaseSet set;
    set.kind = CaseSetKind::Words;
    set.words = std::move(values);
    return set;
}

CaseSet CaseSet::ofAll()
{
    CaseSet set;
    set.kind = CaseSetKind::All;
    return set;
}

std::optional<CaseSet> CaseSet::parse(std::string_view text, std::string& problem)
{
    const std::vector<std::string> words = splitWords(text);
    if (words.empty())
    {
        problem = "the set has no members";
        return std::nullopt;
    }
    if (words.size() == 1 && words.front() == ALL)
        return ofAll();

    bool allIntegers = true;
    bool allNumbers = true;
    bool anyInteger = false;
    for (const std::string& word : words)
    {
        const bool isInteger = base::NumberParser::parseInteger(word).has_value();
        anyInteger = anyInteger || isInteger;
        allIntegers = allIntegers && (isInteger || isStructureWord(word));
        allNumbers = allNumbers && (isInteger || base::NumberParser::parseReal(word).has_value());
    }
    if (allIntegers && anyInteger)
    {
        IntegerSetReader reader(words);
        auto members = reader.read(problem);
        if (!members)
            return std::nullopt;
        return ofIntegers(std::move(*members));
    }
    if (allNumbers)
    {
        std::vector<double> values;
        for (const std::string& word : words)
            values.push_back(*base::NumberParser::parseReal(word));
        return ofReals(std::move(values));
    }
    return ofWords(words);
}

std::vector<std::string> CaseSet::items() const
{
    std::vector<std::string> result;
    switch (kind)
    {
    case CaseSetKind::All:
        result.emplace_back(ALL);
        break;
    case CaseSetKind::Words:
        result = words;
        break;
    case CaseSetKind::Reals:
        for (const double value : reals)
            result.push_back(base::NumberFormatter::formatRealFree(value));
        break;
    case CaseSetKind::Integers:
    {
        std::size_t first = 0;
        while (first < integers.size())
        {
            std::size_t last = first;
            while (last + 1 < integers.size() && integers[last + 1] == integers[last] + 1)
                ++last;
            if (last - first >= 2)
            {
                result.push_back(std::to_string(integers[first]) + " Through " + std::to_string(integers[last]));
            }
            else
            {
                for (std::size_t index = first; index <= last; ++index)
                    result.push_back(std::to_string(integers[index]));
            }
            first = last + 1;
        }
        break;
    }
    }
    return result;
}

std::size_t CaseSet::size() const noexcept
{
    switch (kind)
    {
    case CaseSetKind::Integers:
        return integers.size();
    case CaseSetKind::Reals:
        return reals.size();
    case CaseSetKind::Words:
        return words.size();
    case CaseSetKind::All:
        break;
    }
    return 0;
}

bool CaseSet::contains(std::int64_t id) const
{
    return std::find(integers.begin(), integers.end(), id) != integers.end();
}

} // namespace oor::model
