#include "oor/model/control/ControlRecords.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "oor/model/control/case_control/CaseValueParser.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"
#include "oor/model/control/executive_control/SolutionSequences.hpp"
#include "oor/model/control/statement/StatementDefinition.hpp"

#include <optional>
#include <utility>

namespace oor::model
{

namespace
{

// The one File Management statement a model file carries: the external file of a unit.
constexpr std::string_view FILE_ASSIGNMENT = "FileAssignment";

// Comments are stored without their leading record comment character.
std::string commentText(const std::string& text)
{
    return text;
}

void writeComments(const std::string& leading, const std::string& trailing, RecordOutput& out)
{
    std::string lines = leading.empty() ? std::string{} : commentText(leading);
    if (!trailing.empty())
        lines += (lines.empty() ? "" : "\n") + commentText(trailing);
    if (!lines.empty())
        out.comments(lines);
}

// The INCLUDE line a deck writes for an Include record of a model file.
std::string includeLine(const deck::ModelItem& item)
{
    return item.values.empty() ? std::string{} : item.values.front();
}

std::string locationFile(const std::vector<std::string>& fileNames, std::uint32_t fileIndex)
{
    return fileIndex < fileNames.size() ? fileNames[fileIndex] : std::string{};
}

// ------------------------------------------------------------------------------------------------ statement parameters

// A parameter of an executive statement written as one field: `name`, `name=value`, `name(options)=value`, or a quoted
// value with no name.
StatementParameter parameterOf(const std::string& field)
{
    StatementParameter parameter;
    std::string_view text = field;
    const auto takeValue = [&](std::string_view value)
    {
        parameter.hasValue = true;
        if (value.size() >= 2 && (value.front() == '\'' || value.front() == '"') && value.back() == value.front())
        {
            parameter.quote = value.front();
            value = value.substr(1, value.size() - 2);
        }
        parameter.value = std::string(value);
    };
    if (!text.empty() && (text.front() == '\'' || text.front() == '"'))
    {
        takeValue(text);
        return parameter;
    }
    const std::size_t stop = text.find_first_of("(=");
    parameter.name = std::string(base::trimAscii(text.substr(0, stop)));
    if (stop == std::string_view::npos)
        return parameter;
    std::size_t position = stop;
    if (text[position] == '(')
    {
        // The options end at the parenthesis that closes the first one.
        int depth = 0;
        std::size_t close = position;
        for (; close < text.size(); ++close)
        {
            if (text[close] == '(')
                ++depth;
            else if (text[close] == ')' && --depth == 0)
                break;
        }
        parameter.hasOptions = true;
        parameter.options = std::string(text.substr(position + 1, close - position - 1));
        position = std::min(close + 1, text.size());
    }
    if (position < text.size() && text[position] == '=')
        takeValue(base::trimAscii(text.substr(position + 1)));
    return parameter;
}

bool isAllDigits(std::string_view text) noexcept
{
    return !text.empty() && text.find_first_not_of("0123456789") == std::string_view::npos;
}

// ------------------------------------------------------------------------------------------------ executive reading

class ExecutiveRecordReader
{
public:
    ExecutiveRecordReader(const std::vector<std::string>& fileNames, base::DiagnosticSink& sink, FileManagement& files)
        : m_fileNames(fileNames), m_sink(sink), m_files(files)
    {
    }

    ExecutiveControl read(std::span<const deck::ModelItem> items)
    {
        for (const deck::ModelItem& item : items)
        {
            switch (item.kind)
            {
            case deck::ModelItemKind::Record:
                readRecord(item);
                break;
            case deck::ModelItemKind::Comments:
                addComments(item.comments);
                break;
            case deck::ModelItemKind::IncludeBegin:
            {
                ControlStatement marker = statementOf(ControlKind::IncludeBegin, item);
                marker.name = "IncludeFile";
                marker.text = includeLine(item);
                marker.includedFileIndex = item.includedFileIndex;
                add(std::move(marker));
                break;
            }
            case deck::ModelItemKind::IncludeEnd:
                add(statementOf(ControlKind::IncludeEnd, item));
                break;
            case deck::ModelItemKind::Section:
                break;
            }
        }
        return std::move(m_control);
    }

private:
    static ControlStatement statementOf(ControlKind kind, const deck::ModelItem& item)
    {
        ControlStatement statement;
        statement.kind = kind;
        statement.leadingComments = commentText(item.comments);
        return statement;
    }

    void add(ControlStatement statement) { m_control.insertAt(m_control.entries().size(), std::move(statement)); }

    void addComments(const std::string& lines)
    {
        ControlStatement comment;
        comment.kind = ControlKind::Comment;
        comment.text = commentText(lines);
        add(std::move(comment));
    }

    void problem(const deck::ModelItem& item, const std::string& message)
    {
        m_sink.error(
            "model-file.bad-statement", message + ": the statement is skipped",
            base::SourceLocation{locationFile(m_fileNames, item.fileIndex), item.line, item.values.front(), ""});
    }

    void readRecord(const deck::ModelItem& item)
    {
        const std::string& name = item.values.front();
        if (name == FILE_ASSIGNMENT)
        {
            ControlStatement assignment = statementOf(ControlKind::Keyword, item);
            assignment.name = name;
            for (std::size_t index = 1; index < item.values.size(); ++index)
            {
                if (!item.values[index].empty())
                    assignment.parameters.push_back(parameterOf(item.values[index]));
            }
            m_files.insertAt(m_files.entries().size(), std::move(assignment));
            return;
        }
        const StatementDefinition* definition = executiveStatements().findByName(name);
        if (definition == nullptr)
        {
            problem(item, "\"" + name + "\" is not a statement of the executive section");
            return;
        }
        ControlStatement statement = statementOf(definition->shape, item);
        statement.name = definition->name;
        std::vector<std::string> fields(item.values.begin() + 1, item.values.end());
        while (!fields.empty() && fields.back().empty())
            fields.pop_back();
        switch (definition->shape)
        {
        case ControlKind::JobIdentifier:
        case ControlKind::TimeLimit:
        case ControlKind::DiagnosticSelection:
            if (fields.empty())
            {
                problem(item, name + " needs a value");
                return;
            }
            statement.words = std::move(fields);
            break;
        case ControlKind::SolutionSequence:
        {
            if (fields.empty() || fields.front().empty())
            {
                problem(item, name + " is followed by a solution number or name");
                return;
            }
            statement.words.push_back(fields.front());
            std::size_t next = 1;
            // A second value (a number, or the name of a solution) belongs to the solution; a parameter does not.
            if (next < fields.size() && fields[next].find('=') == std::string::npos &&
                (isAllDigits(fields[next]) || SolutionSequences::numberOf(fields[next])))
                statement.words.push_back(fields[next++]);
            for (; next < fields.size(); ++next)
                statement.parameters.push_back(parameterOf(fields[next]));
            break;
        }
        case ControlKind::Keyword:
            for (const std::string& field : fields)
                statement.parameters.push_back(parameterOf(field));
            break;
        default:
            problem(item, name + " has no place in a model file");
            return;
        }
        add(std::move(statement));
    }

    const std::vector<std::string>& m_fileNames;
    base::DiagnosticSink& m_sink;
    FileManagement& m_files;
    ExecutiveControl m_control;
};

// ------------------------------------------------------------------------------------------------ case reading

class CaseRecordReader
{
public:
    CaseRecordReader(const std::vector<std::string>& fileNames, base::DiagnosticSink& sink)
        : m_fileNames(fileNames), m_sink(sink), m_catalogue(CommandCatalogue::instance())
    {
    }

    CaseControl read(std::span<const deck::ModelItem> items)
    {
        for (const deck::ModelItem& item : items)
        {
            switch (item.kind)
            {
            case deck::ModelItemKind::Record:
                readRecord(item);
                break;
            case deck::ModelItemKind::Comments:
            {
                CaseEntry entry = entryOfKind(EntryKind::Comment);
                entry.text = commentText(item.comments);
                addEntry(std::move(entry));
                break;
            }
            case deck::ModelItemKind::IncludeBegin:
            {
                CaseEntry entry = entryOfKind(EntryKind::IncludeBegin);
                entry.leadingComments = commentText(item.comments);
                entry.text = includeLine(item);
                entry.includedFileIndex = item.includedFileIndex;
                addEntry(std::move(entry));
                break;
            }
            case deck::ModelItemKind::IncludeEnd:
                addEntry(entryOfKind(EntryKind::IncludeEnd));
                break;
            case deck::ModelItemKind::Section:
                break;
            }
        }
        return std::move(m_control);
    }

private:
    CaseEntry entryOfKind(EntryKind kind)
    {
        CaseEntry entry;
        entry.kind = kind;
        entry.key = m_control.newKey();
        return entry;
    }

    Subcase& current()
    {
        Subcase* section = &m_control.segments()[m_top];
        for (const std::size_t child : m_path)
            section = &section->children[child];
        return *section;
    }

    void addEntry(CaseEntry entry) { current().entries.push_back(std::move(entry)); }

    void problem(const deck::ModelItem& item, const std::string& message)
    {
        m_sink.error(
            "model-file.bad-command", message + ": the command is skipped",
            base::SourceLocation{locationFile(m_fileNames, item.fileIndex), item.line, item.values.front(), ""});
    }

    void readRecord(const deck::ModelItem& item)
    {
        const CommandDefinition* definition = m_catalogue.findByName(item.values.front());
        if (definition == nullptr)
        {
            problem(item, "\"" + item.values.front() + "\" is not a command of the case section");
            return;
        }
        std::vector<std::string> fields(item.values.begin() + 1, item.values.end());
        while (!fields.empty() && fields.back().empty())
            fields.pop_back();
        std::string message;
        std::optional<CaseEntry> entry = build(*definition, fields, message);
        if (!entry)
        {
            problem(item, item.values.front() + ": " + message);
            return;
        }
        entry->key = m_control.newKey();
        entry->leadingComments = commentText(item.comments);
        place(std::move(*entry), *definition);
    }

    // Puts an entry where it belongs: a header opens a section, anything else goes in the current one (as the deck
    // parser does).
    void place(CaseEntry entry, const CommandDefinition& definition)
    {
        if (entry.kind != EntryKind::Header)
        {
            addEntry(std::move(entry));
            return;
        }
        Subcase section;
        section.key = m_control.newKey();
        section.id = entry.number;
        section.kind = definition.subcaseKind;
        section.header = std::move(entry);
        if (section.kind == SubcaseKind::Step || section.kind == SubcaseKind::Substep)
        {
            if (section.kind == SubcaseKind::Step && !m_path.empty())
                m_path.clear();
            if (section.kind == SubcaseKind::Substep && m_path.size() > 1)
                m_path.pop_back();
            Subcase& parent = current();
            parent.children.push_back(std::move(section));
            m_path.push_back(parent.children.size() - 1);
            return;
        }
        m_control.segments().push_back(std::move(section));
        m_top = m_control.segments().size() - 1;
        m_path.clear();
    }

    static std::string joined(const std::vector<std::string>& fields, std::size_t first)
    {
        std::string text;
        for (std::size_t index = first; index < fields.size(); ++index)
        {
            if (index > first)
                text.push_back(',');
            text += fields[index];
        }
        return text;
    }

    std::optional<CaseEntry> build(const CommandDefinition& definition, const std::vector<std::string>& fields,
                                   std::string& message)
    {
        CaseEntry entry;
        entry.name = definition.name;
        const std::string value = fields.empty() ? std::string{} : fields.front();
        switch (definition.shape)
        {
        case CommandShape::SubcaseHeader:
            entry.kind = EntryKind::Header;
            if (definition.subcaseKind == SubcaseKind::AuxiliaryCase)
                return entry;
            if (const auto number = base::NumberParser::parseInteger(value))
            {
                entry.number = *number;
                return entry;
            }
            message = "the number of the section is missing or is not an integer";
            return std::nullopt;
        case CommandShape::BlockHeader:
            entry.options.assign(fields.empty() ? fields.begin() : fields.begin() + 1, fields.end());
            entry.variant = definition.blockKind;
            if (entry.variant.empty() && !entry.options.empty())
                entry.variant = entry.options.front() == "CurveOutput" ? "CurvePlot" : entry.options.front();
            entry.kind = entry.variant.empty() ? EntryKind::Command : EntryKind::OutputBlock;
            return entry;
        case CommandShape::SetDefinition:
        {
            entry.kind = EntryKind::Set;
            const auto number = base::NumberParser::parseInteger(value);
            if (!number || *number <= 0)
            {
                message = "the set number is not a positive integer";
                return std::nullopt;
            }
            auto members = CaseSet::parse(joined(fields, 1), message);
            if (!members)
                return std::nullopt;
            entry.number = *number;
            entry.set = std::move(*members);
            return entry;
        }
        case CommandShape::Parameter:
            return buildParameter(entry, fields, message);
        default:
            break;
        }
        entry.kind = definition.shape == CommandShape::Output ? EntryKind::Output : EntryKind::Command;
        for (std::size_t index = 1; index < fields.size(); ++index)
        {
            if (fields[index].empty())
                continue;
            if (entry.variant.empty() && definition.hasVariant(fields[index]))
                entry.variant = fields[index];
            else
                entry.options.push_back(fields[index]);
        }
        const auto parsed = CaseValueParser::parse(definition, value, message);
        if (!parsed)
            return std::nullopt;
        entry.value = *parsed;
        return entry;
    }

    // SubcaseParameter;name;value: the value is an integer, a real, two reals (a complex value), a word or a text.
    static std::optional<CaseEntry> buildParameter(CaseEntry& entry, const std::vector<std::string>& fields,
                                                   std::string& message)
    {
        entry.kind = EntryKind::Param;
        if (fields.empty() || fields.front().empty())
        {
            message = "the name of the parameter is missing";
            return std::nullopt;
        }
        entry.variant = fields.front();
        if (fields.size() < 2 || fields[1].empty())
            return entry;
        const std::string value = joined(fields, 1);
        if (const auto integer = base::NumberParser::parseInteger(value))
            entry.value = CaseValue::ofInteger(*integer);
        else if (const auto real = base::NumberParser::parseReal(value))
            entry.value = CaseValue::ofReal(*real);
        else if (const std::size_t comma = value.find(','); comma != std::string::npos &&
                                                            value.find(',', comma + 1) == std::string::npos &&
                                                            base::NumberParser::parseReal(value.substr(0, comma)) &&
                                                            base::NumberParser::parseReal(value.substr(comma + 1)))
            entry.value = CaseValue::ofReals({*base::NumberParser::parseReal(value.substr(0, comma)),
                                              *base::NumberParser::parseReal(value.substr(comma + 1))});
        else if (value.find_first_of(", ") == std::string::npos)
            entry.value = CaseValue::ofKeyword(value);
        else
            entry.value = CaseValue::ofText(value);
        return entry;
    }

    const std::vector<std::string>& m_fileNames;
    base::DiagnosticSink& m_sink;
    const CommandCatalogue& m_catalogue;
    CaseControl m_control;
    std::size_t m_top = 0;
    std::vector<std::size_t> m_path;
};

// ------------------------------------------------------------------------------------------------ writing

// The statements of the compiler and of the alteration of a solution sequence: a model file has no place for them
// (docs/model-format.md, [executive]), like the sequence code they frame.

// The value of a case command as a model file writes it: the reals in the syntax of the format (0.001, 5.0E-5).
std::string caseValueText(const CaseValue& value)
{
    if (value.kind != CaseValueKind::Real && value.kind != CaseValueKind::RealList)
        return value.format();
    std::string text;
    for (const std::string& item : value.items())
    {
        if (!text.empty())
            text.push_back(',');
        text += deck::RecordText::realText(item);
    }
    return text;
}

void writeStatement(const ControlStatement& statement, RecordOutput& out)
{
    switch (statement.kind)
    {
    case ControlKind::IncludeBegin:
        writeComments(statement.leadingComments, {}, out);
        out.includeBegin(statement.includedFileIndex);
        return;
    case ControlKind::IncludeEnd:
        out.includeEnd();
        return;
    case ControlKind::Comment:
        out.comments(commentText(statement.text));
        return;
    default:
        break;
    }
    // The comments of a statement a model file has no place for stay, with the record that follows.
    writeComments(statement.leadingComments, statement.trailingComment, out);
    const bool kept = (statement.kind == ControlKind::JobIdentifier || statement.kind == ControlKind::TimeLimit ||
                       statement.kind == ControlKind::DiagnosticSelection ||
                       statement.kind == ControlKind::SolutionSequence || statement.kind == ControlKind::Keyword);
    if (!kept)
    {
        out.dropped("executive statement " + (statement.name.empty() ? std::string("(text)") : statement.name) +
                    (statement.original.empty() ? std::string{} : ": " + statement.original));
        return;
    }
    std::vector<std::string> values{statement.name};
    values.insert(values.end(), statement.words.begin(), statement.words.end());
    for (const StatementParameter& parameter : statement.parameters)
        values.push_back(parameter.format());
    out.record(values);
}

void writeEntry(const CaseEntry& entry, std::int64_t number, RecordOutput& out)
{
    switch (entry.kind)
    {
    case EntryKind::IncludeBegin:
        writeComments(entry.leadingComments, {}, out);
        out.includeBegin(entry.includedFileIndex);
        return;
    case EntryKind::IncludeEnd:
        out.includeEnd();
        return;
    case EntryKind::Comment:
        out.comments(commentText(entry.text));
        return;
    case EntryKind::Raw:
        out.dropped("case command kept as text: " + entry.text);
        return;
    default:
        break;
    }
    writeComments(entry.leadingComments, entry.trailingComment, out);
    std::vector<std::string> values{entry.name};
    switch (entry.kind)
    {
    case EntryKind::Header:
        if (number != 0 || !entry.name.empty())
            values.push_back(std::to_string(number));
        break;
    case EntryKind::Set:
        values.push_back(std::to_string(entry.number));
        for (const std::string& item : entry.set.items())
        {
            // "1 Through 5" is three members of the record.
            std::size_t start = 0;
            while (start < item.size())
            {
                const std::size_t end = std::min(item.find(' ', start), item.size());
                if (end > start)
                    values.push_back(item.substr(start, end - start));
                start = end + 1;
            }
        }
        break;
    case EntryKind::Param:
        values.push_back(entry.variant);
        values.push_back(caseValueText(entry.value));
        break;
    case EntryKind::OutputBlock:
        values.emplace_back();
        values.insert(values.end(), entry.options.begin(), entry.options.end());
        break;
    default:
        values.push_back(caseValueText(entry.value));
        if (!entry.variant.empty())
            values.push_back(entry.variant);
        values.insert(values.end(), entry.options.begin(), entry.options.end());
        break;
    }
    out.record(values);
}

void writeSection(const Subcase& section, RecordOutput& out)
{
    if (section.kind != SubcaseKind::Global)
    {
        CaseEntry header = section.header;
        header.name = std::string(subcaseCommandName(section.kind));
        if (section.kind == SubcaseKind::AuxiliaryCase)
        {
            writeComments(header.leadingComments, header.trailingComment, out);
            out.record({header.name});
        }
        else
        {
            writeEntry(header, section.id, out);
        }
    }
    for (const CaseEntry& entry : section.entries)
        writeEntry(entry, entry.number, out);
    for (const Subcase& child : section.children)
        writeSection(child, out);
}

} // namespace

ExecutiveControl ControlRecords::readExecutive(std::span<const deck::ModelItem> items,
                                               const std::vector<std::string>& fileNames, base::DiagnosticSink& sink,
                                               FileManagement& files)
{
    return ExecutiveRecordReader(fileNames, sink, files).read(items);
}

CaseControl ControlRecords::readCase(std::span<const deck::ModelItem> items, const std::vector<std::string>& fileNames,
                                     base::DiagnosticSink& sink)
{
    return CaseRecordReader(fileNames, sink).read(items);
}

void ControlRecords::writeExecutive(const ExecutiveControl& control, RecordOutput& out)
{
    for (const ControlStatement& statement : control.entries())
        writeStatement(statement, out);
}

void ControlRecords::writeFileAssignments(const FileManagement& files, RecordOutput& out)
{
    for (const ControlStatement& statement : files.entries())
    {
        if (statement.kind == ControlKind::Comment)
        {
            out.comments(commentText(statement.text)); // they go with the next record
            continue;
        }
        if (statement.kind == ControlKind::IncludeBegin || statement.kind == ControlKind::IncludeEnd)
            continue; // the Include structure of a section a model file does not have
        // The comments of a statement a model file has no place for stay, with the record that follows.
        writeComments(statement.leadingComments, statement.trailingComment, out);
        if (statement.name != FILE_ASSIGNMENT || statement.kind != ControlKind::Keyword)
        {
            out.dropped("file management statement " +
                        (statement.original.empty() ? statement.name : statement.original));
            continue;
        }
        std::vector<std::string> values{statement.name};
        for (const StatementParameter& parameter : statement.parameters)
            values.push_back(parameter.format());
        out.record(values);
    }
}

void ControlRecords::writeCase(const CaseControl& control, RecordOutput& out)
{
    for (const Subcase& segment : control.segments())
        writeSection(segment, out);
}

} // namespace oor::model
