#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// The room for values on a keyword line: the fields after the keyword.
constexpr int VALUES_PER_LINE = CONTACT_LINE_FIELDS - 1;

// The field where the value number `index` of a list goes, for a list whose keyword is at `lineStart`.
int fieldOfListValue(int lineStart, std::size_t index)
{
    const int line = static_cast<int>(index) / VALUES_PER_LINE;
    const int place = static_cast<int>(index) % VALUES_PER_LINE;
    return lineStart + line * CONTACT_LINE_FIELDS + 1 + place;
}

int endOfList(int lineStart, std::size_t count)
{
    if (count == 0)
        return lineStart + CONTACT_LINE_FIELDS;
    const int lastField = fieldOfListValue(lineStart, count - 1);
    return firstFieldOfNextLine(lastField);
}

} // namespace

int contactPairField(int firstPairField, std::size_t index) noexcept
{
    const int pairsOnFirstLine = (CONTACT_LINE_FIELDS - firstPairField + 1) / 2;
    const int position = static_cast<int>(index);
    if (position < pairsOnFirstLine)
        return firstPairField + 2 * position;
    return firstFieldOfLine(1) + 2 * (position - pairsOnFirstLine);
}

std::vector<ContactSection> splitContactSections(const FieldReader& reader,
                                                 std::initializer_list<std::string_view> keywords)
{
    return splitContactSections(reader, std::span<const std::string_view>(keywords.begin(), keywords.size()));
}

std::vector<ContactSection> splitContactSections(const FieldReader& reader, std::span<const std::string_view> keywords)
{
    std::vector<ContactSection> sections;
    sections.push_back(ContactSection{"", 1, CONTACT_LINE_FIELDS});
    const int lines = (reader.lastUsedField() + CONTACT_LINE_FIELDS - 1) / CONTACT_LINE_FIELDS;
    for (int line = 1; line < lines; ++line)
    {
        const int start = firstFieldOfLine(line);
        const auto keywordAt = [&](int field) -> const std::string_view*
        {
            const std::string_view word = reader.text(field);
            if (word.empty())
                return nullptr;
            const auto found = std::find_if(keywords.begin(), keywords.end(),
                                            [&](std::string_view candidate) { return (word == candidate); });
            return found != keywords.end() ? &*found : nullptr;
        };
        int keywordField = start;
        const std::string_view* keyword = keywordAt(start);
        if (keyword == nullptr && reader.isBlank(start))
        {
            keywordField = start + 1;
            keyword = keywordAt(keywordField);
        }
        if (keyword != nullptr)
            sections.push_back(ContactSection{std::string(*keyword), keywordField, start + CONTACT_LINE_FIELDS - 1});
        else
            sections.back().last = start + CONTACT_LINE_FIELDS - 1;
    }
    return sections;
}

const ContactSection* findContactSection(const std::vector<ContactSection>& sections, std::string_view keyword)
{
    for (const ContactSection& section : sections)
    {
        if (section.keyword == keyword)
            return &section;
    }
    return nullptr;
}

std::vector<base::EntityId> readContactIds(FieldReader& reader, int first, int last, const char* name)
{
    std::vector<base::EntityId> ids;
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            ids.push_back(reader.requiredInteger(field, name));
    }
    return ids;
}

std::vector<double> readContactReals(FieldReader& reader, int first, int last, const char* name)
{
    std::vector<double> values;
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            values.push_back(reader.requiredReal(field, name));
    }
    return values;
}

std::vector<std::string> readContactWords(FieldReader& reader, int first, int last)
{
    std::vector<std::string> words;
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            words.emplace_back(reader.text(field));
    }
    return words;
}

int writeContactIdList(FieldWriter& writer, int lineStart, std::string_view keyword,
                       std::span<const base::EntityId> values)
{
    if (!keyword.empty())
        writer.text(lineStart, keyword);
    for (std::size_t index = 0; index < values.size(); ++index)
        writer.integer(fieldOfListValue(lineStart, index), values[index]);
    return endOfList(lineStart, values.size());
}

int writeContactRealList(FieldWriter& writer, int lineStart, std::string_view keyword, std::span<const double> values)
{
    if (!keyword.empty())
        writer.text(lineStart, keyword);
    for (std::size_t index = 0; index < values.size(); ++index)
        writer.real(fieldOfListValue(lineStart, index), values[index]);
    return endOfList(lineStart, values.size());
}

int writeContactWordList(FieldWriter& writer, int lineStart, std::string_view keyword,
                         std::span<const std::string> values)
{
    if (!keyword.empty())
        writer.text(lineStart, keyword);
    for (std::size_t index = 0; index < values.size(); ++index)
        writer.text(fieldOfListValue(lineStart, index), values[index]);
    return endOfList(lineStart, values.size());
}

void visitContactId(base::EntityId& id, ReferenceVisitor& visitor, const char* name, ReferenceTarget target)
{
    if (id == 0)
        return;
    const base::EntityId updated = visitor.visit(name, target, id);
    if (updated != id)
        id = updated;
}

void visitContactIdAt(Column<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor, const char* name,
                      ReferenceTarget target)
{
    base::EntityId id = column.get(row);
    const base::EntityId before = id;
    visitContactId(id, visitor, name, target);
    if (id != before)
        column.set(row, id);
}

void visitContactIds(std::vector<base::EntityId>& ids, ReferenceVisitor& visitor, const char* name,
                     ReferenceTarget target)
{
    for (base::EntityId& id : ids)
        visitContactId(id, visitor, name, target);
}

void visitContactIdColumn(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                          const char* name, ReferenceTarget target)
{
    const auto current = column.get(row);
    std::vector<base::EntityId> ids(current.begin(), current.end());
    const std::vector<base::EntityId> before = ids;
    visitContactIds(ids, visitor, name, target);
    if (ids != before)
        column.set(row, ids);
}

} // namespace oor::model
