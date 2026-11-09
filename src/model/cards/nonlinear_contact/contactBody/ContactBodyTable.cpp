#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <algorithm>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::string_view ADVANCE = "Advance";
constexpr std::string_view RIGID = "Rigid";
constexpr std::string_view HEAT = "HeatTransfer";
constexpr std::string_view PATCH3D = "PatchSurface";
constexpr std::string_view BEZIER = "BezierSurface";
constexpr std::string_view NURBS2D = "NurbsCurve";
constexpr std::string_view NURBS = "NurbsSurface";

// The name of a rigid body goes over three fields (24 characters).
constexpr int NAME_FIRST_OFFSET = 4;
constexpr int NAME_FIELDS = 3;

// A named field of the HEAT lines: its line (0 is the line of the keyword) and its field in that line.
struct HeatPosition
{
    ContactParameterRule rule;
    int line;
    int offset;
};

constexpr HeatPosition HEAT_POSITIONS[] = {
    {{"EnvironmentFilmCoefficient", ContactParameterKind::Number}, 0, 2},
    {{"SinkTemperature", ContactParameterKind::Number}, 0, 3},
    {{"ContactHeatTransferCoefficient", ContactParameterKind::Number}, 0, 4},
    {{"BodyTemperature", ContactParameterKind::Number}, 0, 5},
    {{"NearContactConvectionCoefficient", ContactParameterKind::Number}, 0, 6},
    {{"NearContactNaturalConvectionCoefficient", ContactParameterKind::Number}, 0, 7},
    {{"ThermalInterfaceKind", ContactParameterKind::Integer}, 0, 8},
    {{"NearContactNaturalConvectionExponent", ContactParameterKind::Number}, 1, 2},
    {{"Emissivity", ContactParameterKind::Number}, 1, 3},
    {{"DistanceDependentConvectionCoefficient", ContactParameterKind::Number}, 1, 4},
    {{"NearContactNonlinearConvectionCoefficient", ContactParameterKind::Number}, 1, 5},
    {{"NearContactNonlinearConvectionExponent", ContactParameterKind::Number}, 1, 6},
    {{"EnvironmentNonlinearConvectionCoefficient", ContactParameterKind::Number}, 1, 7},
    {{"EnvironmentNonlinearConvectionExponent", ContactParameterKind::Number}, 1, 8},
    {{"EnvironmentNaturalConvectionCoefficient", ContactParameterKind::Number}, 2, 2},
    {{"EnvironmentNaturalConvectionExponent", ContactParameterKind::Number}, 2, 3},
    // CMB and CMS: two reals of the thermal line whose use the format reference does not make clear; they are kept.
    {{"AdditionalHeatCoefficient1", ContactParameterKind::Real}, 2, 4},
    {{"AdditionalHeatCoefficient2", ContactParameterKind::Real}, 2, 5},
};

const HeatPosition* findHeatPosition(std::string_view name)
{
    for (const HeatPosition& position : HEAT_POSITIONS)
    {
        if (position.rule.name == name)
            return &position;
    }
    return nullptr;
}

// The line that starts at the first field of the line of `field`.
int lineStartOf(int field)
{
    return firstFieldOfLine((field - 1) / CONTACT_LINE_FIELDS);
}

void readHeat(FieldReader& reader, const ContactSection& section, ContactBodyCard& card)
{
    std::vector<ContactParameter> params;
    const int start = lineStartOf(section.first);
    std::vector<bool> used(static_cast<std::size_t>(section.last) + 1, false);
    used[static_cast<std::size_t>(section.first)] = true;
    for (const HeatPosition& position : HEAT_POSITIONS)
    {
        const int field = start + CONTACT_LINE_FIELDS * position.line + position.offset - 1;
        if (field > section.last)
            continue;
        used[static_cast<std::size_t>(field)] = true;
        if (reader.isBlank(field))
            continue;
        const ContactValue value = readContactParameterValue(reader, field, position.rule, ContactBodyTable::NAME);
        params.push_back(ContactParameter{std::string(position.rule.name), value});
    }
    for (int field = section.first + 1; field <= section.last; ++field)
    {
        if (!used[static_cast<std::size_t>(field)] && !reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the HEAT lines of a " +
                             std::string(reader.cardName()) + " is not used",
                         "heat");
            break;
        }
    }
    card.heat = std::move(params);
}

void readPatches(FieldReader& reader, const ContactSection& section, ContactBodyCard& card)
{
    const std::int32_t count = reader.requiredInteger(section.first + 1, "patchCount");
    std::vector<ContactPatch> patches;
    const int start = lineStartOf(section.first);
    for (int line = start + CONTACT_LINE_FIELDS; line <= section.last && !reader.failed(); line += CONTACT_LINE_FIELDS)
    {
        if (!reader.isBlank(line))
            reader.error("card.bad-value", "a patch line of a " + std::string(reader.cardName()) + " starts in field 2",
                         "patches");
        ContactPatch patch;
        patch.patchId = reader.requiredInteger(line + 1, "patchId");
        for (int corner = 0; corner < 4; ++corner)
            patch.grids[static_cast<std::size_t>(corner)] = reader.requiredInteger(line + 2 + corner, "node");
        patches.push_back(patch);
    }
    if (!reader.failed() && static_cast<std::int64_t>(patches.size()) != count)
        reader.error(
            "card.bad-value",
            "PATCH3D says " + std::to_string(count) + " patches and the card has " + std::to_string(patches.size()),
            "patchCount");
    card.patches = std::move(patches);
}

void readBezier(FieldReader& reader, const ContactSection& section, ContactBodyCard& card)
{
    ContactBodyBezier bezier;
    bezier.pointCount1 = reader.requiredInteger(section.first + 1, "pointCount1");
    bezier.pointCount2 = reader.requiredInteger(section.first + 2, "pointCount2");
    bezier.subdivisions1 = reader.requiredInteger(section.first + 3, "subdivisions1");
    bezier.subdivisions2 = reader.requiredInteger(section.first + 4, "subdivisions2");
    bezier.grids = readContactIds(reader, section.first + 5, section.last, "node");
    card.bezier = std::move(bezier);
}

ContactBodyNurbs readNurbs(FieldReader& reader, const ContactSection& section)
{
    ContactBodyNurbs nurbs;
    const int start = lineStartOf(section.first);
    for (int line = start; line <= section.last; line += CONTACT_LINE_FIELDS)
    {
        if (line != start && !reader.isBlank(line))
            reader.error("card.bad-value",
                         "a data line of a NURBS entry of a " + std::string(reader.cardName()) + " starts in field 2",
                         "nurbs");
        std::array<ContactValue, 7> values;
        for (int offset = 0; offset < 7; ++offset)
            values[static_cast<std::size_t>(offset)] = readContactValue(reader, line + 1 + offset, "nurbs");
        nurbs.lines.push_back(std::move(values));
    }
    return nurbs;
}

// Writes the keyword at the start of a line; returns the first field of the next line.
int writeNurbs(FieldWriter& writer, int lineStart, std::string_view keyword, const ContactBodyNurbs& nurbs)
{
    int line = lineStart;
    for (std::size_t index = 0; index < nurbs.lines.size(); ++index)
    {
        if (index == 0)
            writer.text(line, keyword);
        for (std::size_t offset = 0; offset < 7; ++offset)
            writeContactValue(writer, line + 1 + static_cast<int>(offset), nurbs.lines[index][offset]);
        line += CONTACT_LINE_FIELDS;
    }
    return line;
}

std::optional<CardProblem> checkNurbs(const ContactBodyNurbs& nurbs, const char* field)
{
    if (nurbs.lines.empty())
        return CardProblem{"card.missing-field", "a NURBS entry has at least its header line", field};
    for (const auto& line : nurbs.lines)
    {
        for (const ContactValue& value : line)
        {
            if (!isContactNumber(value))
                return CardProblem{"card.bad-value", "the data of a NURBS entry are integers and reals", field};
        }
    }
    return std::nullopt;
}

} // namespace

std::span<const ContactParameterRule> ContactBodyFormat::heatRules()
{
    static const std::vector<ContactParameterRule> ALL = []
    {
        std::vector<ContactParameterRule> rules;
        for (const HeatPosition& position : HEAT_POSITIONS)
            rules.push_back(position.rule);
        return rules;
    }();
    return ALL;
}

std::optional<ContactBodyCard> ContactBodyFormat::read(FieldReader& reader)
{
    ContactBodyCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dimension = readKeyword(reader, 2, "dimension", {"TwoDimensional", "ThreeDimensional"}, "ThreeDimensional");
    card.behavior =
        readKeyword(reader, 3, "behavior", {"Deformable", "Rigid", "Symmetric", "HeatTransfer"}, "Deformable");
    card.surfaceId = reader.integer(4, "surfaceId", 0);
    card.contactKind = reader.integer(5, "contactKind", 0);
    card.friction = readContactValue(reader, 6, "friction");
    card.splineSegmentsId = reader.integer(7, "splineSegmentsId", 0);
    card.control = reader.integer(8, "control", 0);
    const std::vector<ContactSection> sections =
        splitContactSections(reader, {ADVANCE, RIGID, APPROV, GROW, HEAT, PATCH3D, BEZIER, NURBS2D, NURBS});
    for (const ContactSection& section : sections)
    {
        const std::string& keyword = section.keyword;
        if (keyword.empty())
        {
            if (section.last > 2 * CONTACT_LINE_FIELDS)
                reader.error(
                    "card.bad-value",
                    "a " + std::string(reader.cardName()) + " has one unnamed continuation line, the one of the loads",
                    "loadNode");
            if (section.last >= 2 * CONTACT_LINE_FIELDS)
                card.motion = readContactMotion(reader, firstFieldOfLine(1));
        }
        else if (keyword == ADVANCE)
        {
            ContactBodyAdvance advance;
            advance.smoothingAngle = reader.real(section.first + 1, "smoothingAngle", 60.0);
            advance.contactOption = reader.integer(section.first + 2, "contactOption", 0);
            advance.midsideProjection = reader.integer(section.first + 4, "midsideProjection", 0);
            // The other lines of the card come after this one: only its own blank cells are looked at.
            requireUnusedBlank(reader, {section.first + 3, section.first + 5, section.first + 6, section.first + 7},
                               reader.lastUsedField());
            card.advance = advance;
        }
        else if (keyword == RIGID)
        {
            ContactBodyRigid rigid;
            rigid.referenceNode = reader.integer(section.first + 1, "referenceNode", 0);
            rigid.entityCount = reader.integer(section.first + 2, "entityCount", 1);
            rigid.name = reader.spanText(section.first + NAME_FIRST_OFFSET - 1,
                                         section.first + NAME_FIRST_OFFSET + NAME_FIELDS - 2);
            requireUnusedBlank(reader, {section.first + 6, section.first + 7}, reader.lastUsedField());
            card.rigid = std::move(rigid);
        }
        else if (keyword == APPROV)
            card.approach = readContactApproach(reader, section);
        else if (keyword == GROW)
            card.growth = readContactGrowth(reader, section);
        else if (keyword == HEAT)
            readHeat(reader, section, card);
        else if (keyword == PATCH3D)
            readPatches(reader, section, card);
        else if (keyword == BEZIER)
            readBezier(reader, section, card);
        else if (keyword == NURBS2D)
            card.nurbs2d = readNurbs(reader, section);
        else
            card.nurbs = readNurbs(reader, section);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBodyFormat::check(const ContactBodyCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (auto problem = checkKeyword(card.dimension, "dimension", {"TwoDimensional", "ThreeDimensional"}))
        return problem;
    if (auto problem = checkKeyword(card.behavior, "behavior", {"Deformable", "Rigid", "Symmetric", "HeatTransfer"}))
        return problem;
    if (card.surfaceId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "surfaceId"};
    if (card.contactKind < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "contactKind"};
    if (!isContactNumber(card.friction))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a real or the ID of a table",
                           "friction"};
    if (const auto* real = std::get_if<double>(&card.friction); real != nullptr && *real < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "friction"};
    if (const auto* table = std::get_if<std::int64_t>(&card.friction); table != nullptr && *table <= 0)
        return CardProblem{"card.bad-value",
                           "the ID of the table of " + std::string(CardProblem::FIELD) + " must be positive",
                           "friction"};
    if (card.control < -1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is -1, 0 or a grid point number",
                           "control"};
    if (auto problem = checkContactMotion(card.motion))
        return problem;
    if (card.advance)
    {
        if (!std::isfinite(card.advance->smoothingAngle))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "smoothingAngle"};
        if (card.advance->contactOption < 0 || card.advance->midsideProjection < 0)
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("contactOption") + " and " +
                                   CardProblem::fieldNamed("midsideProjection") + " must not be negative",
                               "contactOption"};
    }
    if (card.rigid)
    {
        if (card.rigid->referenceNode < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a grid point ID, or blank",
                               "referenceNode"};
        if (card.rigid->entityCount < 1)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be positive", "entityCount"};
        if (card.rigid->name.find_first_of(",$\t") != std::string::npos)
            return CardProblem{"card.bad-value", "the rigid body name has no comma, $ or tab", "rigid"};
        if (!card.rigid->name.empty() && (card.rigid->name.front() == ' ' || card.rigid->name.back() == ' '))
            return CardProblem{"card.bad-value", "the rigid body name does not begin or end with a blank", "rigid"};
    }
    if (card.approach)
    {
        if (auto problem = checkContactApproach(*card.approach))
            return problem;
    }
    if (card.growth)
    {
        if (auto problem = checkContactGrowth(*card.growth))
            return problem;
    }
    if (card.heat)
    {
        if (auto problem = checkContactParameters(*card.heat, heatRules(), ContactBodyTable::NAME))
        {
            problem->field = "heat";
            return problem;
        }
    }
    const int geometries =
        (card.patches ? 1 : 0) + (card.bezier ? 1 : 0) + (card.nurbs2d ? 1 : 0) + (card.nurbs ? 1 : 0);
    if (geometries > 1)
        return CardProblem{"card.bad-value", "only one of PATCH3D, BEZIER, NURBS2D and NURBS describes the surface",
                           "patches"};
    if (card.patches)
    {
        if (card.patches->empty())
            return CardProblem{"card.missing-field", "PATCH3D needs at least one patch", "patches"};
        for (const ContactPatch& patch : *card.patches)
        {
            if (patch.patchId <= 0)
                return CardProblem{
                    "card.bad-id",
                    "the patch number " + std::string(CardProblem::FIELD) + " must be a positive integer", "patchId"};
            for (const base::EntityId grid : patch.grids)
            {
                if (grid <= 0)
                    return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "patches"};
            }
        }
    }
    if (card.bezier)
    {
        const ContactBodyBezier& bezier = *card.bezier;
        if (bezier.pointCount1 <= 0 || bezier.pointCount2 <= 0 || bezier.subdivisions1 <= 0 ||
            bezier.subdivisions2 <= 0)
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("pointCount1") + ", " + CardProblem::fieldNamed("pointCount2") +
                                   ", " + CardProblem::fieldNamed("subdivisions1") + " and " +
                                   CardProblem::fieldNamed("subdivisions2") + " must be positive",
                               "pointCount1"};
        if (static_cast<std::int64_t>(bezier.grids.size()) !=
            static_cast<std::int64_t>(bezier.pointCount1) * bezier.pointCount2)
            return CardProblem{"card.bad-value",
                               "BEZIER needs " + CardProblem::fieldNamed("pointCount1") + " x " +
                                   CardProblem::fieldNamed("pointCount2") + " grid points",
                               "node"};
        for (const base::EntityId grid : bezier.grids)
        {
            if (grid <= 0)
                return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "bezier"};
        }
    }
    if (card.nurbs2d)
    {
        if (auto problem = checkNurbs(*card.nurbs2d, "nurbs2d"))
            return problem;
    }
    if (card.nurbs)
    {
        if (auto problem = checkNurbs(*card.nurbs, "nurbs"))
            return problem;
    }
    return std::nullopt;
}

void ContactBodyFormat::write(const ContactBodyCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.dimension, "ThreeDimensional");
    writer.textUnlessDefault(3, card.behavior, "Deformable");
    writer.integerUnlessDefault(4, card.surfaceId, 0);
    writer.integerUnlessDefault(5, card.contactKind, 0);
    writeContactValue(writer, 6, card.friction);
    writer.integerUnlessDefault(7, card.splineSegmentsId, 0);
    writer.integerUnlessDefault(8, card.control, 0);
    int line = writeContactMotion(writer, firstFieldOfLine(1), card.motion);
    if (card.advance)
    {
        writer.text(line, ADVANCE);
        writer.realUnlessDefault(line + 1, card.advance->smoothingAngle, 60.0);
        writer.integerUnlessDefault(line + 2, card.advance->contactOption, 0);
        writer.integerUnlessDefault(line + 4, card.advance->midsideProjection, 0);
        line += CONTACT_LINE_FIELDS;
    }
    if (card.rigid)
    {
        writer.text(line, RIGID);
        writer.integerUnlessDefault(line + 1, card.rigid->referenceNode, 0);
        writer.integerUnlessDefault(line + 2, card.rigid->entityCount, 1);
        if (!card.rigid->name.empty())
            writer.spanText(line + NAME_FIRST_OFFSET - 1, NAME_FIELDS, card.rigid->name);
        line += CONTACT_LINE_FIELDS;
    }
    if (card.approach)
        line = writeContactApproach(writer, line, *card.approach);
    if (card.growth)
        line = writeContactGrowth(writer, line, *card.growth);
    if (card.heat)
    {
        writer.text(line, HEAT);
        int lines = 1;
        for (const ContactParameter& parameter : *card.heat)
        {
            const HeatPosition* position = findHeatPosition(parameter.name);
            if (position == nullptr)
                continue;
            writeContactValue(writer, line + CONTACT_LINE_FIELDS * position->line + position->offset - 1,
                              parameter.value);
            lines = std::max(lines, position->line + 1);
        }
        line += CONTACT_LINE_FIELDS * lines;
    }
    if (card.patches)
    {
        writer.text(line, PATCH3D);
        writer.integer(line + 1, static_cast<std::int64_t>(card.patches->size()));
        line += CONTACT_LINE_FIELDS;
        for (const ContactPatch& patch : *card.patches)
        {
            writer.integer(line + 1, patch.patchId);
            for (int corner = 0; corner < 4; ++corner)
                writer.integer(line + 2 + corner, patch.grids[static_cast<std::size_t>(corner)]);
            line += CONTACT_LINE_FIELDS;
        }
    }
    if (card.bezier)
    {
        writer.text(line, BEZIER);
        writer.integer(line + 1, card.bezier->pointCount1);
        writer.integer(line + 2, card.bezier->pointCount2);
        writer.integer(line + 3, card.bezier->subdivisions1);
        writer.integer(line + 4, card.bezier->subdivisions2);
        line = writeContactIdList(writer, line + CONTACT_LINE_FIELDS, "", card.bezier->grids);
    }
    if (card.nurbs2d)
        line = writeNurbs(writer, line, NURBS2D, *card.nurbs2d);
    if (card.nurbs)
        writeNurbs(writer, line, NURBS, *card.nurbs);
}

std::uint32_t ContactBodyTable::append(const ContactBodyCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactBodyTable::store(std::uint32_t row, const ContactBodyCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactBodyTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactBodyTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactBodyTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactBodyCard& card = m_cards.edit(row);
    visitContactId(card.surfaceId, visitor, "surfaceId", ReferenceTarget::ContactSurface);
    if (card.control > 0)
        visitContactId(card.control, visitor, "control", ReferenceTarget::Node);
    visitContactId(card.motion.loadNode, visitor, "loadNode", ReferenceTarget::Node);
    if (card.rigid)
        visitContactId(card.rigid->referenceNode, visitor, "referenceNode", ReferenceTarget::Node);
    if (card.patches)
    {
        for (ContactPatch& patch : *card.patches)
        {
            for (base::EntityId& grid : patch.grids)
                visitContactId(grid, visitor, "node", ReferenceTarget::Node);
        }
    }
    if (card.bezier)
        visitContactIds(card.bezier->grids, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
