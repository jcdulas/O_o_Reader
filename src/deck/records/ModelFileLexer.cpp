#include "oor/deck/records/ModelFileLexer.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/includes/IncludeBudget.hpp"
#include "oor/deck/includes/IncludeTree.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "oor/io/files/MappedFile.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace oor::deck
{

namespace
{

constexpr char COMMENT_MARK = '#';
constexpr std::string_view BYTE_ORDER_MARK = "\xEF\xBB\xBF";
// How often (in lines) a long file checks whether the read was cancelled.
constexpr std::uint32_t CANCEL_CHECK_LINES = 65536;

std::uint64_t fileBytes(const std::filesystem::path& path)
{
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    return error ? 0 : static_cast<std::uint64_t>(size);
}

std::string_view trimLine(std::string_view line) noexcept
{
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
        line.remove_prefix(1);
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
        line.remove_suffix(1);
    return line;
}

std::string_view textOf(const io::MappedFile& mapping)
{
    const auto bytes = mapping.bytes();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

struct ResolvedInclude
{
    std::filesystem::path path;
    bool exists = false;
};

ResolvedInclude resolveInclude(std::string text, const std::filesystem::path& parent,
                               const std::filesystem::path& working, const std::filesystem::path& main)
{
    std::replace(text.begin(), text.end(), '\\', '/');
    const std::filesystem::path wanted = base::pathFromUtf8(text);
    const auto exists = [](const std::filesystem::path& path)
    {
        std::error_code ignored;
        return std::filesystem::is_regular_file(path, ignored);
    };
    if (wanted.is_absolute())
        return {wanted.lexically_normal(), exists(wanted)};
    const std::array candidates{parent / wanted, working / wanted, main / wanted};
    for (const auto& candidate : candidates)
        if (exists(candidate))
            return {candidate.lexically_normal(), true};
    return {candidates.front().lexically_normal(), false};
}

class Lexer
{
public:
    Lexer(ModelText& text, const ReadOptions& options, base::DiagnosticSink& sink, const base::CancellationToken* token)
        : m_text(text), m_options(options), m_sink(sink), m_token(token), m_tree(options.maxIncludeDepth)
    {
    }

    void readMain(const std::filesystem::path& mainFile)
    {
        m_mainDirectory = mainFile.parent_path();
        m_workingDirectory =
            m_options.workingDirectory.empty() ? std::filesystem::current_path() : m_options.workingDirectory;
        (void)m_tree.enter(mainFile);
        (void)m_includes.admit(mainFile, fileBytes(mainFile));
        const io::MappedFile mapping(mainFile); // IoError for the main file
        m_text.files.push_back(SourceFile{mainFile, {}, -1, 0});
        lex(textOf(mapping), 0);
    }

    void readText(std::string_view text, const std::filesystem::path& folder)
    {
        m_mainDirectory = folder;
        m_workingDirectory = folder;
        m_text.files.push_back(SourceFile{folder / "model.oos", {}, -1, 0});
        lex(text, 0);
    }

private:
    // The items of one file. The comment lines no record of the file follows stay at its end.
    void lex(std::string_view text, std::uint32_t fileIndex)
    {
        if (text.starts_with(BYTE_ORDER_MARK))
            text.remove_prefix(BYTE_ORDER_MARK.size());
        std::string comments;
        std::uint32_t lineNumber = 0;
        std::size_t start = 0;
        while (start < text.size() && !m_sink.shouldStop())
        {
            if (m_token != nullptr && lineNumber % CANCEL_CHECK_LINES == 0)
                m_token->throwIfCancelled();
            const std::size_t end = std::min(text.find('\n', start), text.size());
            const std::string_view line = trimLine(text.substr(start, end - start));
            start = end + 1;
            ++lineNumber;
            if (line.empty())
                continue;
            if (line.front() == COMMENT_MARK)
            {
                if (!comments.empty())
                    comments.push_back('\n');
                comments.append(line.substr(1));
                continue;
            }
            readLine(line, fileIndex, lineNumber, comments);
        }
        flushComments(comments, fileIndex, lineNumber);
    }

    void readLine(std::string_view line, std::uint32_t fileIndex, std::uint32_t lineNumber, std::string& comments)
    {
        ModelItem item;
        item.fileIndex = fileIndex;
        item.line = lineNumber;
        std::string problem;
        if (line.front() == '[')
        {
            if (line.back() != ']')
            {
                report(fileIndex, lineNumber, "a section line ends with ']'");
                return;
            }
            item.kind = ModelItemKind::Section;
            if (!RecordText::split(line.substr(1, line.size() - 2), item.values, problem))
            {
                report(fileIndex, lineNumber, problem);
                return;
            }
            // The comments before a section line stay before it, not with the first record of the section.
            flushComments(comments, fileIndex, lineNumber);
            m_text.items.push_back(std::move(item));
            return;
        }
        if (!RecordText::split(line, item.values, problem))
        {
            report(fileIndex, lineNumber, problem);
            return;
        }
        item.comments = std::exchange(comments, std::string{});
        if (item.values.front() == ModelFileLexer::INCLUDE_RECORD)
            include(std::move(item));
        else
            m_text.items.push_back(std::move(item));
    }

    void flushComments(std::string& comments, std::uint32_t fileIndex, std::uint32_t line)
    {
        if (comments.empty())
            return;
        ModelItem item;
        item.kind = ModelItemKind::Comments;
        item.comments = std::exchange(comments, std::string{});
        item.fileIndex = fileIndex;
        item.line = line;
        m_text.items.push_back(std::move(item));
    }

    void report(std::uint32_t fileIndex, std::uint32_t line, const std::string& problem,
                std::string code = "model-file.bad-line")
    {
        m_sink.error(std::move(code), problem + ": the line is skipped",
                     base::SourceLocation{base::pathToUtf8(m_text.files[fileIndex].path), line, "", ""});
    }

    void include(ModelItem item)
    {
        const std::uint32_t parentIndex = item.fileIndex;
        if (item.values.size() != 2 || item.values[1].empty())
        {
            report(parentIndex, item.line, "an Include record names one file: Include;path", "model-file.bad-include");
            return;
        }
        const std::filesystem::path parentFolder = m_text.files[parentIndex].path.parent_path();
        const ResolvedInclude resolved =
            resolveInclude(item.values[1], parentFolder, m_workingDirectory, m_mainDirectory);
        if (!resolved.exists)
        {
            report(parentIndex, item.line, "the included file is not found: " + base::pathToUtf8(resolved.path),
                   "model-file.include-not-found");
            return;
        }
        if (!m_includes.admit(resolved.path, fileBytes(resolved.path)))
        {
            report(parentIndex, item.line,
                   "the model includes its files again more than a read allows (" + m_includes.describe() + ")",
                   "model-file.include-limit");
            return;
        }
        switch (m_tree.enter(resolved.path))
        {
        case IncludeTree::EnterResult::Cycle:
            report(parentIndex, item.line, "Include cycle: " + m_tree.describe(), "model-file.include-cycle");
            return;
        case IncludeTree::EnterResult::TooDeep:
            report(parentIndex, item.line,
                   "Include nesting is deeper than " + std::to_string(m_options.maxIncludeDepth) + " files",
                   "model-file.include-too-deep");
            return;
        case IncludeTree::EnterResult::Entered:
            break;
        }
        const std::uint32_t childIndex = static_cast<std::uint32_t>(m_text.files.size());
        try
        {
            const io::MappedFile mapping(resolved.path);
            m_text.files.push_back(SourceFile{resolved.path, RecordText::format(item.values),
                                              static_cast<std::int32_t>(parentIndex), item.line});
            item.kind = ModelItemKind::IncludeBegin;
            item.values = {item.values[1]};
            item.includedFileIndex = childIndex;
            m_text.items.push_back(std::move(item));
            lex(textOf(mapping), childIndex);
        }
        catch (const base::IoError& error)
        {
            m_tree.leave();
            report(parentIndex, item.line, error.what(), "model-file.include-unreadable");
            return;
        }
        m_tree.leave();
        ModelItem end;
        end.kind = ModelItemKind::IncludeEnd;
        end.fileIndex = childIndex;
        m_text.items.push_back(std::move(end));
    }

    ModelText& m_text;
    const ReadOptions& m_options;
    base::DiagnosticSink& m_sink;
    const base::CancellationToken* m_token;
    IncludeTree m_tree;
    IncludeBudget m_includes;
    std::filesystem::path m_mainDirectory;
    std::filesystem::path m_workingDirectory;
};

} // namespace

bool isModelFile(const std::filesystem::path& path)
{
    return base::toUpperAscii(base::pathToUtf8(path.extension())) == ".OOS";
}

ModelText ModelFileLexer::read(const std::filesystem::path& mainFile, const ReadOptions& options,
                               base::DiagnosticSink& sink, const base::CancellationToken* token)
{
    ModelText text;
    Lexer(text, options, sink, token).readMain(mainFile);
    return text;
}

ModelText ModelFileLexer::readText(std::string_view text, const std::filesystem::path& folder,
                                   base::DiagnosticSink& sink)
{
    ModelText result;
    const ReadOptions options;
    Lexer(result, options, sink, nullptr).readText(text, folder);
    return result;
}

} // namespace oor::deck
