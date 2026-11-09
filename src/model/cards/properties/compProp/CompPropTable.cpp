#include "oor/model/cards/properties/compProp/CompPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_PLY_FIELD = 9;
constexpr int FIELDS_PER_PLY = 4;

struct NamedTheory
{
    const char* name;
    FailureTheory theory;
};
constexpr NamedTheory THEORIES[] = {{"Hill", FailureTheory::Hill},
                                    {"Hoffman", FailureTheory::Hoffman},
                                    {"TsaiWu", FailureTheory::TsaiWu},
                                    {"StrainTheory", FailureTheory::MaxStrain},
                                    {"HashinCriterion", FailureTheory::HashinFailure},
                                    {"HashinTape", FailureTheory::HashinTape},
                                    {"HashinFabricFailure", FailureTheory::HashinFabric}};

struct NamedOption
{
    const char* name;
    LaminateOption option;
};
constexpr NamedOption OPTIONS[] = {{"Symmetry", LaminateOption::Sym},
                                   {"MembraneOnly", LaminateOption::Mem},
                                   {"BendingOnly", LaminateOption::Bend},
                                   {"SmearedLaminate", LaminateOption::Smear},
                                   {"SandwichCore", LaminateOption::SmCore}};

const char* theoryName(FailureTheory theory)
{
    for (const NamedTheory& entry : THEORIES)
        if (entry.theory == theory)
            return entry.name;
    return "";
}

const char* optionName(LaminateOption option)
{
    for (const NamedOption& entry : OPTIONS)
        if (entry.option == option)
            return entry.name;
    return "";
}

} // namespace

std::optional<CompPropCard> CompPropFormat::read(FieldReader& reader)
{
    CompPropCard card;
    card.id = reader.requiredInteger(1, "id");
    const auto bottomOffset = reader.optionalReal(2, "bottomOffset");
    card.nonStructMass = reader.real(3, "nonStructMass", 0.0);
    card.bondShearAllowable = reader.real(4, "bondAllowable", 0.0);
    if (const std::string_view theory = reader.text(5); !theory.empty())
    {
        bool found = false;
        for (const NamedTheory& entry : THEORIES)
        {
            if ((theory == entry.name))
            {
                card.failureTheory = entry.theory;
                found = true;
            }
        }
        if (!found)
            reader.error("card.bad-value",
                         std::string("failureTheory") +
                             " must be HILL, HOFF, TSAI, STRN, HFAIL, HTAPE, HFABR or blank, found \"" +
                             std::string(theory) + "\"",
                         "failureTheory");
    }
    card.referenceTemperature = reader.real(6, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(7, "structuralDamping", 0.0);
    if (const std::string_view option = reader.text(8); !option.empty())
    {
        bool found = false;
        for (const NamedOption& entry : OPTIONS)
        {
            if ((option == entry.name))
            {
                card.option = entry.option;
                found = true;
            }
        }
        if (!found)
            reader.error("card.bad-value",
                         std::string("laminateOption") + " must be SYM, MEM, BEND, SMEAR, SMCORE or blank, found \"" +
                             std::string(option) + "\"",
                         "laminateOption");
    }

    // Plies: groups of four fields from field 9. A group with nothing in it is padding (one ply per line format).
    base::EntityId lastMatId = 0;
    double lastThickness = 0.0;
    bool haveLast = false;
    for (int field = FIRST_PLY_FIELD; field <= reader.lastUsedField(); field += FIELDS_PER_PLY)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2) &&
            reader.isBlank(field + 3))
            continue;
        PlyCard ply;
        ply.matId = reader.integer(field, "plyMatId",
                                   lastMatId); // format reference: a blank MID or T repeats the previous ply's
        ply.thickness = reader.real(field + 1, "plyThickness", lastThickness);
        ply.thetaInDegrees = reader.real(field + 2, "plyAngle", 0.0);
        const std::string_view stressOutput = reader.text(field + 3);
        ply.stressOutput = (stressOutput == "Yes");
        if (!stressOutput.empty() && !ply.stressOutput && !(stressOutput == "No"))
            reader.error(
                "card.bad-value",
                std::string("plyStressOutput") + " must be YES or NO, found \"" + std::string(stressOutput) + "\"",
                "plyStressOutput");
        if (!haveLast && (reader.isBlank(field) || reader.isBlank(field + 1)))
            reader.error("card.missing-field", "the first ply needs a material and a thickness", "plyMatId1");
        lastMatId = ply.matId;
        lastThickness = ply.thickness;
        haveLast = true;
        card.plies.push_back(ply);
    }
    // format reference: Z0 defaults to minus half the thickness of the laminate. Stored resolved.
    card.bottomOffset = bottomOffset.value_or(-0.5 * card.totalThickness());
    card.bottomOffsetGiven = bottomOffset.has_value();
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CompPropFormat::check(const CompPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.plies.empty())
        return CardProblem{"card.missing-field", "a laminate needs at least one ply", "plyMatId1"};
    for (const PlyCard& ply : card.plies)
    {
        if (ply.matId <= 0)
            return CardProblem{"card.bad-id", "a ply needs a material ID", "plyMatId1"};
        if (ply.thickness < 0.0)
            return CardProblem{"card.bad-value", "the thickness of a ply must not be negative", "plyThickness1"};
    }
    return std::nullopt;
}

void CompPropFormat::write(const CompPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.bottomOffsetGiven) // a blank is -T/2 of the plies: written blank, so that it follows them when read again
        writer.real(2, card.bottomOffset);
    writer.realUnlessDefault(3, card.nonStructMass, 0.0);
    writer.realUnlessDefault(4, card.bondShearAllowable, 0.0);
    if (card.failureTheory != FailureTheory::None)
        writer.text(5, theoryName(card.failureTheory));
    writer.realUnlessDefault(6, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(7, card.structuralDamping, 0.0);
    if (card.option != LaminateOption::None)
        writer.text(8, optionName(card.option));
    int field = FIRST_PLY_FIELD;
    for (const PlyCard& ply : card.plies)
    {
        writer.integer(field, ply.matId);
        writer.real(field + 1, ply.thickness);
        writer.realUnlessDefault(field + 2, ply.thetaInDegrees, 0.0);
        if (ply.stressOutput)
            writer.text(field + 3, "Yes");
        field += FIELDS_PER_PLY;
    }
}

std::uint32_t CompPropTable::append(const CompPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bottomOffset.push(card.bottomOffset);
    m_z0Given.push(card.bottomOffsetGiven ? 1 : 0);
    m_nonStructMass.push(card.nonStructMass);
    m_bondShearAllowable.push(card.bondShearAllowable);
    m_failureTheory.push(static_cast<std::uint8_t>(card.failureTheory));
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_option.push(static_cast<std::uint8_t>(card.option));
    std::vector<base::EntityId> matIds;
    std::vector<double> thicknesses;
    std::vector<double> angles;
    std::vector<std::uint8_t> stressOutputs;
    for (const PlyCard& ply : card.plies)
    {
        matIds.push_back(ply.matId);
        thicknesses.push_back(ply.thickness);
        angles.push_back(ply.thetaInDegrees);
        stressOutputs.push_back(ply.stressOutput ? 1 : 0);
    }
    m_plyMatId.push(matIds);
    m_plyThickness.push(thicknesses);
    m_plyAngle.push(angles);
    m_plyStressOutput.push(stressOutputs);
    return row;
}

CompPropCard CompPropTable::card(std::uint32_t row) const
{
    CompPropCard card;
    card.id = m_id.get(row);
    card.bottomOffset = m_bottomOffset.get(row);
    card.bottomOffsetGiven = m_z0Given.get(row) != 0;
    card.nonStructMass = m_nonStructMass.get(row);
    card.bondShearAllowable = m_bondShearAllowable.get(row);
    card.failureTheory = static_cast<FailureTheory>(m_failureTheory.get(row));
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    card.option = static_cast<LaminateOption>(m_option.get(row));
    const auto matIds = m_plyMatId.get(row);
    const auto thicknesses = m_plyThickness.get(row);
    const auto angles = m_plyAngle.get(row);
    const auto stressOutputs = m_plyStressOutput.get(row);
    for (std::size_t index = 0; index < matIds.size(); ++index)
        card.plies.push_back(PlyCard{matIds[index], thicknesses[index], angles[index], stressOutputs[index] != 0});
    return card;
}

void CompPropTable::store(std::uint32_t row, const CompPropCard& card)
{
    m_id.set(row, card.id);
    m_bottomOffset.set(row, card.bottomOffset);
    m_z0Given.set(row, card.bottomOffsetGiven ? 1 : 0);
    m_nonStructMass.set(row, card.nonStructMass);
    m_bondShearAllowable.set(row, card.bondShearAllowable);
    m_failureTheory.set(row, static_cast<std::uint8_t>(card.failureTheory));
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_option.set(row, static_cast<std::uint8_t>(card.option));
    std::vector<base::EntityId> matIds;
    std::vector<double> thicknesses;
    std::vector<double> angles;
    std::vector<std::uint8_t> stressOutputs;
    for (const PlyCard& ply : card.plies)
    {
        matIds.push_back(ply.matId);
        thicknesses.push_back(ply.thickness);
        angles.push_back(ply.thetaInDegrees);
        stressOutputs.push_back(ply.stressOutput ? 1 : 0);
    }
    m_plyMatId.set(row, matIds);
    m_plyThickness.set(row, thicknesses);
    m_plyAngle.set(row, angles);
    m_plyStressOutput.set(row, stressOutputs);
}

void CompPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bottomOffset.keepRows(rows);
    m_z0Given.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_bondShearAllowable.keepRows(rows);
    m_failureTheory.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_option.keepRows(rows);
    m_plyMatId.keepRows(rows);
    m_plyThickness.keepRows(rows);
    m_plyAngle.keepRows(rows);
    m_plyStressOutput.keepRows(rows);
}

void CompPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> matIds(m_plyMatId.get(row).begin(), m_plyMatId.get(row).end());
    bool changed = false;
    for (base::EntityId& matId : matIds)
    {
        const base::EntityId updated = visitor.visit("plyMatId", ReferenceTarget::Material, matId);
        changed = changed || updated != matId;
        matId = updated;
    }
    if (changed)
        m_plyMatId.set(row, matIds);
}

} // namespace oor::model
