#include "oor/model/cards/nodes/moduleRigidBodyStar/ModuleRigidBodyStarTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 5;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

// True when the field holds a real (a number that is not an integer). Reports an error for text that is neither.
bool isReal(FieldReader& reader, int field)
{
    const auto value = reader.integerOrReal(field, "alphaCoefficient");
    return value && std::holds_alternative<double>(*value);
}

} // namespace

std::optional<ModuleRigidBodyStarCard> ModuleRigidBodyStarFormat::read(FieldReader& reader)
{
    ModuleRigidBodyStarCard card;
    card.id = reader.requiredInteger(1, "id");
    card.independentModuleId = reader.integer(2, "independentModuleId", 0);
    card.independentNode = reader.requiredInteger(3, "independentNode");
    card.components = base::sortedDigits(reader.requiredInteger(4, "dependentComponents"));

    // After the last pair there may be ALPHA, or ALPHA and TREF: the reals at the end of the card, the same rule as the
    // RBE2. A blank before a final real is a blank ALPHA.
    int lastPairField = reader.lastUsedField();
    if (lastPairField >= FIRST_PAIR_FIELD && isReal(reader, lastPairField))
    {
        const int before = lastPairField - 1;
        if (before >= FIRST_PAIR_FIELD && (reader.isBlank(before) || isReal(reader, before)))
        {
            card.alphaCoefficient = reader.real(before, "alphaCoefficient", 0.0);
            card.referenceTemperature = reader.real(lastPairField, "referenceTemperature", 0.0);
            lastPairField -= 2;
        }
        else
        {
            card.alphaCoefficient = reader.real(lastPairField, "alphaCoefficient", 0.0);
            lastPairField -= 1;
        }
    }

    // The pairs of a module and a grid point sit in fixed places, one after the other from field 5; a pair with both
    // fields blank is padding.
    for (int field = FIRST_PAIR_FIELD; field <= lastPairField && !reader.failed(); field += 2)
    {
        if (field == lastPairField && !reader.isBlank(field))
        {
            reader.error("card.missing-field",
                         "a module of an " + std::string(reader.cardName()) + " needs the grid point that follows it",
                         "dependentNode");
            break;
        }
        if (const std::optional<ModuleId> pair =
                readModuleId(reader, field, field + 1, "dependentModule", "dependentNode"))
            card.dependents.push_back(*pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRigidBodyStarFormat::check(const ModuleRigidBodyStarCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkModuleNumber(card.independentModuleId, "independentModuleId"))
        return problem;
    if (const auto problem = checkRequiredId(card.independentNode, "independentNode"))
        return problem;
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                           "dependentComponents"};
    if (card.dependents.empty())
        return CardProblem{"card.missing-field", "the card needs at least one dependent grid point", "dependentNode"};
    for (const ModuleId& pair : card.dependents)
    {
        if (const auto problem = checkModuleId(pair, "dependentModule", "dependentNode"))
            return problem;
    }
    if (const auto problem = checkReal(card.alphaCoefficient, "alphaCoefficient", RealRule::Finite))
        return problem;
    return checkReal(card.referenceTemperature, "referenceTemperature", RealRule::Finite);
}

void ModuleRigidBodyStarFormat::write(const ModuleRigidBodyStarCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.independentModuleId, 0);
    writer.integer(3, card.independentNode);
    writer.integer(4, card.components);
    int field = FIRST_PAIR_FIELD;
    for (const ModuleId& pair : card.dependents)
    {
        writeModuleId(writer, field, field + 1, pair, false);
        field += 2;
    }
    // ALPHA and TREF in the next two fields. When only TREF is not zero, ALPHA is written as 0.0 so that the reals at
    // the end of the card read back as the same two values.
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0)
    {
        writer.real(field++, card.alphaCoefficient);
        writer.realUnlessDefault(field, card.referenceTemperature, 0.0);
    }
}

std::uint32_t ModuleRigidBodyStarTable::append(const ModuleRigidBodyStarCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRigidBodyStarTable::store(std::uint32_t row, const ModuleRigidBodyStarCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRigidBodyStarTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRigidBodyStarTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRigidBodyStarTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRigidBodyStarCard& card = m_cards.edit(row);
    // Each grid point is looked up in its module (ModuleConnectionSupport.hpp).
    visitIdInModule(visitor, "independentNode", ReferenceTarget::Node, card.independentModuleId, card.independentNode);
    for (ModuleId& pair : card.dependents)
        visitModuleId(visitor, "dependentNode", ReferenceTarget::Node, pair);
}

} // namespace oor::model
