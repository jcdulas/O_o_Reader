#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The continuation lines of SEDRSP2 and SEDRSP3 are groups, as those of DRESP2 and DRESP3 are, with one difference:
// every item is led by the number of the part superelement that holds what it names (a design variable of part 3 is not
// one of part 5, each part having its own ID space). A group is a keyword in the first field of a line ("DESVAR",
// "DTABLE", "DRESP1", "DNODE", ...) and the items it introduces on that line and on the lines after it that have a
// blank keyword. Each card has its own array of SuperelementGroupKeyword, and this reads, writes, checks and visits the
// groups by it. It describes no card: what a keyword means stays in the card.

enum class SuperelementGroupContent : std::uint8_t
{
    Pairs,  // (SEID, ID) pairs, three to a line (DESVAR, DRESP1, DVPREL1, DVCREL1, DVMREL1, DVPREL2, DVCREL2, DVMREL2)
    Labels, // (SEID, label) pairs, three to a line (DTABLE)
    Nodes,  // (SEID, grid point, component) triples, two to a line (DNODE)
    Text    // a text of up to seven fields to a line, with blanks (USRDATA)
};

struct SuperelementGroupKeyword
{
    std::string_view keyword; // the neutral name of a card ("DesignVar"), or another word ("DesignNodes")
    SuperelementGroupContent content;
    ReferenceTarget target; // what the ID of a Pairs item or the grid point of a Nodes item names, in its superelement
    const char* valueName;  // the name of one value, for the messages and the diagnostics
};

// An item of a group: the superelement and what it names. Which members count depends on the content of the group: `id`
// for Pairs and Nodes (the grid point), `component` for Nodes (1, 2 or 3: a translation of the grid point), `label` for
// Labels.
struct SuperelementGroupItem
{
    base::EntityId superelementId = 0;
    base::EntityId id = 0;
    std::int32_t component = 0;
    std::string label;

    [[nodiscard]] bool operator==(const SuperelementGroupItem&) const noexcept = default;
};

struct SuperelementGroup
{
    std::string keyword;
    std::vector<SuperelementGroupItem> items;
    std::vector<std::string> lines; // Text groups: one entry for each line

    [[nodiscard]] bool operator==(const SuperelementGroup&) const noexcept = default;
};

// The groups from `firstField` to the end of the card. A keyword the card does not allow, an item before any keyword
// and a field left over after the items of a line are errors; a keyword repeated right after its own group goes on with
// it.
[[nodiscard]] std::vector<SuperelementGroup> readSuperelementGroups(FieldReader& reader, int firstField,
                                                                    std::span<const SuperelementGroupKeyword> keywords);

// Writes the groups the way readSuperelementGroups reads them: a group starts a line, and its items fill the lines.
void writeSuperelementGroups(FieldWriter& writer, int firstField, const std::vector<SuperelementGroup>& groups,
                             std::span<const SuperelementGroupKeyword> keywords);

// The problem with the groups of a card, if there is one: a keyword the card does not allow, a group with no item, a
// superelement that is not a number of the deck (0, the residual structure, is one), an ID or a grid point that is not
// positive, components that are not digits 1 to 6, a label or a text that would not read back.
[[nodiscard]] std::optional<CardProblem> checkSuperelementGroups(const std::vector<SuperelementGroup>& groups,
                                                                 std::span<const SuperelementGroupKeyword> keywords);

// Visits the superelement of every item and what it names in that superelement (a design variable, a response, a
// relation, a grid point). The labels of a DTABLE name nothing a card owns.
void visitSuperelementGroups(ReferenceVisitor& visitor, std::vector<SuperelementGroup>& groups,
                             std::span<const SuperelementGroupKeyword> keywords);

// The keyword entry of a group, or null.
[[nodiscard]] const SuperelementGroupKeyword* findSuperelementGroupKeyword(
    std::span<const SuperelementGroupKeyword> keywords, std::string_view keyword) noexcept;

} // namespace oor::model
