#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class CaseSetKind : std::uint8_t
{
    Integers, // IDs: 1, 5 Through 9 Except 7 (expanded, in order, no repeats)
    Reals,    // frequencies or times: 1.0, 2.0
    All,      // the All selection
    Words     // anything else, kept as the items written: names (M1, M2) and ID/component pairs (101/T1, 5/0)
};

// The members of a SetDefinition entry; its identifier and surrounding record belong to CaseEntry.
//
// An integer set is stored expanded: Through (with an optional By) and Except are applied when the set is read, and a run
// of three or more consecutive IDs is compressed back to "a Through b" when it is written. Except follows the format
// reference: the IDs after it are removed while they stay in ascending order and do not pass the end of the Through range
// before it; the first one that does is a member again ("1 Through 10 Except 4, 5, 11" holds 1 2 3 6 7 8 9 10 11).
struct CaseSet
{
    CaseSetKind kind = CaseSetKind::Integers;
    std::vector<std::int64_t> integers;
    std::vector<double> reals;
    std::vector<std::string> words;

    // Most members a set may expand to; a bigger one is refused (a hostile "1 Through 2000000000").
    static constexpr std::size_t MAX_MEMBERS = 5'000'000;

    // Reads member text using exact Through, By, Except and All words. Returns nothing, with `problem` set, on error.
    [[nodiscard]] static std::optional<CaseSet> parse(std::string_view text, std::string& problem);

    [[nodiscard]] static CaseSet ofIntegers(std::vector<std::int64_t> values);
    [[nodiscard]] static CaseSet ofReals(std::vector<double> values);
    [[nodiscard]] static CaseSet ofWords(std::vector<std::string> values);
    [[nodiscard]] static CaseSet ofAll();

    // The members as written: one item per member, with runs of consecutive IDs as "a Through b".
    [[nodiscard]] std::vector<std::string> items() const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool contains(std::int64_t id) const;
    [[nodiscard]] bool operator==(const CaseSet&) const = default;
};

} // namespace oor::model
