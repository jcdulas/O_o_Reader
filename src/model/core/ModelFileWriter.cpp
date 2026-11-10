#include "oor/model/core/ModelFileWriter.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "oor/deck/writer/FileCommit.hpp"
#include "oor/deck/writer/StagedFiles.hpp"
#include "oor/model/control/ControlRecords.hpp"
#include "oor/model/core/UnknownCardTable.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <utility>

namespace oor::model
{

namespace
{

constexpr char COMMENT_MARK = '#';

// The record of one card: its neutral name, then the fields of its layout.
std::vector<std::string> cardValues(const Model& model, const CardTable& table, std::uint16_t type, std::uint32_t row,
                                    const CardLayouts& layouts, bool omitDefaults = true)
{
    if (type == CardRegistry::UNKNOWN_TYPE)
    {
        // A card the library does not know, or could not read, keeps its name and its fields as they were.
        const auto& unknown = static_cast<const UnknownCardTable&>(table);
        std::vector<std::string> values{std::string(unknown.nameAt(row).view())};
        for (std::string& field : unknown.fieldsAt(row))
            values.push_back(std::move(field));
        return values;
    }
    const std::string_view name = CardRegistry::instance().entry(type).info.name;
    const CardLayout* layout = layouts.find(name);
    if (layout == nullptr)
        throw base::UsageError("the card " + std::string(name) + " has no layout in the card catalogue");
    // Exact reals, the defaults left blank, the sign of a zero kept: what reads back to the same card.
    FieldWriter writer(omitDefaults, 0, false, table.fieldsGovernedByDefaults(model));
    writer.keepSignOfZero(true);
    table.writeCard(row, writer);
    std::vector<std::string> slots = writer.fields();
    std::vector<SlotKind> kinds(slots.size(), SlotKind::Blank);
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        const int field = static_cast<int>(index) + 1;
        if (slots[index].empty())
            continue;
        kinds[index] = writer.holdsReal(field)     ? SlotKind::Real
                       : writer.holdsNumber(field) ? SlotKind::Integer
                                                   : SlotKind::Text;
        if (kinds[index] == SlotKind::Real)
            slots[index] = deck::RecordText::realText(std::move(slots[index]));
    }
    std::vector<std::string> values;
    std::string problem;
    if (!layout->toValues(slots, kinds, values, problem))
        throw base::UsageError("a " + std::string(name) + " " + std::to_string(table.idAt(row)) +
                               " cannot be written as a record: " + problem);
    values.insert(values.begin(), std::string(name));
    return values;
}

// One file being written: the main file, or an included file when the Include structure is kept.
struct FileText
{
    std::string text;
    std::optional<std::filesystem::path> target; // none for toString, and for a second inclusion of a file
};

// The text of a model file, file by file.
class Output final : public RecordOutput
{
public:
    Output(const Model& model, const ModelFileWriteOptions& options, std::optional<std::filesystem::path> mainFile,
           ModelFileWriteReport* report)
        : m_model(model), m_options(options), m_mainFile(std::move(mainFile)), m_report(report)
    {
        m_files.push_back(FileText{{}, m_mainFile});
        m_stack.push_back(0);
        if (m_mainFile)
        {
            m_targets.push_back(*m_mainFile);
            m_seen.insert(m_mainFile->lexically_normal());
        }
    }

    [[nodiscard]] std::vector<FileText>& files() noexcept { return m_files; }

    void line(const std::string& text)
    {
        current() += text;
        current().push_back('\n');
    }

    void comments(const std::string& text) override
    {
        std::size_t start = 0;
        while (start <= text.size())
        {
            const std::size_t end = std::min(text.find('\n', start), text.size());
            current().push_back(COMMENT_MARK);
            current().append(text, start, end - start);
            current().push_back('\n');
            start = end + 1;
        }
    }

    void record(const std::vector<std::string>& values) override { line(deck::RecordText::format(values)); }

    void includeBegin(std::uint32_t fileIndex) override
    {
        if (!m_options.splitIncludes || !m_mainFile)
        {
            m_stack.push_back(m_stack.back()); // the records go on in the same file
            return;
        }
        const std::filesystem::path target = targetOf(fileIndex);
        const std::filesystem::path& parent = m_targets[m_stack.size() - 1];
        // The path relative to the including file, with '/' on every system.
        std::string relative = base::pathToUtf8(target.lexically_relative(parent.parent_path()));
        std::replace(relative.begin(), relative.end(), '\\', '/');
        line(deck::RecordText::format({std::string(deck::ModelFileLexer::INCLUDE_RECORD), relative}));
        // A file included twice is written once: the records of the second inclusion go nowhere.
        const bool first = m_seen.insert(target.lexically_normal()).second;
        m_files.push_back(FileText{{}, first ? std::optional(target) : std::nullopt});
        m_stack.push_back(m_files.size() - 1);
        m_targets.push_back(target);
    }

    void includeEnd() override
    {
        if (m_stack.size() > 1)
        {
            m_stack.pop_back();
            if (m_targets.size() > m_stack.size())
                m_targets.pop_back();
        }
    }

    void dropped(const std::string& what) override
    {
        if (m_report != nullptr)
            m_report->dropped.push_back(what);
    }

private:
    std::string& current() { return m_files[m_stack.back()].text; }

    // Where the model's file `fileIndex` is written: its place under the folder of the main source file, under the
    // folder of the main file written, as a model file.
    [[nodiscard]] std::filesystem::path targetOf(std::uint32_t fileIndex) const
    {
        const std::vector<SourceFileInfo>& sources = m_model.root().files();
        if (sources.empty() || fileIndex >= sources.size())
            throw base::UsageError("an Include names a file the model does not have");
        std::filesystem::path target = deck::outputPathOfInclude(sources[0].path.parent_path(), sources[fileIndex].path,
                                                                 m_mainFile->parent_path(), m_options.includeOutside);
        target.replace_extension(deck::MODEL_FILE_EXTENSION);
        return target;
    }

    const Model& m_model;
    const ModelFileWriteOptions& m_options;
    std::optional<std::filesystem::path> m_mainFile;
    ModelFileWriteReport* m_report;
    std::vector<FileText> m_files;
    std::vector<std::size_t> m_stack;             // the file each open Include writes to
    std::vector<std::filesystem::path> m_targets; // the target of each level of m_stack (with a main file)
    std::set<std::filesystem::path> m_seen;
};

// Counts the records a section would write, to know whether it has a section line.
class RecordCounter final : public RecordOutput
{
public:
    void comments(const std::string& /*text*/) override {}
    void record(const std::vector<std::string>& /*values*/) override { ++m_count; }
    void includeBegin(std::uint32_t /*fileIndex*/) override {}
    void includeEnd() override {}
    void dropped(const std::string& /*what*/) override {}
    [[nodiscard]] std::size_t count() const noexcept { return m_count; }

private:
    std::size_t m_count = 0;
};

class Builder
{
public:
    Builder(const Model& model, const ModelFileWriteOptions& options, Output& out)
        : m_model(model),
          m_options(options),
          m_layouts(options.layouts ? *options.layouts : *CardLayouts::standard()),
          m_out(out)
    {
    }

    void build()
    {
        if (!m_options.headerComment.empty())
            m_out.comments(m_options.headerComment);
        // The comment lines a section starts with are written before its line (where a model file read them from).
        ExecutiveControl executive = m_model.control().executive;
        const FileManagement& files = m_model.control().fileManagement;
        RecordCounter executiveRecords;
        ControlRecords::writeFileAssignments(files, executiveRecords);
        ControlRecords::writeExecutive(executive, executiveRecords);
        if (executiveRecords.count() > 0)
        {
            std::vector<ControlStatement>& statements = executive.entries();
            if (!statements.empty() && statements.front().kind == ControlKind::Comment)
            {
                m_out.comments(sectionComment(statements.front().text));
                statements.erase(statements.begin());
            }
            m_out.line("[executive]");
            ControlRecords::writeFileAssignments(files, m_out);
            ControlRecords::writeExecutive(executive, m_out);
        }
        CaseControl caseControl = m_model.control().caseControl;
        RecordCounter caseRecords;
        ControlRecords::writeCase(caseControl, caseRecords);
        if (caseRecords.count() > 0)
        {
            std::vector<CaseEntry>& entries = caseControl.global().entries;
            if (!entries.empty() && entries.front().kind == EntryKind::Comment)
            {
                m_out.comments(sectionComment(entries.front().text));
                entries.erase(entries.begin());
            }
            m_out.line("[case]");
            ControlRecords::writeCase(caseControl, m_out);
        }
        if (m_model.cardCount() > 0 || !m_model.markers().empty())
            appendModel(m_model, "[model]");
        for (const std::unique_ptr<Model>& part : m_model.parts())
            appendModel(*part, partLine(*part->partInfo()));
    }

private:
    static std::string partLine(const PartInfo& info)
    {
        std::vector<std::string> values{"model", info.kind};
        if (info.kind != "ActivePart" && info.kind != "PassivePart")
            values.push_back(std::to_string(info.id));
        if (info.massId != 0)
            values.push_back("massId=" + std::to_string(info.massId));
        if (info.appends)
            values.emplace_back("append");
        return "[" + deck::RecordText::format(values) + "]";
    }

    // The comment lines of a section as a model file writes them (the structures keep a deck's '$').
    static std::string sectionComment(const std::string& text)
    {
        std::string lines;
        std::size_t start = 0;
        while (start <= text.size())
        {
            const std::size_t end = std::min(text.find('\n', start), text.size());
            std::string_view line = std::string_view(text).substr(start, end - start);
            if (!line.empty() && line.front() == '$')
                line.remove_prefix(1);
            lines += (start > 0 ? "\n" : "") + std::string(line);
            start = end + 1;
        }
        return lines;
    }

    // A model section: the comment lines it starts with (a marker before its first card), its line, its body.
    void appendModel(const Model& model, const std::string& sectionLine)
    {
        const std::vector<SourceMarker>& markers = model.markers();
        std::size_t firstMarker = 0;
        if (!markers.empty() && markers.front().kind == SourceMarkerKind::Comments && markers.front().atKey == 0)
        {
            m_out.comments(markers.front().text);
            firstMarker = 1;
        }
        m_out.line(sectionLine);
        appendBody(model, firstMarker);
    }

    struct CardRef
    {
        std::uint32_t key;
        std::uint16_t type;
        std::uint32_t row;
    };

    // The cards of one model (the main model or a part) in key order, with the comments and markers between them.
    void appendBody(const Model& model, std::size_t firstMarker)
    {
        std::vector<CardRef> cards;
        for (std::uint16_t type = 0; type < CardRegistry::instance().size(); ++type)
        {
            const CardTable* table = model.findTable(type);
            if (table == nullptr)
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row))
                    cards.push_back(CardRef{table->rows().keyAt(row).value, type, row});
            }
        }
        std::sort(cards.begin(), cards.end(),
                  [](const CardRef& left, const CardRef& right) { return left.key < right.key; });
        const std::vector<SourceMarker>& markers = model.markers();
        std::size_t marker = firstMarker;
        const auto markersUpTo = [&](std::uint32_t key)
        {
            for (; marker < markers.size() && markers[marker].atKey <= key; ++marker)
                appendMarker(markers[marker]);
        };
        for (const CardRef& card : cards)
        {
            markersUpTo(card.key);
            if (const std::string comment = model.commentOf(base::CardKey{card.key}); !comment.empty())
                m_out.comments(comment);
            m_out.record(
                cardValues(model, *model.findTable(card.type), card.type, card.row, m_layouts, m_options.omitDefaults));
        }
        for (; marker < markers.size(); ++marker)
            appendMarker(markers[marker]);
    }

    void appendMarker(const SourceMarker& marker)
    {
        switch (marker.kind)
        {
        case SourceMarkerKind::IncludeBegin:
            m_out.includeBegin(marker.fileIndex);
            break;
        case SourceMarkerKind::IncludeEnd:
            m_out.includeEnd();
            break;
        case SourceMarkerKind::Comments:
            m_out.comments(marker.text);
            break;
        }
    }

    const Model& m_model;
    const ModelFileWriteOptions& m_options;
    const CardLayouts& m_layouts;
    Output& m_out;
};

} // namespace

std::string ModelFileWriter::cardToRecord(const Model& model, CardLocation location, const CardLayouts& layouts)
{
    const CardTable* table = model.findTable(location.table);
    if (table == nullptr)
        return {};
    return deck::RecordText::format(cardValues(model, *table, location.table, location.row, layouts));
}

std::string ModelFileWriter::toString(const Model& model, const ModelFileWriteOptions& options,
                                      ModelFileWriteReport* report)
{
    ModelFileWriteOptions flat = options;
    flat.splitIncludes = false;
    Output out(model, flat, std::nullopt, report);
    Builder(model, flat, out).build();
    return std::move(out.files().front().text);
}

void ModelFileWriter::write(const Model& model, const std::filesystem::path& mainFile,
                            const ModelFileWriteOptions& options, ModelFileWriteReport* report,
                            const base::CancellationToken* token)
{
    Output out(model, options, mainFile, report);
    Builder(model, options, out).build();
    deck::StagedFiles staged;
    for (const FileText& file : out.files())
    {
        if (!file.target)
            continue;
        const std::size_t index = staged.open(*file.target);
        staged.append(index, file.text);
    }
    staged.commit(token);
}

} // namespace oor::model
