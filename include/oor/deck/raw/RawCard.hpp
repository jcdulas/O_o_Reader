#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/deck/raw/CardName.hpp"

#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::deck
{

// A model card before it is given a meaning: its name, its fields as text, its comments. The fields are numbered
// the way the format reference numbers them (field 1 is the first data field after the name, continuation lines carry
// on the numbering), so slot i of a card is field i + 1. A blank field is an empty string_view.
//
// A RawCard does not own its text: its views point into the source file (kept mapped by the caller) or into the
// arena of the RawCardBlock that holds it (text placed by the card layout).
struct RawCard
{
    CardName name;
    // The compact source values, when available: tolerant reading retains them if the typed card is refused.
    std::span<const std::string> sourceValues;
    std::uint32_t line = 0;         // first physical line of the card, 1-based
    std::uint32_t fieldBegin = 0;   // index of its first field in RawCardBlock::fields
    std::uint32_t fieldCount = 0;   // number of field slots (a multiple of 4 or 8, trailing blanks included)
    std::uint32_t commentBegin = 0; // index of its first comment line in RawCardBlock::comments
    std::uint32_t commentCount = 0;
};

// The raw cards of one piece of a file, in file order, with the storage that backs them. A file is cut into chunks at
// card boundaries and each chunk is lexed into its own block, in parallel; blocks are then processed in chunk order,
// which keeps results identical for any number of threads.
struct RawCardBlock
{
    std::vector<RawCard> cards;
    std::vector<std::string_view> fields;   // all field slots of all cards
    std::vector<std::string_view> comments; // comment lines, without the leading '$'
    // The comment lines after the last card of the block (no card follows them in the block): the last entries of
    // `comments`. The reader gives them to the next card read, or keeps them where the section ends.
    std::uint32_t trailingCommentCount = 0;
    base::DiagnosticSink diagnostics;  // problems found while lexing this chunk
    std::deque<std::string> ownedText; // text that is not in the source (stable addresses: deque)

    [[nodiscard]] std::span<const std::string_view> fieldsOf(const RawCard& card) const noexcept
    {
        return {fields.data() + card.fieldBegin, card.fieldCount};
    }
    [[nodiscard]] std::span<const std::string_view> commentsOf(const RawCard& card) const noexcept
    {
        return {comments.data() + card.commentBegin, card.commentCount};
    }
    [[nodiscard]] std::span<const std::string_view> trailingComments() const noexcept
    {
        return {comments.data() + comments.size() - trailingCommentCount, trailingCommentCount};
    }
    // Copies `text` into the arena and returns a view that stays valid as long as this block (even when moved).
    std::string_view own(std::string text)
    {
        ownedText.push_back(std::move(text));
        return ownedText.back();
    }
};

} // namespace oor::deck
