#pragma once

#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// The cards whose continuation lines start with a keyword and then have named fields in fixed places (BCTABLE, NLSTEP,
// NLADAPT, NLHARM...): one kind of value used inside one family. Each card has its own table of the places of its
// fields, written from its description in the format reference; nothing here knows a field.
namespace oor::model
{

// A named field of a keyword section: which section it is in, what it holds, in which line of the section (0 is the
// line of the keyword; the lines after it have no keyword) and in which field of that line (1 is the first field of the
// line).
struct ContactFieldPosition
{
    std::string_view keyword;
    ContactParameterRule rule;
    int line;
    int offset;
};

// A keyword section of a card as the card keeps it: the keyword and the named fields that it has, in the order they
// were given. A keyword with no field is a section too (NLSTEP ... ADAPT with nothing after it).
struct ContactNamedSection
{
    std::string keyword; // upper case
    std::vector<ContactParameter> params;

    [[nodiscard]] bool operator==(const ContactNamedSection&) const = default;
};

// Reads the named fields of `section` that the table puts there, into `params` (a blank field gives none). A field of
// the section that the table does not place, is not one of the `ownOffsets` (the fields of the section that the card
// reads itself, such as the ID after a SECNDRY) and is not blank, is an error. `owner` is the card type, whose
// keywords the messages show.
void readContactSectionFields(FieldReader& reader, const ContactSection& section,
                              std::span<const ContactFieldPosition> positions, std::initializer_list<int> ownOffsets,
                              std::vector<ContactParameter>& params, std::string_view owner);

// How many lines the fields of `keyword` in `params` take (at least `minimum`).
[[nodiscard]] int contactSectionLines(const std::vector<ContactParameter>& params, std::string_view keyword,
                                      std::span<const ContactFieldPosition> positions, int minimum);

// Writes the keyword (in field `keywordOffset` of the line that starts at `lineStart`) and the fields of `params` that
// the table puts in that section; returns the first field of the next free line.
int writeContactSectionFields(FieldWriter& writer, int lineStart, int keywordOffset, std::string_view keyword,
                              const std::vector<ContactParameter>& params,
                              std::span<const ContactFieldPosition> positions, int minimumLines = 1);

// The position of a neutral name in the table, or nullptr.
[[nodiscard]] const ContactFieldPosition* findContactFieldPosition(std::span<const ContactFieldPosition> positions,
                                                                   std::string_view name);

// The rules of a table of positions (for checkContactParameters), in the order of the table.
[[nodiscard]] std::vector<ContactParameterRule> contactFieldRules(std::span<const ContactFieldPosition> positions);

// The cards made only of a head line and keyword sections (NLSTEP, NLADAPT): the sections of the lines after the head,
// each with the named fields the table places in it. The head is read by the card. A line with no keyword before the
// first keyword is an error. `owner` is the card type.
[[nodiscard]] std::vector<ContactNamedSection> readContactNamedSections(FieldReader& reader,
                                                                        std::span<const std::string_view> keywords,
                                                                        std::span<const ContactFieldPosition> positions,
                                                                        std::string_view owner);

// Writes the sections one after the other from the second line of the card, each as writeContactSectionFields does.
void writeContactNamedSections(FieldWriter& writer, const std::vector<ContactNamedSection>& sections,
                               std::span<const ContactFieldPosition> positions);

// The first problem of the sections of a card: a keyword that is not in `keywords` or is given twice, a name that the
// table does not place in that section, a name given twice, a value of the wrong kind (`rules` is what the table gives
// contactFieldRules). The problem is on the field "sections"; its message shows the keywords of the card type `owner`.
[[nodiscard]] std::optional<CardProblem> checkContactNamedSections(const std::vector<ContactNamedSection>& sections,
                                                                   std::span<const std::string_view> keywords,
                                                                   std::span<const ContactFieldPosition> positions,
                                                                   std::span<const ContactParameterRule> rules,
                                                                   std::string_view owner);

} // namespace oor::model
