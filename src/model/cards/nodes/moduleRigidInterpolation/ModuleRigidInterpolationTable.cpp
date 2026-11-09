#include "oor/model/cards/nodes/moduleRigidInterpolation/ModuleRigidInterpolationTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

#include <algorithm>
#include <string>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 5;
constexpr int FIELDS_PER_LINE = 8;
constexpr int FIELDS_PER_TRIPLE = 3;
constexpr int TRIPLES_PER_LINE = 2;
constexpr int LAST_FIELD_OF_LINE = FIELDS_PER_LINE - 1; // the offset of the blank cell that ends a line of the UM part
constexpr base::EntityId ID_LIMIT = 100000000;          // an element ID is below it (format reference, EID)

bool isNonEmptyComponentSet(std::int32_t components)
{
    return components > 0 && base::isComponentSet(components);
}

// The first field of the line a part that must begin a line (UM, ALPHA) goes to, when `next` is the next free field.
int lineStartAtOrAfter(int next)
{
    const int used = next - 1;
    return used % FIELDS_PER_LINE == 0 ? next : (used / FIELDS_PER_LINE + 1) * FIELDS_PER_LINE + 1;
}

// The first field in [from, to] that holds the keyword, or 0.
int findKeyword(const FieldReader& reader, int from, int to, std::string_view keyword)
{
    for (int field = from; field <= to; ++field)
    {
        if ((reader.text(field) == keyword))
            return field;
    }
    return 0;
}

// True when the field holds an integer (a real is the weight of the next group). Reports an error for text that is
// neither.
bool holdsInteger(FieldReader& reader, int field)
{
    const auto value = reader.integerOrReal(field, "node");
    return value && std::holds_alternative<std::int32_t>(*value);
}

// The first field of the dependent point number `index` of the UM part, when "UM" is in the field `umField`: the
// keyword (or a blank, on the lines that go on) takes the first field of a line, two triples of a module, a grid point
// and components follow it and the last field of the line is blank.
int tripleField(int umField, std::size_t index)
{
    const int line = static_cast<int>(index) / TRIPLES_PER_LINE;
    const int place = static_cast<int>(index) % TRIPLES_PER_LINE;
    return umField + FIELDS_PER_LINE * line + 1 + FIELDS_PER_TRIPLE * place;
}

// Reads the weighted groups in fields [from, end): WT, C, then the pairs of a module and a grid point up to the next
// real. Blank fields are skipped (the padding before a keyword, the end of a line); a blank module before a grid point
// is the main Bulk Data.
void readGroups(FieldReader& reader, int from, int end, ModuleRigidInterpolationCard& card)
{
    int field = from;
    while (field < end && !reader.failed())
    {
        if (reader.isBlank(field))
        {
            ++field;
            continue;
        }
        ModuleRigidInterpolationGroup group;
        group.weight = reader.requiredReal(field, "weight");
        group.components = base::sortedDigits(reader.requiredInteger(field + 1, "components"));
        field += 2;
        while (field < end && !reader.failed())
        {
            if (reader.isBlank(field))
            {
                if (field + 1 < end && !reader.isBlank(field + 1) && holdsInteger(reader, field + 1))
                {
                    group.points.push_back(ModuleId{0, reader.requiredInteger(field + 1, "node")});
                    field += 2;
                }
                else
                {
                    ++field;
                }
                continue;
            }
            const auto module = reader.integerOrReal(field, "moduleId");
            if (!module || std::holds_alternative<double>(*module))
                break; // a real is the weight of the next group
            if (field + 1 >= end)
            {
                reader.error(
                    "card.missing-field",
                    "a module of an " + std::string(reader.cardName()) + " group needs the grid point that follows it",
                    "node");
                break;
            }
            group.points.push_back(
                ModuleId{std::get<std::int32_t>(*module), reader.requiredInteger(field + 1, "node")});
            field += 2;
        }
        card.groups.push_back(std::move(group));
    }
}

// Reads the UM part, which starts with the keyword in the first field of a line and ends at `end` (the field of ALPHA,
// or the field after the last one of the card).
void readDependents(FieldReader& reader, int umField, int end, ModuleRigidInterpolationCard& card)
{
    // The cells the format leaves blank: the last field of every line, and the first field of the lines after the
    // keyword's.
    for (int line = umField; line < end; line += FIELDS_PER_LINE)
    {
        const bool firstUsed = line == umField;
        const int lastField = line + LAST_FIELD_OF_LINE;
        if ((!firstUsed && !reader.isBlank(line)) || (lastField < end && !reader.isBlank(lastField)))
        {
            reader.error("card.bad-value",
                         "the first field of a line that goes on after " + std::string("dependentList") +
                             " and the last field of every line of the " + std::string("dependentList") +
                             " part of an " + std::string(reader.cardName()) + " are not used and must be blank",
                         "");
            return;
        }
    }
    for (std::size_t index = 0; !reader.failed(); ++index)
    {
        const int field = tripleField(umField, index);
        if (field >= end)
            break;
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        if (field + 2 >= end)
        {
            reader.error(
                "card.missing-field",
                "a dependent point of an " + std::string(reader.cardName()) + " needs its module, point and components",
                "dependentComponents");
            break;
        }
        ModuleRigidInterpolationDependent dependent;
        dependent.point.module = reader.integer(field, "dependentModuleId", 0);
        dependent.point.id = reader.requiredInteger(field + 1, "dependentNode");
        dependent.components = base::sortedDigits(reader.requiredInteger(field + 2, "dependentComponents"));
        card.dependents.push_back(dependent);
    }
}

} // namespace

std::optional<ModuleRigidInterpolationCard> ModuleRigidInterpolationFormat::read(FieldReader& reader)
{
    ModuleRigidInterpolationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceModuleId = reader.integer(2, "referenceModuleId", 0);
    card.referenceNode = reader.requiredInteger(3, "referenceNode");
    card.referenceComponents = base::sortedDigits(reader.requiredInteger(4, "referenceComponents"));

    const int last = reader.lastUsedField();
    const int umField = findKeyword(reader, FIRST_GROUP_FIELD, last, "DependentDofs");
    const int alphaField = findKeyword(reader, FIRST_GROUP_FIELD, last, "ThermalExpansion");
    if (umField != 0 && alphaField != 0 && alphaField < umField)
        reader.error("card.bad-value",
                     std::string("dependentList") + " must come before " + std::string("alphaCoefficient"),
                     "dependentList");

    int groupsEnd = last + 1;
    for (const int keywordField : {umField, alphaField})
    {
        if (keywordField != 0)
            groupsEnd = std::min(groupsEnd, keywordField);
    }
    readGroups(reader, FIRST_GROUP_FIELD, groupsEnd, card);

    if (umField != 0 && !reader.failed())
    {
        if ((umField - 1) % FIELDS_PER_LINE != 0)
            reader.error("card.bad-value", std::string("dependentList") + " must be the first field of a line",
                         "dependentList");
        else
            readDependents(reader, umField, alphaField > umField ? alphaField : last + 1, card);
    }
    if (alphaField != 0 && !reader.failed())
    {
        card.alphaCoefficient = reader.real(alphaField + 1, "alphaCoefficient", 0.0);
        card.referenceTemperature = reader.real(alphaField + 2, "referenceTemperature", 0.0);
        if (last > alphaField + 2)
            reader.error("card.bad-value",
                         "an " + std::string(reader.cardName()) + " ends with " + std::string("referenceTemperature") +
                             ": nothing follows it",
                         "referenceTemperature");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRigidInterpolationFormat::check(const ModuleRigidInterpolationCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (const auto problem =
            checkModuleId(ModuleId{card.referenceModuleId, card.referenceNode}, "referenceModuleId", "referenceNode"))
        return problem;
    if (!isNonEmptyComponentSet(card.referenceComponents))
        return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                           "referenceComponents"};
    if (card.groups.empty())
        return CardProblem{"card.missing-field", "the card needs at least one weighted group", "weight1"};
    for (const ModuleRigidInterpolationGroup& group : card.groups)
    {
        if (const auto problem = checkReal(group.weight, "weight", RealRule::Finite))
            return problem;
        if (!isNonEmptyComponentSet(group.components))
            return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                               "components"};
        if (group.points.empty())
            return CardProblem{"card.missing-field", "a weighted group needs at least one grid point", "node"};
        for (const ModuleId& pair : group.points)
        {
            if (const auto problem = checkModuleId(pair, "moduleId", "node"))
                return problem;
        }
    }
    for (const ModuleRigidInterpolationDependent& dependent : card.dependents)
    {
        if (const auto problem = checkModuleId(dependent.point, "dependentModuleId", "dependentNode"))
            return problem;
        if (!isNonEmptyComponentSet(dependent.components))
            return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                               "dependentComponents"};
    }
    if (const auto problem = checkReal(card.alphaCoefficient, "alphaCoefficient", RealRule::Finite))
        return problem;
    return checkReal(card.referenceTemperature, "referenceTemperature", RealRule::Finite);
}

void ModuleRigidInterpolationFormat::write(const ModuleRigidInterpolationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.referenceModuleId, 0);
    writer.integer(3, card.referenceNode);
    writer.integer(4, card.referenceComponents);
    int field = FIRST_GROUP_FIELD;
    for (const ModuleRigidInterpolationGroup& group : card.groups)
    {
        writer.real(field++, group.weight);
        writer.integer(field++, group.components);
        for (const ModuleId& pair : group.points)
        {
            // The module is written even when it is 0: the pairs follow each other with no fixed place.
            writer.integer(field++, pair.module);
            writer.integer(field++, pair.id);
        }
    }
    if (!card.dependents.empty())
    {
        const int umField = lineStartAtOrAfter(field);
        writer.text(umField, "DependentDofs");
        for (std::size_t index = 0; index < card.dependents.size(); ++index)
        {
            const ModuleRigidInterpolationDependent& dependent = card.dependents[index];
            const int first = tripleField(umField, index);
            writeModuleId(writer, first, first + 1, dependent.point, false);
            writer.integer(first + 2, dependent.components);
        }
        // The next part begins on the line after the last triple.
        const int triples = static_cast<int>(card.dependents.size());
        field = umField + FIELDS_PER_LINE * ((triples - 1) / TRIPLES_PER_LINE + 1);
    }
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0)
    {
        field = lineStartAtOrAfter(field);
        writer.text(field++, "ThermalExpansion");
        writer.real(field++, card.alphaCoefficient);
        writer.realUnlessDefault(field, card.referenceTemperature, 0.0);
    }
}

std::uint32_t ModuleRigidInterpolationTable::append(const ModuleRigidInterpolationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRigidInterpolationTable::store(std::uint32_t row, const ModuleRigidInterpolationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRigidInterpolationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRigidInterpolationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRigidInterpolationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRigidInterpolationCard& card = m_cards.edit(row);
    // Each grid point is looked up in its module (ModuleConnectionSupport.hpp).
    visitIdInModule(visitor, "referenceNode", ReferenceTarget::Node, card.referenceModuleId, card.referenceNode);
    for (ModuleRigidInterpolationGroup& group : card.groups)
    {
        for (ModuleId& pair : group.points)
            visitModuleId(visitor, "node", ReferenceTarget::Node, pair);
    }
    for (ModuleRigidInterpolationDependent& dependent : card.dependents)
        visitModuleId(visitor, "dependentNode", ReferenceTarget::Node, dependent.point);
}

} // namespace oor::model
