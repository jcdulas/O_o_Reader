#include "oor/model/cards/nodes/moduleContactPair/ModuleContactPairTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

// A line of a list has the keyword (or a blank, on the lines that go on) in its first field, three pairs of module and
// ID after it, and a blank field.
constexpr int LINE_FIELDS = 8;
constexpr int PAIRS_PER_LINE = 3;
constexpr const char* SECONDARY_WORD = "Secondary";
constexpr const char* PRIMARY_WORD = "Primary";

enum class ListKind : std::uint8_t
{
    None,
    Secondary,
    Primary
};

// The field of a line: `start` is its first field.
constexpr int firstFieldOfListLine(int line) noexcept
{
    return 1 + LINE_FIELDS * line;
}

// Reads the pairs of the line that starts at field `start` and adds them to `list`; the last field of the line is
// blank.
void readListLine(FieldReader& reader, int start, std::vector<ModuleId>& list, const char* moduleName,
                  const char* idName)
{
    for (int place = 0; place < PAIRS_PER_LINE; ++place)
    {
        const int field = start + 1 + 2 * place;
        if (const std::optional<ModuleId> pair = readModuleId(reader, field, field + 1, moduleName, idName))
            list.push_back(*pair);
    }
    if (!reader.isBlank(start + LINE_FIELDS - 1))
        reader.error("card.bad-value",
                     "field " + std::to_string(start + LINE_FIELDS - 1) + " of an " + std::string(reader.cardName()) +
                         " is not used and must be blank",
                     "");
}

// Writes a list with its keyword from the line at `start`; returns the first field of the line after the list.
int writeList(FieldWriter& writer, int start, const char* keyword, const std::vector<ModuleId>& list)
{
    writer.text(start, keyword);
    for (std::size_t index = 0; index < list.size(); ++index)
    {
        const int line = static_cast<int>(index) / PAIRS_PER_LINE;
        const int place = static_cast<int>(index) % PAIRS_PER_LINE;
        const int field = start + LINE_FIELDS * line + 1 + 2 * place;
        writeModuleId(writer, field, field + 1, list[index], false);
    }
    const int lines = (static_cast<int>(list.size()) + PAIRS_PER_LINE - 1) / PAIRS_PER_LINE;
    return start + LINE_FIELDS * (lines > 0 ? lines : 1);
}

// The problem of a body given as a short form: a module with no ID, or a module and ID that are not valid.
std::optional<CardProblem> checkShortBody(base::EntityId module, base::EntityId id, const char* moduleName,
                                          const char* idName)
{
    if (id == 0)
    {
        if (module != 0)
            return CardProblem{"card.missing-field",
                               CardProblem::fieldNamed(moduleName) + " needs the ID that follows it", idName};
        return std::nullopt;
    }
    return checkModuleId(ModuleId{module, id}, moduleName, idName);
}

} // namespace

std::optional<ModuleContactPairCard> ModuleContactPairFormat::read(FieldReader& reader)
{
    ModuleContactPairCard card;
    card.id = reader.requiredInteger(1, "id");
    card.geometricParamsId = reader.integer(2, "geometricParamsId", 0);
    card.physicalParamsId = reader.integer(3, "physicalParamsId", 0);
    if (const std::optional<ModuleId> touching = readModuleId(reader, 4, 5, "secondaryModule", "secondaryBodyId"))
    {
        card.secondaryModule = touching->module;
        card.secondaryBodyId = touching->id;
    }
    if (const std::optional<ModuleId> touched = readModuleId(reader, 6, 7, "primaryModule", "primaryBodyId"))
    {
        card.primaryModule = touched->module;
        card.primaryBodyId = touched->id;
    }
    if (!reader.isBlank(LINE_FIELDS))
        reader.error("card.bad-value",
                     "field 8 of an " + std::string(reader.cardName()) + " is not used and must be blank", "");
    // The lines after the first: a keyword starts a list, a blank first field goes on with the list above.
    ListKind current = ListKind::None;
    const int lines = (reader.lastUsedField() + LINE_FIELDS - 1) / LINE_FIELDS;
    for (int line = 1; line < lines && !reader.failed(); ++line)
    {
        const int start = firstFieldOfListLine(line);
        const std::string_view word = reader.text(start);
        if (word.empty())
        {
            bool used = false;
            for (int field = start + 1; field < start + LINE_FIELDS; ++field)
                used = used || !reader.isBlank(field);
            if (!used)
                continue; // a line with nothing on it
        }
        else if ((word == SECONDARY_WORD))
        {
            current = ListKind::Secondary;
        }
        else if ((word == PRIMARY_WORD))
        {
            current = ListKind::Primary;
        }
        else
        {
            reader.error("card.bad-value",
                         "a continuation line of an " + std::string(reader.cardName()) + " starts with " +
                             std::string("secondaryList") + " or " + std::string("primaryList"),
                         "secondaryList");
            break;
        }
        if (current == ListKind::None)
        {
            reader.error("card.bad-value",
                         "a continuation line of an " + std::string(reader.cardName()) + " starts with " +
                             std::string("secondaryList") + " or " + std::string("primaryList"),
                         "secondaryList");
            break;
        }
        if (current == ListKind::Secondary)
            readListLine(reader, start, card.secondary, "secondaryModule", "secondaryBodyIds");
        else
            readListLine(reader, start, card.primary, "primaryModule", "primaryBodyId");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleContactPairFormat::check(const ModuleContactPairCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", "the {field} must not be negative", "id"};
    if (const auto problem = checkOptionalId(card.geometricParamsId, "geometricParamsId"))
        return problem;
    if (const auto problem = checkOptionalId(card.physicalParamsId, "physicalParamsId"))
        return problem;
    if (const auto problem =
            checkShortBody(card.secondaryModule, card.secondaryBodyId, "secondaryModule", "secondaryBodyId"))
        return problem;
    if (const auto problem = checkShortBody(card.primaryModule, card.primaryBodyId, "primaryModule", "primaryBodyId"))
        return problem;
    for (const ModuleId& pair : card.secondary)
    {
        if (const auto problem = checkModuleId(pair, "secondaryModule", "secondaryBodyIds"))
            return problem;
    }
    for (const ModuleId& pair : card.primary)
    {
        if (const auto problem = checkModuleId(pair, "primaryModule", "primaryBodyId"))
            return problem;
    }
    // A body is the short field or the first of its list, and the list is required without the field.
    if (card.secondaryBodyId == 0 && card.secondary.empty())
        return CardProblem{"card.missing-field",
                           "the card needs {field} or a " + CardProblem::fieldNamed("secondaryList") + " list",
                           "secondaryBodyId"};
    if (card.primaryBodyId == 0 && card.primary.empty())
        return CardProblem{"card.missing-field",
                           "the card needs {field} or a " + CardProblem::fieldNamed("primaryList") + " list",
                           "primaryBodyId"};
    return std::nullopt;
}

void ModuleContactPairFormat::write(const ModuleContactPairCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.geometricParamsId, 0);
    writer.integerUnlessDefault(3, card.physicalParamsId, 0);
    if (card.secondaryBodyId != 0)
        writeModuleId(writer, 4, 5, ModuleId{card.secondaryModule, card.secondaryBodyId}, false);
    if (card.primaryBodyId != 0)
        writeModuleId(writer, 6, 7, ModuleId{card.primaryModule, card.primaryBodyId}, false);
    int line = firstFieldOfListLine(1);
    if (!card.secondary.empty())
        line = writeList(writer, line, SECONDARY_WORD, card.secondary);
    if (!card.primary.empty())
        writeList(writer, line, PRIMARY_WORD, card.primary);
}

std::uint32_t ModuleContactPairTable::append(const ModuleContactPairCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleContactPairTable::store(std::uint32_t row, const ModuleContactPairCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleContactPairTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleContactPairTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleContactPairTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleContactPairCard& card = m_cards.edit(row);
    visitId(visitor, "geometricParamsId", ReferenceTarget::ContactGeometricParameters, card.geometricParamsId);
    visitId(visitor, "physicalParamsId", ReferenceTarget::ContactPhysicalParameters, card.physicalParamsId);
    // A body is looked up only when its module is the main Bulk Data (ModuleIds.hpp).
    ModuleId touching{card.secondaryModule, card.secondaryBodyId};
    visitModuleId(visitor, "secondaryBodyId", ReferenceTarget::ContactBody, touching);
    card.secondaryBodyId = touching.id;
    ModuleId touched{card.primaryModule, card.primaryBodyId};
    visitModuleId(visitor, "primaryBodyId", ReferenceTarget::ContactBody, touched);
    card.primaryBodyId = touched.id;
    for (ModuleId& pair : card.secondary)
        visitModuleId(visitor, "secondaryBodyIds", ReferenceTarget::ContactBody, pair);
    for (ModuleId& pair : card.primary)
        visitModuleId(visitor, "primaryBodyId", ReferenceTarget::ContactBody, pair);
}

} // namespace oor::model
