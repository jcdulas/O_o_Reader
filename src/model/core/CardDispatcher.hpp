#pragma once

// Dispatches the internal slots of compact model records into typed card tables.

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/deck/raw/RawCard.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oor::model
{

// Reads the raw cards of one block into `target`.
class CardDispatcher
{
public:
    CardDispatcher(base::DiagnosticSink& sink, std::string fileName)
        : m_registry(CardRegistry::instance()), m_sink(sink), m_file(std::move(fileName))
    {
    }

    void setTarget(Model& target)
    {
        m_target = &target;
        m_insideExplicitText = false;
    }
    void setFile(std::string fileName) { m_file = std::move(fileName); }

    // True while the cards read are those of an ExplicitTextStart block that no ExplicitTextEnd has closed.
    [[nodiscard]] bool insideExplicitText() const noexcept { return m_insideExplicitText; }

    // Returns false when reading must stop (strict mode, first error).
    bool readBlock(const deck::RawCardBlock& block)
    {
        for (const deck::RawCard& card : block.cards)
        {
            readCard(block, card, typeOf(card.name));
            if (m_sink.shouldStop())
                return false;
        }
        // The comments after the last card of the block go to the next card read, or stay where the file or the
        // section ends (takeCarriedComments).
        for (const std::string_view comment : block.trailingComments())
            appendLine(m_carried, comment);
        return true;
    }

    // The same for cards whose types are known already: `types[i]` is the type of the card i of the block (a record of
    // a model file names its card type by its neutral name; nullopt for a name the library does not know).
    bool readBlock(const deck::RawCardBlock& block, std::span<const std::optional<std::uint16_t>> types)
    {
        for (std::size_t index = 0; index < block.cards.size(); ++index)
        {
            readCard(block, block.cards[index], types[index]);
            if (m_sink.shouldStop())
                return false;
        }
        for (const std::string_view comment : block.trailingComments())
            appendLine(m_carried, comment);
        return true;
    }

    // The comment lines read after the last card and not given to a card yet (joined with '\n'); empty after.
    [[nodiscard]] std::string takeCarriedComments() { return std::exchange(m_carried, std::string{}); }

    // The raw cards read so far, and the length of the comments carried: a stretch of the Bulk Data with comments and
    // no card leaves no trace (its comments are dropped, as when the splitter lexed the stretch itself).
    [[nodiscard]] std::uint64_t cardsRead() const noexcept { return m_cardsRead; }
    [[nodiscard]] std::size_t carriedLength() const noexcept { return m_carried.size(); }
    void dropCarriedAfter(std::size_t length) { m_carried.resize(std::min(length, m_carried.size())); }

private:
    void readCard(const deck::RawCardBlock& block, const deck::RawCard& card, std::optional<std::uint16_t> named)
    {
        ++m_cardsRead;
        Model& target = *m_target;
        ModelKeys keys{target};
        std::optional<base::CardKey> firstKey;
        // The cards between an ExplicitTextStart and an ExplicitTextEnd are for an external explicit solver, whose
        // layouts are not those of the Nastran cards of the same name (SOL 700 hands the text over as it is): they are
        // kept as text and never read as cards of the format.
        const bool isTextEnd = named && named == m_explicitTextEndType;
        const bool passedOn = m_insideExplicitText && !isTextEnd;
        const std::optional<std::uint16_t> type = passedOn ? std::nullopt : named;
        if (type)
        {
            CardTable& table = target.ensureTable(*type);
            FieldReader reader(block, card, m_sink, m_file);
            // Retain the precision of real values parsed from model record text.
            m_digits.clear();
            reader.recordDigits(m_digits);
            if (table.readCards(reader, keys, m_rows))
            {
                for (const std::uint32_t row : m_rows)
                {
                    target.registerRead(CardLocation{*type, row}, table.idAt(row), m_sink, m_file, card.line);
                    table.rows().setSource(row, m_digits);
                }
                firstKey = table.rows().keyAt(m_rows.front());
            }
        }
        if (!firstKey && !(type && m_sink.shouldStop()))
        {
            // A card the library does not know, or one that failed to parse (tolerant mode): kept as text.
            auto& unknown = static_cast<UnknownCardTable&>(target.ensureTable(CardRegistry::UNKNOWN_TYPE));
            firstKey = keys.next();
            if (card.sourceValues.empty())
                unknown.append(*firstKey, card.name, block.fieldsOf(card));
            else
            {
                std::vector<std::string_view> values;
                values.reserve(card.sourceValues.size());
                for (const std::string& value : card.sourceValues)
                    values.push_back(value);
                unknown.append(*firstKey, card.name, values);
            }
        }
        if (isTextEnd)
            m_insideExplicitText = false;
        else if (named && named == m_explicitTextStartType)
            m_insideExplicitText = true;
        if (!firstKey)
            return;
        const base::CardKey key = *firstKey;
        // The comments carried from the end of the block before (a blank line kept them out of this card's chunk)
        // come first.
        std::string joined = takeCarriedComments();
        for (const std::string_view comment : block.commentsOf(card))
            appendLine(joined, comment);
        if (!joined.empty())
            target.comments().push_back(CommentRecord{key, std::move(joined)});
    }

    // Cache exact neutral card-name lookups.
    std::optional<std::uint16_t> typeOf(const deck::CardName& keyword)
    {
        const auto cached = m_types.find(keyword);
        if (cached != m_types.end())
            return cached->second;
        const std::optional<std::uint16_t> type = m_registry.typeIndexOf(keyword.view());
        m_types.emplace(keyword, type);
        return type;
    }

    static void appendLine(std::string& joined, std::string_view line)
    {
        if (!joined.empty())
            joined.push_back('\n');
        joined.append(line);
    }

    // The model's card keys, for the table that reads a card.
    struct ModelKeys final : KeySource
    {
        explicit ModelKeys(Model& model) : target(model) {}
        base::CardKey next() override { return target.newKey(); }
        Model& target;
    };

    const CardRegistry& m_registry;
    std::unordered_map<deck::CardName, std::optional<std::uint16_t>> m_types; // see typeOf
    std::vector<std::uint32_t> m_rows;
    std::vector<std::uint8_t> m_digits; // the significant digits of the card being read
    base::DiagnosticSink& m_sink;
    std::string m_file;
    Model* m_target = nullptr;
    // The card types that open and close a block of text for an external explicit solver.
    std::optional<std::uint16_t> m_explicitTextStartType = m_registry.typeIndexOf("ExplicitTextStart");
    std::optional<std::uint16_t> m_explicitTextEndType = m_registry.typeIndexOf("ExplicitTextEnd");
    bool m_insideExplicitText = false;
    std::string m_carried; // see takeCarriedComments
    std::uint64_t m_cardsRead = 0;
};

} // namespace oor::model
