#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// What the contact and nonlinear cards have in common in the way they are laid out: lines of eight fields that start
// with a keyword (SECNDRY, PRIMARY, GRID, KNOT, ...), the lists that follow the keyword, and the lines that have no
// keyword and are told by their place. Each card still reads and writes its own fields by hand: these functions only
// find where the lines of a card are and move lists of one kind of value.
namespace oor::model
{

inline constexpr int CONTACT_LINE_FIELDS = 8;

// The first field of the line `line` of a card (the first line is line 0): fields 1, 9, 17, ...
[[nodiscard]] constexpr int firstFieldOfLine(int line) noexcept
{
    return 1 + CONTACT_LINE_FIELDS * line;
}

// The first field of the line after the one that holds `field`.
[[nodiscard]] constexpr int firstFieldOfNextLine(int field) noexcept
{
    return 1 + CONTACT_LINE_FIELDS * ((field - 1) / CONTACT_LINE_FIELDS + 1);
}

// The field of the NAME of the pair number `index` in a card of (name, value) pairs whose first pair is at field
// `firstPairField`: the pairs that fit on the first line (up to field 8), then the continuation lines, whose pairs
// start at their first field (9, 11, 13, 15, 17, ...).
[[nodiscard]] int contactPairField(int firstPairField, std::size_t index) noexcept;

// A piece of a card: the lines from one keyword to the next. The lines before the first keyword (and the first line
// itself) are the section whose keyword is empty. A line that starts with no keyword goes on with the section above it.
struct ContactSection
{
    std::string keyword; // upper case; empty for the head of the card
    int first = 1;       // the field of the keyword (the first or the second field of the section's first line)
    int last = 8;        // the last field of the section's last line
};

// Cuts the fields of a card into sections at the lines that start with one of `keywords` (any case): the keyword is in
// the first field of the line, or in the second when the first is blank (the format reference draws the options of a BCTABLE that
// way). Only the continuation lines are looked at: the first line is the head.
[[nodiscard]] std::vector<ContactSection> splitContactSections(const FieldReader& reader,
                                                               std::initializer_list<std::string_view> keywords);
[[nodiscard]] std::vector<ContactSection> splitContactSections(const FieldReader& reader,
                                                               std::span<const std::string_view> keywords);

// The first section of that keyword, or nullptr.
[[nodiscard]] const ContactSection* findContactSection(const std::vector<ContactSection>& sections,
                                                       std::string_view keyword);

// The non-blank fields from `first` to `last`, read one kind of value each (the blank ones are the padding of a line).
[[nodiscard]] std::vector<base::EntityId> readContactIds(FieldReader& reader, int first, int last, const char* name);
[[nodiscard]] std::vector<double> readContactReals(FieldReader& reader, int first, int last, const char* name);
[[nodiscard]] std::vector<std::string> readContactWords(FieldReader& reader, int first, int last);

// Writes a keyword at the start of the line that begins at field `lineStart` and the values after it, seven to a line
// (a line after the first begins with a blank field, as the format reference draws such lists). With an empty keyword the first
// field of the line stays blank. Returns the first field of the next line.
int writeContactIdList(FieldWriter& writer, int lineStart, std::string_view keyword,
                       std::span<const base::EntityId> values);
int writeContactRealList(FieldWriter& writer, int lineStart, std::string_view keyword, std::span<const double> values);
int writeContactWordList(FieldWriter& writer, int lineStart, std::string_view keyword,
                         std::span<const std::string> values);

// Visits the IDs of a list (a blank one, 0, is skipped) and keeps what the visitor returns.
void visitContactIds(std::vector<base::EntityId>& ids, ReferenceVisitor& visitor, const char* name,
                     ReferenceTarget target);
void visitContactIdColumn(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                          const char* name, ReferenceTarget target);

// The ID in `id` visited as a reference (a blank one, 0, is skipped); keeps what the visitor returns.
void visitContactId(base::EntityId& id, ReferenceVisitor& visitor, const char* name, ReferenceTarget target);
// The same for the ID of a row of a column.
void visitContactIdAt(Column<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor, const char* name,
                      ReferenceTarget target);

} // namespace oor::model
