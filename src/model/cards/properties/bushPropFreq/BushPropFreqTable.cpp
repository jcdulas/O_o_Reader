#include "oor/model/cards/properties/bushPropFreq/BushPropFreqTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;

struct NamedRule
{
    const char* name;
    ForceDeflectionRule rule;
};
constexpr NamedRule RULES[] = {{"TransverseXy", ForceDeflectionRule::Trxy},
                               {"TransverseXz", ForceDeflectionRule::Trxz},
                               {"TransverseYz", ForceDeflectionRule::Tryz},
                               {"TransverseShear", ForceDeflectionRule::Ts}};

struct NamedOption
{
    const char* name;
    FuseOption option;
};
constexpr NamedOption OPTIONS[] = {{"RelativeDisplacement", FuseOption::RelativeDisplacement},
                                   {"UltimateLoad", FuseOption::UltimateLoad},
                                   {"ActualUltimateLoad", FuseOption::ActualUltimateLoad}};

const char* ruleName(ForceDeflectionRule rule)
{
    for (const NamedRule& entry : RULES)
        if (entry.rule == rule)
            return entry.name;
    return "";
}

const char* optionName(FuseOption option)
{
    for (const NamedOption& entry : OPTIONS)
        if (entry.option == option)
            return entry.name;
    return "RelativeDisplacement";
}

// The fields from `first` to `last` must be blank.
void requireBlank(FieldReader& reader, int first, int last, const char* name)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "this field of a " + std::string(reader.cardName()) + " must be blank",
                         name);
            return;
        }
    }
}

// Six table IDs from `first`; blank is 0. Returns true when any of them is given.
bool readTables(FieldReader& reader, int first, const char* const (&names)[6], std::array<base::EntityId, 6>& ids)
{
    bool given = false;
    for (int index = 0; index < 6; ++index)
    {
        given = given || !reader.isBlank(first + index);
        ids[static_cast<std::size_t>(index)] = reader.integer(first + index, names[index], 0);
    }
    return given;
}

bool anyGiven(const std::array<base::EntityId, 6>& ids)
{
    for (const base::EntityId id : ids)
    {
        if (id != 0)
            return true;
    }
    return false;
}

void writeTables(FieldWriter& writer, int start, const char* flag, const std::array<base::EntityId, 6>& ids,
                 bool explicitZeros)
{
    writer.text(start + 1, flag);
    for (int index = 0; index < 6; ++index)
    {
        if (explicitZeros)
            writer.integer(start + 2 + index, ids[static_cast<std::size_t>(index)]);
        else
            writer.integerUnlessDefault(start + 2 + index, ids[static_cast<std::size_t>(index)], 0);
    }
}

} // namespace

std::optional<BushPropFreqCard> BushPropFreqFormat::read(FieldReader& reader)
{
    static constexpr const char* K_NAMES[6] = {"stiffnessTableId1", "stiffnessTableId2", "stiffnessTableId3",
                                               "stiffnessTableId4", "stiffnessTableId5", "stiffnessTableId6"};
    static constexpr const char* B_NAMES[6] = {"dampingTableId1", "dampingTableId2", "dampingTableId3",
                                               "dampingTableId4", "dampingTableId5", "dampingTableId6"};
    static constexpr const char* STRUCTURAL_DAMPING_TABLE_NAMES[6] = {
        "structuralDampingTableId1", "structuralDampingTableId2", "structuralDampingTableId3",
        "structuralDampingTableId4", "structuralDampingTableId5", "structuralDampingTableId6"};
    static constexpr const char* NONLINEAR_FORCE_NAMES[6] = {"nonlinearForceTableId1", "nonlinearForceTableId2",
                                                             "nonlinearForceTableId3", "nonlinearForceTableId4",
                                                             "nonlinearForceTableId5", "nonlinearForceTableId6"};

    BushPropFreqCard card;
    card.id = reader.requiredInteger(1, "id");

    unsigned seen = 0;   // bit per group already read
    int optionLines = 0; // the lines with a blank flag field: the fuse lines
    for (int start = 1; start <= reader.lastUsedField(); start += FIELDS_PER_LINE)
    {
        if (start > 1 && !reader.isBlank(start))
            reader.error("card.bad-value",
                         "the first field of a continuation line of a " + std::string(reader.cardName()) + " is blank",
                         "id");
        const std::string_view flag = reader.text(start + 1);
        if (flag.empty())
        {
            // A fuse line: FDC FUSE DIR OPTION LOWER UPPER, then FSRS LRGR.
            if (optionLines == 0)
            {
                if (const std::string_view rule = reader.text(start + 2);
                    !rule.empty() && !(rule == "NonlinearResponse"))
                {
                    bool found = false;
                    for (const NamedRule& entry : RULES)
                    {
                        if ((rule == entry.name))
                        {
                            card.forceDeflectionRule = entry.rule;
                            found = true;
                        }
                    }
                    if (!found)
                        reader.error("card.bad-value",
                                     std::string("forceDeflectionRule") +
                                         " must be NR, TRXY, TRXZ, TRYZ, TS or blank, found \"" + std::string(rule) +
                                         "\"",
                                     "forceDeflectionRule");
                }
                card.fuseMode = reader.integer(start + 3, "fuseMode", 0);
                card.direction = base::sortedDigits(reader.integer(start + 4, "direction", 0));
                if (const std::string_view option = reader.text(start + 5); !option.empty())
                {
                    bool found = false;
                    for (const NamedOption& entry : OPTIONS)
                    {
                        if ((option == entry.name))
                        {
                            card.fuseOption = entry.option;
                            found = true;
                        }
                    }
                    if (!found)
                        reader.error("card.bad-value",
                                     std::string("fuseOption") + " must be RELDIS, ULTLD or RULTLD, found \"" +
                                         std::string(option) + "\"",
                                     "fuseOption");
                }
                card.lowerBound = reader.real(start + 6, "lowerBound", 0.0);
                card.upperBound = reader.real(start + 7, "upperBound", 0.0);
            }
            else if (optionLines == 1)
            {
                card.fuseStiffnessRetention = reader.real(start + 2, "fuseStiffnessRetention",
                                                          BushPropFreqCard::DEFAULT_FUSE_STIFFNESS_RETENTION);
                card.largeRotationEndA = reader.integer(start + 3, "largeRotationEndA", 0);
                requireBlank(reader, start + 4, start + 7, "largeRotationEndA");
            }
            else
            {
                reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has at most two fuse lines",
                             "largeRotationEndA");
            }
            ++optionLines;
            continue;
        }
        if (optionLines > 0)
            reader.error(
                "card.bad-value",
                "the K, B, GE and KN lines of a " + std::string(reader.cardName()) + " come before the fuse lines",
                "id");
        const auto is = [&](const char* name) { return (flag == name); };
        unsigned bit = 0;
        if (is("Stiffness"))
        {
            bit = 1;
            readTables(reader, start + 2, K_NAMES, card.stiffnessTableIds);
        }
        else if (is("Damping"))
        {
            bit = 2;
            readTables(reader, start + 2, B_NAMES, card.dampingTableIds);
        }
        else if (is("StructuralDamping"))
        {
            bit = 4;
            readTables(reader, start + 2, STRUCTURAL_DAMPING_TABLE_NAMES, card.structuralDampingTableIds);
            // TGEID1 alone, with TGEID2 to TGEID6 blank, is the table of all six directions.
            bool othersBlank = true;
            for (int index = 1; index < 6; ++index)
                othersBlank = othersBlank && reader.isBlank(start + 2 + index);
            if (othersBlank)
                card.structuralDampingTableIds.fill(card.structuralDampingTableIds[0]);
        }
        else if (is("NonlinearForce"))
        {
            bit = 8;
            readTables(reader, start + 2, NONLINEAR_FORCE_NAMES, card.nonlinearForceTableIds);
        }
        else
        {
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " starts with K, B, GE or KN, found \"" +
                             std::string(flag) + "\"",
                         "id");
            continue;
        }
        if ((seen & bit) != 0)
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " may be given only once: \"" +
                             std::string(flag) + "\"",
                         "id");
        seen |= bit;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushPropFreqFormat::check(const BushPropFreqCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    const std::array<base::EntityId, 6>* const lists[4] = {
        &card.stiffnessTableIds, &card.dampingTableIds, &card.structuralDampingTableIds, &card.nonlinearForceTableIds};
    const char* const names[4] = {"stiffnessTableId1", "dampingTableId1", "structuralDampingTableId1",
                                  "nonlinearForceTableId1"};
    for (int list = 0; list < 4; ++list)
    {
        for (const base::EntityId id : *lists[list])
        {
            if (id < 0)
                return CardProblem{"card.bad-id", "the table IDs must not be negative", names[list]};
        }
    }
    if (card.fuseMode < 0 || card.fuseMode > 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1 or 2", "fuseMode"};
    if (!base::isComponentSet(card.direction))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 0 or digits 1 to 6, each at most once",
                           "direction"};
    if (!(card.fuseStiffnessRetention > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "fuseStiffnessRetention"};
    if (card.largeRotationEndA < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "largeRotationEndA"};
    return std::nullopt;
}

void BushPropFreqFormat::write(const BushPropFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int start = 1; // the first field of the line
    if (anyGiven(card.stiffnessTableIds))
    {
        writeTables(writer, start, "Stiffness", card.stiffnessTableIds, false);
        start += FIELDS_PER_LINE;
    }
    if (anyGiven(card.dampingTableIds))
    {
        writeTables(writer, start, "Damping", card.dampingTableIds, false);
        start += FIELDS_PER_LINE;
    }
    if (anyGiven(card.structuralDampingTableIds))
    {
        // TGEID2 to TGEID6 all blank would mean "TGEID1 for all": they are always written.
        writeTables(writer, start, "StructuralDamping", card.structuralDampingTableIds, true);
        start += FIELDS_PER_LINE;
    }
    if (anyGiven(card.nonlinearForceTableIds))
    {
        writeTables(writer, start, "NonlinearForce", card.nonlinearForceTableIds, false);
        start += FIELDS_PER_LINE;
    }
    const bool fuseLine = card.forceDeflectionRule != ForceDeflectionRule::None || card.fuseMode != 0 ||
                          card.direction != 0 || card.fuseOption != FuseOption::RelativeDisplacement ||
                          card.lowerBound != 0.0 || card.upperBound != 0.0;
    const bool retentionLine = card.fuseStiffnessRetention != BushPropFreqCard::DEFAULT_FUSE_STIFFNESS_RETENTION ||
                               card.largeRotationEndA != 0;
    if (fuseLine)
    {
        if (card.forceDeflectionRule != ForceDeflectionRule::None)
            writer.text(start + 2, ruleName(card.forceDeflectionRule));
        writer.integerUnlessDefault(start + 3, card.fuseMode, 0);
        writer.integerUnlessDefault(start + 4, card.direction, 0);
        writer.textUnlessDefault(start + 5, optionName(card.fuseOption), "RelativeDisplacement");
        writer.realUnlessDefault(start + 6, card.lowerBound, 0.0);
        writer.realUnlessDefault(start + 7, card.upperBound, 0.0);
    }
    if (retentionLine)
    {
        start += FIELDS_PER_LINE; // the second fuse line, after the first one even when that one is blank
        writer.realUnlessDefault(start + 2, card.fuseStiffnessRetention,
                                 BushPropFreqCard::DEFAULT_FUSE_STIFFNESS_RETENTION);
        writer.integerUnlessDefault(start + 3, card.largeRotationEndA, 0);
    }
}

std::uint32_t BushPropFreqTable::append(const BushPropFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffnessTableIds.push(card.stiffnessTableIds);
    m_dampingTableIds.push(card.dampingTableIds);
    m_structuralDampingTableIds.push(card.structuralDampingTableIds);
    m_nonlinearForceTableIds.push(card.nonlinearForceTableIds);
    m_forceDeflectionRule.push(static_cast<std::uint8_t>(card.forceDeflectionRule));
    m_fuseMode.push(static_cast<std::uint8_t>(card.fuseMode));
    m_direction.push(card.direction);
    m_fuseOption.push(static_cast<std::uint8_t>(card.fuseOption));
    m_bounds.push({card.lowerBound, card.upperBound});
    m_fuseStiffnessRetention.push(card.fuseStiffnessRetention);
    m_largeRotationEndA.push(card.largeRotationEndA);
    return row;
}

BushPropFreqCard BushPropFreqTable::card(std::uint32_t row) const
{
    BushPropFreqCard card;
    card.id = m_id.get(row);
    card.stiffnessTableIds = m_stiffnessTableIds.get(row);
    card.dampingTableIds = m_dampingTableIds.get(row);
    card.structuralDampingTableIds = m_structuralDampingTableIds.get(row);
    card.nonlinearForceTableIds = m_nonlinearForceTableIds.get(row);
    card.forceDeflectionRule = static_cast<ForceDeflectionRule>(m_forceDeflectionRule.get(row));
    card.fuseMode = m_fuseMode.get(row);
    card.direction = m_direction.get(row);
    card.fuseOption = static_cast<FuseOption>(m_fuseOption.get(row));
    const std::array<double, 2> bounds = m_bounds.get(row);
    card.lowerBound = bounds[0];
    card.upperBound = bounds[1];
    card.fuseStiffnessRetention = m_fuseStiffnessRetention.get(row);
    card.largeRotationEndA = m_largeRotationEndA.get(row);
    return card;
}

void BushPropFreqTable::store(std::uint32_t row, const BushPropFreqCard& card)
{
    m_id.set(row, card.id);
    m_stiffnessTableIds.set(row, card.stiffnessTableIds);
    m_dampingTableIds.set(row, card.dampingTableIds);
    m_structuralDampingTableIds.set(row, card.structuralDampingTableIds);
    m_nonlinearForceTableIds.set(row, card.nonlinearForceTableIds);
    m_forceDeflectionRule.set(row, static_cast<std::uint8_t>(card.forceDeflectionRule));
    m_fuseMode.set(row, static_cast<std::uint8_t>(card.fuseMode));
    m_direction.set(row, card.direction);
    m_fuseOption.set(row, static_cast<std::uint8_t>(card.fuseOption));
    m_bounds.set(row, {card.lowerBound, card.upperBound});
    m_fuseStiffnessRetention.set(row, card.fuseStiffnessRetention);
    m_largeRotationEndA.set(row, card.largeRotationEndA);
}

void BushPropFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffnessTableIds.keepRows(rows);
    m_dampingTableIds.keepRows(rows);
    m_structuralDampingTableIds.keepRows(rows);
    m_nonlinearForceTableIds.keepRows(rows);
    m_forceDeflectionRule.keepRows(rows);
    m_fuseMode.keepRows(rows);
    m_direction.keepRows(rows);
    m_fuseOption.keepRows(rows);
    m_bounds.keepRows(rows);
    m_fuseStiffnessRetention.keepRows(rows);
    m_largeRotationEndA.keepRows(rows);
}

void BushPropFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PBUSH that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Property, m_id.get(row)));
    const auto visitTables = [&](Column<std::array<base::EntityId, 6>>& column, const char* field)
    {
        std::array<base::EntityId, 6> ids = column.get(row);
        bool changed = false;
        for (std::size_t direction = 0; direction < ids.size(); ++direction)
        {
            base::EntityId& id = ids[direction];
            if (id <= 0)
                continue;
            // The field of each direction is numbered from 1 (stiffnessTableId1 to stiffnessTableId6).
            const std::string name = std::string(field) + std::to_string(direction + 1);
            const base::EntityId updated = visitor.visit(name, ReferenceTarget::DynamicTable, id);
            changed = changed || updated != id;
            id = updated;
        }
        if (changed)
            column.set(row, ids);
    };
    visitTables(m_stiffnessTableIds, "stiffnessTableId");
    visitTables(m_dampingTableIds, "dampingTableId");
    visitTables(m_structuralDampingTableIds, "structuralDampingTableId");
    visitTables(m_nonlinearForceTableIds, "nonlinearForceTableId");
}

} // namespace oor::model
