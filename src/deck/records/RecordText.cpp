#include "oor/deck/records/RecordText.hpp"

#include "oor/base/errors/Errors.hpp"

namespace oor::deck
{

namespace
{

constexpr char SEPARATOR = ';';
constexpr char QUOTE = '"';

bool isBlankCharacter(char character) noexcept
{
    return character == ' ' || character == '\t';
}

std::string_view trimBlanks(std::string_view text) noexcept
{
    while (!text.empty() && isBlankCharacter(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && (isBlankCharacter(text.back()) || text.back() == '\r'))
        text.remove_suffix(1);
    return text;
}

} // namespace

bool RecordText::split(std::string_view line, std::vector<std::string>& values, std::string& problem)
{
    values.clear();
    std::size_t position = 0;
    while (true)
    {
        // Spaces before a value are not part of it.
        while (position < line.size() && isBlankCharacter(line[position]))
            ++position;
        if (position < line.size() && line[position] == QUOTE)
        {
            std::string value;
            ++position;
            bool closed = false;
            while (position < line.size())
            {
                if (line[position] == QUOTE)
                {
                    if (position + 1 < line.size() && line[position + 1] == QUOTE)
                    {
                        value.push_back(QUOTE);
                        position += 2;
                        continue;
                    }
                    closed = true;
                    ++position;
                    break;
                }
                value.push_back(line[position]);
                ++position;
            }
            if (!closed)
            {
                problem = "a quote is not closed";
                return false;
            }
            while (position < line.size() && (isBlankCharacter(line[position]) || line[position] == '\r'))
                ++position;
            if (position < line.size() && line[position] != SEPARATOR)
            {
                problem = "a quoted value is followed by text before the next ';'";
                return false;
            }
            values.push_back(std::move(value));
        }
        else
        {
            const std::size_t end = std::min(line.find(SEPARATOR, position), line.size());
            values.emplace_back(trimBlanks(line.substr(position, end - position)));
            position = end;
        }
        if (position >= line.size())
            return true;
        ++position; // the separator
    }
}

bool RecordText::needsQuotes(std::string_view value) noexcept
{
    if (value.empty())
        return false;
    if (isBlankCharacter(value.front()) || isBlankCharacter(value.back()) || value.back() == '\r')
        return true;
    return value.find_first_of(";\"") != std::string_view::npos;
}

std::string RecordText::format(const std::vector<std::string>& values)
{
    for (const std::string& value : values)
        if (value.find_first_of("\r\n") != std::string::npos)
            throw base::UsageError("a model record value cannot contain CR or LF");
    std::size_t count = values.size();
    while (count > 0 && values[count - 1].empty())
        --count;
    std::string line;
    for (std::size_t index = 0; index < count; ++index)
    {
        if (index > 0)
            line.push_back(SEPARATOR);
        const std::string& value = values[index];
        const bool reservedPrefix = index == 0 && !value.empty() && (value.front() == '#' || value.front() == '[');
        if (!reservedPrefix && !needsQuotes(value))
        {
            line += value;
            continue;
        }
        line.push_back(QUOTE);
        for (const char character : value)
        {
            if (character == QUOTE)
                line.push_back(QUOTE);
            line.push_back(character);
        }
        line.push_back(QUOTE);
    }
    return line;
}

std::string RecordText::realText(std::string text)
{
    if (text.starts_with('.'))
        text.insert(text.begin(), '0');
    else if (text.starts_with("-.") || text.starts_with("+."))
        text.insert(text.begin() + 1, '0');
    return text;
}

} // namespace oor::deck
