#include "oor/model/core/ModelFileReader.hpp"

#include "CardDispatcher.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/control/ControlRecords.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace oor::model
{

namespace
{

// The section names of a model file.
constexpr std::string_view EXECUTIVE_SECTION = "executive";
constexpr std::string_view CASE_SECTION = "case";
constexpr std::string_view MODEL_SECTION = "model";
// The options of a part line after its kind and number: [model;SuperelementPart;5;massId=300], [model;ModuleSection;
// 7;append].
constexpr std::string_view PART_MASS_OPTION = "massId=";
constexpr std::string_view PART_APPEND_OPTION = "append";
// The part kinds that have no number (the sub-models of a transfer path analysis).
constexpr std::string_view ACTIVE_PART = "ActivePart";
constexpr std::string_view PASSIVE_PART = "PassivePart";

// Cards are handed to the tables in blocks of this many records.
constexpr std::size_t BLOCK_RECORDS = 4096;
// A raw card has its slots in lines of this many (FieldReader numbering), as a deck's lexer gives them.
constexpr std::uint32_t SLOTS_PER_LINE = 8;

enum class Section : std::uint8_t
{
    None,
    Executive,
    Case,
    Model
};

class Builder
{
public:
    Builder(Model& model, const ModelReadOptions& options, const base::CancellationToken* token,
            std::vector<std::string> fileNames)
        : m_model(model),
          m_token(token),
          m_layouts(options.layouts ? *options.layouts : *CardLayouts::standard()),
          m_registry(CardRegistry::instance()),
          m_sink(model.diagnostics()),
          m_fileNames(std::move(fileNames)),
          m_dispatcher(m_sink, m_fileNames.empty() ? std::string{} : m_fileNames.front())
    {
        m_target = &model;
        m_dispatcher.setTarget(model);
    }

    void run(const deck::ModelText& text)
    {
        std::vector<Section> includeSections;             // the section each open Include began in
        const deck::ModelItem* sectionComments = nullptr; // the comment lines before a section line
        for (std::size_t index = 0; index < text.items.size(); ++index)
        {
            const deck::ModelItem& item = text.items[index];
            if (m_sink.shouldStop())
                return;
            if (m_token != nullptr)
                m_token->throwIfCancelled();
            const bool beforeSection =
                index + 1 < text.items.size() && text.items[index + 1].kind == deck::ModelItemKind::Section;
            switch (item.kind)
            {
            case deck::ModelItemKind::Section:
                flush();
                openSection(item);
                // The comment lines before a section line are the first item of the section (the writer puts them
                // back before its line).
                if (sectionComments != nullptr)
                    sectionItem(m_section, *std::exchange(sectionComments, nullptr));
                continue;
            case deck::ModelItemKind::Comments:
                if (beforeSection)
                {
                    sectionComments = &item;
                    continue;
                }
                break;
            case deck::ModelItemKind::IncludeBegin:
                includeSections.push_back(m_section);
                break;
            case deck::ModelItemKind::IncludeEnd:
                if (!includeSections.empty())
                {
                    const Section opened = includeSections.back();
                    includeSections.pop_back();
                    sectionItem(opened, item);
                    continue;
                }
                break;
            default:
                break;
            }
            sectionItem(m_section, item);
        }
        flush();
        finishControl();
    }

private:
    void sectionItem(Section section, const deck::ModelItem& item)
    {
        switch (section)
        {
        case Section::Executive:
            m_executiveItems.push_back(item);
            return;
        case Section::Case:
            m_caseItems.push_back(item);
            return;
        case Section::None:
        case Section::Model:
            modelItem(item);
            return;
        }
    }

    void openSection(const deck::ModelItem& item)
    {
        const std::string& name = item.values.front();
        if (name == EXECUTIVE_SECTION)
            m_section = Section::Executive;
        else if (name == CASE_SECTION)
            m_section = Section::Case;
        else if (name == MODEL_SECTION)
            openModel(item);
        else
            error(item, "model-file.unknown-section",
                  "unknown section [" + name + "] (executive, case, model or model;Kind;id)");
    }

    // [model] is the main model again; [model;Kind;id;...] opens a part.
    void openModel(const deck::ModelItem& item)
    {
        m_section = Section::Model;
        if (item.values.size() == 1)
        {
            setTarget(m_model);
            return;
        }
        PartInfo info;
        info.kind = item.values[1];
        std::size_t next = 2;
        if (info.kind != ACTIVE_PART && info.kind != PASSIVE_PART)
        {
            const auto id = next < item.values.size() ? base::NumberParser::parseInteger(item.values[next])
                                                      : std::optional<std::int64_t>{};
            if (!id)
            {
                error(item, "model-file.bad-section", "a part line gives the kind and the number: [model;Kind;id]");
                setTarget(m_model);
                return;
            }
            info.id = *id;
            ++next;
        }
        for (; next < item.values.size(); ++next)
        {
            const std::string& option = item.values[next];
            if (option.starts_with(PART_MASS_OPTION))
                info.massId = base::NumberParser::parseInteger(option.substr(PART_MASS_OPTION.size())).value_or(0);
            else if (option == PART_APPEND_OPTION)
                info.appends = true;
            else if (!option.empty())
                error(item, "model-file.bad-section", "unknown option of a part line: " + option);
        }
        setTarget(m_model.addPart(std::move(info)));
    }

    void setTarget(Model& target)
    {
        m_target = &target;
        m_dispatcher.setTarget(target);
    }

    void modelItem(const deck::ModelItem& item)
    {
        switch (item.kind)
        {
        case deck::ModelItemKind::Record:
            addRecord(item);
            return;
        case deck::ModelItemKind::Comments:
            flush();
            m_target->markers().push_back(
                SourceMarker{m_target->nextKeyValue(), SourceMarkerKind::Comments, item.comments, 0});
            return;
        case deck::ModelItemKind::IncludeBegin:
            flush();
            if (!item.comments.empty())
                m_target->markers().push_back(
                    SourceMarker{m_target->nextKeyValue(), SourceMarkerKind::Comments, item.comments, 0});
            m_target->markers().push_back(SourceMarker{m_target->nextKeyValue(), SourceMarkerKind::IncludeBegin,
                                                       includeText(item), item.includedFileIndex});
            return;
        case deck::ModelItemKind::IncludeEnd:
            flush();
            m_target->markers().push_back(SourceMarker{m_target->nextKeyValue(), SourceMarkerKind::IncludeEnd, {}, 0});
            return;
        case deck::ModelItemKind::Section:
            return;
        }
    }

    static std::string includeText(const deck::ModelItem& item)
    {
        return item.values.empty() ? std::string{} : item.values.front();
    }

    // Turns a card record into a raw card of the current block.
    void addRecord(const deck::ModelItem& item)
    {
        const std::string& name = item.values.front();
        const std::optional<std::uint16_t> type = m_registry.typeIndexOf(name);
        std::optional<deck::CardName> cardName;
        std::vector<std::string> slots;
        if (type && *type != CardRegistry::UNKNOWN_TYPE)
        {
            const CardLayout* layout = m_layouts.find(name);
            if (layout == nullptr)
            {
                error(item, "model-file.no-layout", "the card " + name + " has no layout in the card catalogue");
                return;
            }
            std::string problem;
            const std::span<const std::string> values(item.values.data() + 1, item.values.size() - 1);
            if (!layout->toSlots(values, slots, problem))
            {
                error(item, "model-file.bad-record", problem);
                return;
            }
            cardName = deck::CardName::fromText(name);
        }
        else
        {
            cardName = deck::CardName::fromText(name);
            if (!cardName || cardName->empty())
            {
                error(item, "model-file.unknown-card", "unknown card " + name);
                return;
            }
            slots.assign(item.values.begin() + 1, item.values.end());
        }
        if (m_block.cards.empty())
        {
            m_blockFile = item.fileIndex;
        }
        else if (m_blockFile != item.fileIndex)
        {
            flush();
            m_blockFile = item.fileIndex;
        }
        appendCard(*cardName, item, slots);
        m_types.push_back(type && *type != CardRegistry::UNKNOWN_TYPE ? type : std::nullopt);
        if (m_block.cards.size() >= BLOCK_RECORDS)
            flush();
    }

    void appendCard(const deck::CardName& name, const deck::ModelItem& item, const std::vector<std::string>& slots)
    {
        deck::RawCard card;
        card.name = name;
        card.sourceValues = std::span<const std::string>(item.values).subspan(1);
        card.line = item.line;
        card.fieldBegin = static_cast<std::uint32_t>(m_block.fields.size());
        // The slot texts of the record, in one string of the block's arena, and a view of each.
        std::string text;
        for (const std::string& slot : slots)
            text += slot;
        const std::string_view owned = m_block.own(std::move(text));
        std::size_t offset = 0;
        for (const std::string& slot : slots)
        {
            m_block.fields.push_back(owned.substr(offset, slot.size()));
            offset += slot.size();
        }
        // Whole lines of slots, as a deck's lexer gives them.
        const auto used = static_cast<std::uint32_t>(slots.size());
        const std::uint32_t count =
            std::max(SLOTS_PER_LINE, (used + SLOTS_PER_LINE - 1) / SLOTS_PER_LINE * SLOTS_PER_LINE);
        m_block.fields.resize(card.fieldBegin + count);
        card.fieldCount = count;
        card.commentBegin = static_cast<std::uint32_t>(m_block.comments.size());
        if (!item.comments.empty())
        {
            const std::string_view comments = m_block.own(item.comments);
            std::size_t start = 0;
            while (start <= comments.size())
            {
                const std::size_t end = std::min(comments.find('\n', start), comments.size());
                m_block.comments.push_back(comments.substr(start, end - start));
                start = end + 1;
            }
        }
        card.commentCount = static_cast<std::uint32_t>(m_block.comments.size()) - card.commentBegin;
        m_block.cards.push_back(card);
    }

    // Hands the cards of the current block to their tables.
    void flush()
    {
        if (m_block.cards.empty())
            return;
        m_dispatcher.setFile(m_blockFile < m_fileNames.size() ? m_fileNames[m_blockFile] : std::string{});
        (void)m_dispatcher.readBlock(m_block, m_types);
        deck::RawCardBlock fresh;
        m_block = std::move(fresh);
        m_types.clear();
    }

    void finishControl()
    {
        ControlSections& control = m_model.control();
        control.fileManagement = FileManagement();
        control.executive =
            ControlRecords::readExecutive(m_executiveItems, m_fileNames, m_sink, control.fileManagement);
        control.caseControl = ControlRecords::readCase(m_caseItems, m_fileNames, m_sink);
    }

    void error(const deck::ModelItem& item, const char* code, const std::string& message)
    {
        const std::string card = item.values.empty() ? std::string{} : item.values.front();
        m_sink.error(code, message,
                     base::SourceLocation{item.fileIndex < m_fileNames.size() ? m_fileNames[item.fileIndex] : "",
                                          item.line, card, ""});
    }

    Model& m_model;
    const base::CancellationToken* m_token;
    const CardLayouts& m_layouts;
    const CardRegistry& m_registry;
    base::DiagnosticSink& m_sink;
    std::vector<std::string> m_fileNames;
    CardDispatcher m_dispatcher;
    Model* m_target = nullptr;
    Section m_section = Section::None;
    std::vector<deck::ModelItem> m_executiveItems;
    std::vector<deck::ModelItem> m_caseItems;
    deck::RawCardBlock m_block;
    std::vector<std::optional<std::uint16_t>> m_types; // the type of each card of m_block
    std::uint32_t m_blockFile = 0;
};

Model build(Model model, deck::ModelText text, const ModelReadOptions& options, const base::CancellationToken* token)
{
    std::vector<std::string> fileNames;
    for (const deck::SourceFile& file : text.files)
    {
        fileNames.push_back(base::pathToUtf8(file.path));
        model.files().push_back(SourceFileInfo{file.path, file.includeTextAsWritten, file.parent, file.includeLine});
    }
    Builder(model, options, token, std::move(fileNames)).run(text);
    base::DiagnosticSink& sink = model.diagnostics();
    if (!sink.shouldStop())
        model.applyDefaults();
    if (options.strict && sink.hasErrors())
        throw base::DeckError(*sink.firstError());
    return model;
}

Model emptyModel(const ModelReadOptions& options)
{
    Model model;
    model.setStrict(options.strict);
    return model;
}

} // namespace

Model ModelFileReader::read(const std::filesystem::path& path, const ModelReadOptions& options,
                            const base::CancellationToken* token)
{
    Model model = emptyModel(options);
    deck::ModelText text = deck::ModelFileLexer::read(path, options.files, model.diagnostics(), token);
    return build(std::move(model), std::move(text), options, token);
}

Model ModelFileReader::fromItems(deck::ModelText text, const ModelReadOptions& options,
                                 const base::CancellationToken* token)
{
    return build(emptyModel(options), std::move(text), options, token);
}

} // namespace oor::model
