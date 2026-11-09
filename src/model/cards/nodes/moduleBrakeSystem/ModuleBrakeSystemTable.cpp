#include "oor/model/cards/nodes/moduleBrakeSystem/ModuleBrakeSystemTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// A disk is a line of its own after the first: DISK in the first field of the line and seven numbers after it.
constexpr int FIELDS_PER_LINE = 8;

// The binary flags of the first line (IVEC, ISLIDEBS): 0 or 1.
std::optional<CardProblem> checkFlag(std::int32_t value, const char* field)
{
    if (value == 0 || value == 1)
        return std::nullopt;
    return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", field};
}

// The problem with the values of one disk line, if there is one.
std::optional<CardProblem> checkDisk(const ModuleBrakeSystemDisk& disk)
{
    if (const auto problem = checkNameWord(disk.name, "diskName"))
        return problem;
    if (const auto problem = checkBoundaryModule(disk.moduleId1, "moduleId1"))
        return problem;
    if (const auto problem = checkRequiredId(disk.body1Id, "body1Id"))
        return problem;
    if (const auto problem = checkBoundaryModule(disk.moduleId2, "moduleId2"))
        return problem;
    if (const auto problem = checkRequiredId(disk.body2Id, "body2Id"))
        return problem;
    if (const auto problem = checkOptionalId(disk.motion1Id, "motion1Id"))
        return problem;
    if (const auto problem = checkOptionalId(disk.motion2Id, "motion2Id"))
        return problem;
    return checkOptionalId(disk.brakeSystemId, "brakeSystemId");
}

} // namespace

std::optional<ModuleBrakeSystemCard> ModuleBrakeSystemFormat::read(FieldReader& reader)
{
    ModuleBrakeSystemCard card;
    card.id = reader.requiredInteger(1, "id");
    card.squealLoadFactor = reader.real(2, "squealLoadFactor", 0.0);
    card.frictionVectorMethod = reader.integer(3, "frictionVectorMethod", 0);
    card.squealOnly =
        readKeyword(reader, 4, "squealOnly", MODULE_BRAKE_SQUEAL_ONLY_WORDS, ModuleBrakeSystemSquealOnly::Yes);
    card.contactMethod = reader.integer(5, "contactMethod", 0);
    for (int field = 6; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the first line of an " +
                             std::string(reader.cardName()) +
                             " is not used and must be "
                             "blank",
                         "");
    }
    const int last = reader.lastUsedField();
    for (int start = FIELDS_PER_LINE + 1; start <= last && !reader.failed(); start += FIELDS_PER_LINE)
    {
        bool used = false;
        for (int field = start; field < start + FIELDS_PER_LINE; ++field)
            used = used || !reader.isBlank(field);
        if (!used)
            continue; // a line with nothing on it
        ModuleBrakeSystemDisk disk;
        disk.name = std::string(reader.text(start));
        disk.moduleId1 = reader.integer(start + 1, "moduleId1", 0);
        disk.body1Id = reader.requiredInteger(start + 2, "body1Id");
        disk.moduleId2 = reader.integer(start + 3, "moduleId2", 0);
        disk.body2Id = reader.requiredInteger(start + 4, "body2Id");
        disk.motion1Id = reader.integer(start + 5, "motion1Id", 0);
        disk.motion2Id = reader.integer(start + 6, "motion2Id", 0);
        disk.brakeSystemId = reader.integer(start + 7, "brakeSystemId", 0);
        card.disks.push_back(std::move(disk));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleBrakeSystemFormat::check(const ModuleBrakeSystemCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.squealLoadFactor, "squealLoadFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkFlag(card.frictionVectorMethod, "frictionVectorMethod"))
        return problem;
    if (const auto problem = checkFlag(card.contactMethod, "contactMethod"))
        return problem;
    for (const ModuleBrakeSystemDisk& disk : card.disks)
    {
        if (const auto problem = checkDisk(disk))
            return problem;
    }
    return std::nullopt;
}

void ModuleBrakeSystemFormat::write(const ModuleBrakeSystemCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.squealLoadFactor, 0.0);
    writer.integerUnlessDefault(3, card.frictionVectorMethod, 0);
    writeKeywordUnlessDefault(writer, 4, MODULE_BRAKE_SQUEAL_ONLY_WORDS, card.squealOnly,
                              ModuleBrakeSystemSquealOnly::Yes);
    writer.integerUnlessDefault(5, card.contactMethod, 0);
    int start = FIELDS_PER_LINE + 1;
    for (const ModuleBrakeSystemDisk& disk : card.disks)
    {
        writer.text(start, disk.name);
        writer.integerUnlessDefault(start + 1, disk.moduleId1, 0);
        writer.integer(start + 2, disk.body1Id);
        writer.integerUnlessDefault(start + 3, disk.moduleId2, 0);
        writer.integer(start + 4, disk.body2Id);
        writer.integerUnlessDefault(start + 5, disk.motion1Id, 0);
        writer.integerUnlessDefault(start + 6, disk.motion2Id, 0);
        writer.integerUnlessDefault(start + 7, disk.brakeSystemId, 0);
        start += FIELDS_PER_LINE;
    }
}

std::uint32_t ModuleBrakeSystemTable::append(const ModuleBrakeSystemCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleBrakeSystemTable::store(std::uint32_t row, const ModuleBrakeSystemCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleBrakeSystemTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleBrakeSystemTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleBrakeSystemTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleBrakeSystemCard& card = m_cards.edit(row);
    // A body is a BCBODY1 of its module, looked up there (ModuleBoundarySupport.hpp). The MOTION and BRKSYS numbers
    // name cards that are not typed yet.
    for (ModuleBrakeSystemDisk& disk : card.disks)
    {
        visitBoundaryId(visitor, "body1Id", ReferenceTarget::ContactBody, disk.moduleId1, disk.body1Id);
        visitBoundaryId(visitor, "body2Id", ReferenceTarget::ContactBody, disk.moduleId2, disk.body2Id);
    }
}

} // namespace oor::model
