#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The continuation lines of DVPREL2, DVMREL2, DVCREL2, DRESP2 and DRESP3 are groups: a keyword in the first field of a
// line ("DESVAR", "DTABLE", "DRESP1", ...) and the values it introduces in the seven fields after it and on the lines
// after it that have a blank keyword. Each card has its own list of the keywords it allows (a GroupKeyword for each, in
// the order the format reference requires them), and this reads, writes and checks the groups by it. It describes no
// card: what a keyword means, and which card the numbers of a group name, stays in the card.

enum class GroupContent : std::uint8_t
{
    Integers,   // positive integers, seven to a line (DESVAR, DRESP1, DVPREL1, ...)
    Words,      // names, seven to a line (the labels of a DTABLE)
    Components, // a grid point and a component, three pairs to a line (DNODE)
    Text,       // text with blanks, one line of text to a line of the card (USRDATA)
};

struct GroupKeyword
{
    std::string_view keyword; // the key word of the group: a neutral card name ("DesignVar") or a word ("DesignNodes")
    GroupContent content;
    const char* valueName; // the name of one value, for the messages and the diagnostics
};

// One group of a card. Integers and Components keep their numbers in `ids` (a pair of Components is two consecutive
// numbers), Words and Text keep their strings in `words`.
struct DesignGroup
{
    std::string keyword; // the key word of the table (a neutral card name or a word of the format)
    std::vector<base::EntityId> ids;
    std::vector<std::string> words;

    [[nodiscard]] bool operator==(const DesignGroup&) const noexcept = default;
};

// The groups from `firstField` to the end of the card. A keyword that the card does not allow, a keyword out of order,
// a value before any keyword and a value that is not of the kind of its group are errors; a keyword repeated right
// after its own group goes on with the group.
[[nodiscard]] std::vector<DesignGroup> readDesignGroups(FieldReader& reader, int firstField,
                                                        std::span<const GroupKeyword> keywords);

// Writes the groups the way readDesignGroups reads them.
void writeDesignGroups(FieldWriter& writer, int firstField, const std::vector<DesignGroup>& groups,
                       std::span<const GroupKeyword> keywords);

// The problem with the groups of a card, if there is one: a keyword the card does not allow, groups out of order or
// repeated apart, an ID that is not positive, an odd number of numbers in a group of components, a word that would not
// read back.
[[nodiscard]] std::optional<CardProblem> checkDesignGroups(const std::vector<DesignGroup>& groups,
                                                           std::span<const GroupKeyword> keywords);

// The group of a keyword, or null.
[[nodiscard]] const DesignGroup* findDesignGroup(const std::vector<DesignGroup>& groups,
                                                 std::string_view keyword) noexcept;
[[nodiscard]] DesignGroup* findDesignGroup(std::vector<DesignGroup>& groups, std::string_view keyword) noexcept;

// Puts `group` in the list at the place the order of the card gives its keyword: it replaces the group of the same
// keyword and is removed when it has no value. A keyword the card does not allow is left out (the check of the card
// reports it).
void setDesignGroup(std::vector<DesignGroup>& groups, std::span<const GroupKeyword> keywords, DesignGroup group);

// Visits every ID of a group that names another card (not the grid points of Components, which the card visits itself).
void visitGroupIds(ReferenceVisitor& visitor, DesignGroup& group, std::string_view fieldName, ReferenceTarget target);

} // namespace oor::model
