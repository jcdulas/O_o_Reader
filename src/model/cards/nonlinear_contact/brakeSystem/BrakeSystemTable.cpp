#include "oor/model/cards/nonlinear_contact/brakeSystem/BrakeSystemTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// A disk is a line of its own after the first: the name in the first field of the line and five IDs after it.
constexpr int FIELDS_PER_LINE = 8;
constexpr int DISK_FIELDS = 6;

// The binary flags of the first line (IVEC, ISLIDEBS): 0 or 1.
std::optional<CardProblem> checkFlag(std::int32_t value, const char* field)
{
    if (value == 0 || value == 1)
        return std::nullopt;
    return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", field};
}

// The problem with the values of one disk line, if there is one.
std::optional<CardProblem> checkDisk(const BrakeSystemDisk& disk)
{
    if (const auto problem = checkNameWord(disk.name, "diskName"))
        return problem;
    if (const auto problem = checkRequiredId(disk.firstBodyId, "firstBodyId"))
        return problem;
    if (const auto problem = checkRequiredId(disk.secondBodyId, "secondBodyId"))
        return problem;
    if (const auto problem = checkRequiredId(disk.firstMotionId, "firstMotionId"))
        return problem;
    if (const auto problem = checkRequiredId(disk.secondMotionId, "secondMotionId"))
        return problem;
    return checkRequiredId(disk.brakePropId, "brakePropId");
}

} // namespace

std::optional<BrakeSystemCard> BrakeSystemFormat::read(FieldReader& reader)
{
    BrakeSystemCard card;
    card.id = reader.requiredInteger(1, "id");
    card.loadFactor = reader.real(2, "loadFactor", 0.0);
    card.frictionVectorMode = reader.integer(3, "frictionVectorMode", 0);
    card.squealOnly = readKeyword(reader, 4, "squealOnly", BRAKE_SQUEAL_ONLY_WORDS, BrakeSquealOnly::Yes);
    card.slidingContactOption = reader.integer(5, "slidingContactOption", 0);
    for (int field = 6; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the first line of a " +
                             std::string(reader.cardName()) + " is not used and must be blank",
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
        for (int field = start + DISK_FIELDS; field < start + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of a disk line of a " +
                                 std::string(reader.cardName()) + " is not used and must be blank",
                             "");
        }
        BrakeSystemDisk disk;
        disk.name = std::string(reader.text(start));
        disk.firstBodyId = reader.requiredInteger(start + 1, "firstBodyId");
        disk.secondBodyId = reader.requiredInteger(start + 2, "secondBodyId");
        disk.firstMotionId = reader.requiredInteger(start + 3, "firstMotionId");
        // MT2_ID: a blank field is the motion of the first body (format reference).
        disk.secondMotionId = reader.integer(start + 4, "secondMotionId", disk.firstMotionId);
        disk.brakePropId = reader.requiredInteger(start + 5, "brakePropId");
        card.disks.push_back(std::move(disk));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BrakeSystemFormat::check(const BrakeSystemCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.loadFactor, "loadFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkFlag(card.frictionVectorMode, "frictionVectorMode"))
        return problem;
    if (const auto problem = checkFlag(card.slidingContactOption, "slidingContactOption"))
        return problem;
    for (const BrakeSystemDisk& disk : card.disks)
    {
        if (const auto problem = checkDisk(disk))
            return problem;
    }
    return std::nullopt;
}

void BrakeSystemFormat::write(const BrakeSystemCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.loadFactor, 0.0);
    writer.integerUnlessDefault(3, card.frictionVectorMode, 0);
    writeKeywordUnlessDefault(writer, 4, BRAKE_SQUEAL_ONLY_WORDS, card.squealOnly, BrakeSquealOnly::Yes);
    writer.integerUnlessDefault(5, card.slidingContactOption, 0);
    int start = FIELDS_PER_LINE + 1;
    for (const BrakeSystemDisk& disk : card.disks)
    {
        writer.text(start, disk.name);
        writer.integer(start + 1, disk.firstBodyId);
        writer.integer(start + 2, disk.secondBodyId);
        writer.integer(start + 3, disk.firstMotionId);
        writer.integer(start + 4, disk.secondMotionId); // resolved from MT1_ID, so always written
        writer.integer(start + 5, disk.brakePropId);
        start += FIELDS_PER_LINE;
    }
}

std::uint32_t BrakeSystemTable::append(const BrakeSystemCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BrakeSystemTable::store(std::uint32_t row, const BrakeSystemCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BrakeSystemTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BrakeSystemTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BrakeSystemTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (BrakeSystemDisk& disk : m_cards.edit(row).disks)
    {
        visitId(visitor, "firstBodyId", ReferenceTarget::ContactBody, disk.firstBodyId);
        visitId(visitor, "secondBodyId", ReferenceTarget::ContactBody, disk.secondBodyId);
        visitId(visitor, "firstMotionId", ReferenceTarget::BrakeMotion, disk.firstMotionId);
        visitId(visitor, "secondMotionId", ReferenceTarget::BrakeMotion, disk.secondMotionId);
        visitId(visitor, "brakePropId", ReferenceTarget::BrakeProperty, disk.brakePropId);
    }
}

} // namespace oor::model
