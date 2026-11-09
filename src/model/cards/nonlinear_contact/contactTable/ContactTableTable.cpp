#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSectionFields.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <algorithm>
#include <array>

namespace oor::model
{

namespace
{

constexpr std::string_view SECNDRY = "Secondary";
constexpr std::string_view PRIMARY = "Primary";
constexpr std::string_view FBSH = "FrictionAndBias";
constexpr std::string_view BKGL = "GlueBreak";
constexpr std::string_view SEGS = "SegmentContact";
constexpr std::string_view HHHB = "HeatTransfer";

// The layout of the format reference, line by line (the keyword of a section, the field, the line of the section and
// the place in it). FRIC, IGLUE and JGLUE are in the same places in the SOL 700 form, whose lines follow the SECNDRY
// line with no keyword.
constexpr ContactFieldPosition POSITIONS[] = {
    // The SECNDRY line (IDSEC1 is field 2).
    {SECNDRY, {"ContactDistanceTolerance", ContactParameterKind::Real}, 0, 3},
    {SECNDRY, {"SeparationForce", ContactParameterKind::Real}, 0, 4},
    {SECNDRY, {"FrictionCoefficient", ContactParameterKind::Number}, 0, 5},
    {SECNDRY, {"InterferenceClosure", ContactParameterKind::Real}, 0, 6},
    {SECNDRY, {"GlueOption", ContactParameterKind::Integer}, 0, 7},
    // The line after it.
    {SECNDRY, {"ContactSearchOrder", ContactParameterKind::Integer}, 1, 2},
    {SECNDRY, {"StressFreeInitialContact", ContactParameterKind::Integer}, 1, 3},
    {SECNDRY, {"GlueSeparationOption", ContactParameterKind::Integer}, 1, 4},
    {SECNDRY, {"NearContactDistance", ContactParameterKind::Real}, 1, 6},
    // The lines of the SOL 700 form.
    {SECNDRY, {"KineticFrictionCoefficient", ContactParameterKind::Real}, 2, 2},
    {SECNDRY, {"FrictionDecayCoefficient", ContactParameterKind::Real}, 2, 3},
    {SECNDRY, {"ContactMethod", ContactParameterKind::Text}, 2, 4},
    {SECNDRY, {"AdaptiveContactSurface", ContactParameterKind::Text}, 2, 5},
    {SECNDRY, {"ContactThickness", ContactParameterKind::Real}, 2, 6},
    {SECNDRY, {"ThicknessOffset", ContactParameterKind::Real}, 2, 7},
    {SECNDRY, {"PenetrationLimit", ContactParameterKind::Real}, 2, 8},
    {SECNDRY, {"PenaltyScaleFactor", ContactParameterKind::Real}, 3, 2},
    {SECNDRY, {"ContactStartTime", ContactParameterKind::Real}, 3, 3},
    {SECNDRY, {"ContactEndTime", ContactParameterKind::Real}, 3, 4},
    {SECNDRY, {"MaxParametricCoordinate", ContactParameterKind::Real}, 3, 5},
    {SECNDRY, {"InitialPenetrationAdjustment", ContactParameterKind::Integer}, 4, 4},
    {SECNDRY, {"IgnoreInitialPenetration", ContactParameterKind::Integer}, 5, 4},
    {SECNDRY, {"AutomaticContact", ContactParameterKind::Text}, 6, 8},
    {SECNDRY, {"ContactSide", ContactParameterKind::Text}, 7, 2},
    {SECNDRY, {"ContactForceWeighting", ContactParameterKind::Text}, 7, 3},
    {SECNDRY, {"MonitorDisplacement", ContactParameterKind::Text}, 7, 4},
    {SECNDRY, {"MonitorDisplacementValue", ContactParameterKind::Real}, 7, 5},
    {SECNDRY, {"InitialMonitor", ContactParameterKind::Real}, 7, 6},
    {SECNDRY, {"ContactDamping", ContactParameterKind::Text}, 7, 7},
    // The FBSH line (the keyword is field 2).
    {FBSH, {"FrictionStressLimit", ContactParameterKind::Real}, 0, 3},
    {FBSH, {"ContactToleranceBias", ContactParameterKind::Real}, 0, 4},
    {FBSH, {"SlidingVelocityThreshold", ContactParameterKind::Real}, 0, 5},
    {FBSH, {"HardSoftRatio", ContactParameterKind::Real}, 0, 6},
    {FBSH, {"TouchingBodyOptions", ContactParameterKind::Integer}, 0, 7},
    {FBSH, {"TouchedBodyOptions", ContactParameterKind::Integer}, 0, 8},
    // The BKGL line.
    {BKGL, {"GlueBreakNormalStress", ContactParameterKind::Real}, 0, 3},
    {BKGL, {"GlueBreakShearStress", ContactParameterKind::Real}, 0, 4},
    {BKGL, {"GlueBreakNormalExponent", ContactParameterKind::Real}, 0, 5},
    {BKGL, {"GlueBreakShearExponent", ContactParameterKind::Real}, 0, 6},
    // The SEGS line.
    {SEGS, {"NormalPenaltyFactor", ContactParameterKind::Real}, 0, 3},
    {SEGS, {"AugmentationDistance", ContactParameterKind::Real}, 0, 4},
    {SEGS, {"TangentPenaltyFactor", ContactParameterKind::Real}, 0, 5},
    {SEGS, {"StickSlip", ContactParameterKind::Real}, 0, 6},
    // The HHHB line and the line after it.
    {HHHB, {"ContactHeatTransferCoefficient", ContactParameterKind::Number}, 0, 3},
    {HHHB, {"NearContactConvectionCoefficient", ContactParameterKind::Number}, 0, 4},
    {HHHB, {"NearContactNaturalConvectionCoefficient", ContactParameterKind::Number}, 0, 5},
    {HHHB, {"NearContactNaturalConvectionExponent", ContactParameterKind::Number}, 0, 6},
    {HHHB, {"Emissivity", ContactParameterKind::Number}, 0, 7},
    {HHHB, {"DistanceDependentConvectionCoefficient", ContactParameterKind::Number}, 0, 8},
    {HHHB, {"NearContactNonlinearConvectionCoefficient", ContactParameterKind::Number}, 1, 3},
    {HHHB, {"NearContactNonlinearConvectionExponent", ContactParameterKind::Number}, 1, 4},
    {HHHB, {"GlueHeatTransfer", ContactParameterKind::Integer}, 1, 5},
};

// The option keywords, which are in field 2 of their line.
constexpr std::string_view OPTIONS[] = {FBSH, BKGL, SEGS, HHHB};

// Writes a set from the line that starts at `lineStart`; returns the first field of the next line.
int writeGroup(FieldWriter& writer, int lineStart, const ContactTableGroup& group)
{
    int next = writeContactSectionFields(writer, lineStart, 1, SECNDRY, group.params, POSITIONS);
    if (group.secondaryBodyId != 0)
        writer.integer(lineStart + 1, group.secondaryBodyId);
    for (const std::string_view option : OPTIONS)
    {
        if (contactSectionLines(group.params, option, POSITIONS, 0) != 0)
            next = writeContactSectionFields(writer, next, 2, option, group.params, POSITIONS);
    }
    if (!group.primaryBodies.empty())
        next = writeContactIdList(writer, next, PRIMARY, group.primaryBodies);
    return next;
}

} // namespace

std::span<const ContactParameterRule> ContactTableFormat::rules()
{
    static const std::vector<ContactParameterRule> ALL = contactFieldRules(POSITIONS);
    return ALL;
}

std::optional<ContactTableCard> ContactTableFormat::read(FieldReader& reader)
{
    ContactTableCard card;
    card.id = reader.requiredInteger(1, "id");
    card.secondaryBodyId = reader.integer(2, "secondaryBodyId", 0);
    card.primaryBodyId = reader.integer(3, "primaryBodyId", 0);
    card.setCount = reader.integer(4, "setCount", 1);
    card.secondaryContactOption = reader.integer(5, "secondaryContactOption", 0);
    card.primaryContactOption = reader.integer(6, "primaryContactOption", 0);
    requireUnusedBlank(reader, {7, 8}, reader.lastUsedField());
    // SLAVE, MASTER and MASTERS (the public contact deck) are the older names of SECNDRY and PRIMARY.
    const std::vector<ContactSection> sections = splitContactSections(
        reader, {SECNDRY, "SecondaryBody", PRIMARY, "PrimaryBody", "PrimaryBodies", FBSH, BKGL, SEGS, HHHB});
    for (ContactSection section : sections)
    {
        if (section.keyword.empty())
        {
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error(
                    "card.bad-value",
                    "a continuation line of a " + std::string(reader.cardName()) + " starts with SECNDRY or PRIMARY",
                    "secondaryList");
            continue;
        }
        if (section.keyword == "SecondaryBody")
            section.keyword = SECNDRY;
        else if (section.keyword == "PrimaryBody" || section.keyword == "PrimaryBodies")
            section.keyword = PRIMARY;
        if (section.keyword == SECNDRY)
        {
            ContactTableGroup& group = card.groups.emplace_back();
            group.secondaryBodyId = reader.integer(section.first + 1, "setSecondaryBodyId", 0);
            readContactSectionFields(reader, section, POSITIONS, {2}, group.params, ContactTableTable::NAME);
        }
        else if (card.groups.empty())
        {
            reader.error("card.bad-value",
                         section.keyword + " comes before the first SECNDRY of the " + std::string(reader.cardName()),
                         "secondaryList");
            break;
        }
        else if (section.keyword == PRIMARY)
        {
            ContactTableGroup& group = card.groups.back();
            const std::vector<base::EntityId> ids =
                readContactIds(reader, section.first + 1, section.last, "primaryBodyId");
            group.primaryBodies.insert(group.primaryBodies.end(), ids.begin(), ids.end());
        }
        else
            readContactSectionFields(reader, section, POSITIONS, {}, card.groups.back().params,
                                     ContactTableTable::NAME);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactTableFormat::check(const ContactTableCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    if (card.secondaryBodyId < 0 || card.primaryBodyId < 0)
        return CardProblem{"card.bad-id", "a body ID must not be negative",
                           card.secondaryBodyId < 0 ? "secondaryBodyId" : "primaryBodyId"};
    if (card.setCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "setCount"};
    if (card.secondaryContactOption < 0 || card.primaryContactOption < 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("secondaryContactOption") + " and " +
                               CardProblem::fieldNamed("primaryContactOption") + " must not be negative",
                           card.secondaryContactOption < 0 ? "secondaryContactOption" : "primaryContactOption"};
    // the short form is two bodies and no continuation line.
    if (card.secondaryBodyId != 0 || card.primaryBodyId != 0)
    {
        if (card.secondaryBodyId == 0 || card.primaryBodyId == 0)
            return CardProblem{"card.missing-field",
                               "the short form has both " + CardProblem::fieldNamed("secondaryBodyId") + " and " +
                                   CardProblem::fieldNamed("primaryBodyId"),
                               "secondaryBodyId"};
        if (!card.groups.empty())
            return CardProblem{"card.bad-value",
                               "the short form (" + CardProblem::fieldNamed("secondaryBodyId") + " and " +
                                   CardProblem::fieldNamed("primaryBodyId") + ") has no continuation line",
                               "groups"};
    }
    for (const ContactTableGroup& group : card.groups)
    {
        if (group.secondaryBodyId < 0)
            return CardProblem{"card.bad-id", CardProblem::fieldNamed("setSecondaryBodyId") + " must not be negative",
                               "groups"};
        if (auto problem = checkContactParameters(group.params, rules(), ContactTableTable::NAME))
        {
            problem->field = "groups";
            return problem;
        }
        for (std::size_t index = 0; index < group.params.size(); ++index)
        {
            for (std::size_t other = 0; other < index; ++other)
            {
                if (group.params[index].name == group.params[other].name)
                    return CardProblem{"card.bad-value", group.params[index].name + " is given twice in a set",
                                       "groups"};
            }
        }
        for (const base::EntityId body : group.primaryBodies)
        {
            if (body <= 0)
                return CardProblem{"card.bad-id", "a touched body ID must be a positive integer", "groups"};
        }
    }
    return std::nullopt;
}

void ContactTableFormat::write(const ContactTableCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.secondaryBodyId, 0);
    writer.integerUnlessDefault(3, card.primaryBodyId, 0);
    writer.integerUnlessDefault(4, card.setCount, 1);
    writer.integerUnlessDefault(5, card.secondaryContactOption, 0);
    writer.integerUnlessDefault(6, card.primaryContactOption, 0);
    int line = firstFieldOfLine(1);
    for (const ContactTableGroup& group : card.groups)
        line = writeGroup(writer, line, group);
}

std::uint32_t ContactTableTable::append(const ContactTableCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactTableTable::store(std::uint32_t row, const ContactTableCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactTableTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactTableTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactTableTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactTableCard& card = m_cards.edit(row);
    visitContactId(card.secondaryBodyId, visitor, "secondaryBodyId", ReferenceTarget::ContactBody);
    visitContactId(card.primaryBodyId, visitor, "primaryBodyId", ReferenceTarget::ContactBody);
    for (ContactTableGroup& group : card.groups)
    {
        visitContactId(group.secondaryBodyId, visitor, "setSecondaryBodyId", ReferenceTarget::ContactBody);
        visitContactIds(group.primaryBodies, visitor, "primaryBodyId", ReferenceTarget::ContactBody);
    }
}

} // namespace oor::model
