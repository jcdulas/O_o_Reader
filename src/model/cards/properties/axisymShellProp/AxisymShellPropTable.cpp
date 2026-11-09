#include "oor/model/cards/properties/axisymShellProp/AxisymShellPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BLANK_FIELD = 3; // the cell between MID and T1 in the grid of the format
constexpr int THICKNESS1_FIELD = 4;
constexpr int THICKNESS2_FIELD = 5;
constexpr int ANALYSIS_TYPE_FIELD = 6;
constexpr int LAST_FIELD = ANALYSIS_TYPE_FIELD;

} // namespace

std::optional<AxisymShellPropCard> AxisymShellPropFormat::read(FieldReader& reader)
{
    AxisymShellPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.thickness1 = reader.requiredReal(THICKNESS1_FIELD, "thickness1");
    card.thickness2 = reader.real(THICKNESS2_FIELD, "thickness2", 0.0);
    card.analysisType = readKeyword(reader, ANALYSIS_TYPE_FIELD, "analysisType", AXISYM_SHELL_PROP_ANALYSIS_WORDS,
                                    AxisymShellPropAnalysis::StructuralHeat);
    requireBlankFields(reader, {BLANK_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymShellPropFormat::check(const AxisymShellPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.thickness1, "thickness1", RealRule::Positive))
        return problem;
    // The format allows 0.0 for T2 (a constant thickness), not a negative thickness.
    return checkReal(card.thickness2, "thickness2", RealRule::NonNegative);
}

void AxisymShellPropFormat::write(const AxisymShellPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(THICKNESS1_FIELD, card.thickness1);
    writer.realUnlessDefault(THICKNESS2_FIELD, card.thickness2, 0.0);
    writeKeywordUnlessDefault(writer, ANALYSIS_TYPE_FIELD, AXISYM_SHELL_PROP_ANALYSIS_WORDS, card.analysisType,
                              AxisymShellPropAnalysis::StructuralHeat);
}

std::uint32_t AxisymShellPropTable::append(const AxisymShellPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_thickness1.push(card.thickness1);
    m_thickness2.push(card.thickness2);
    m_analysisType.push(static_cast<std::uint8_t>(card.analysisType));
    return row;
}

AxisymShellPropCard AxisymShellPropTable::card(std::uint32_t row) const
{
    AxisymShellPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.thickness1 = m_thickness1.get(row);
    card.thickness2 = m_thickness2.get(row);
    card.analysisType = static_cast<AxisymShellPropAnalysis>(m_analysisType.get(row));
    return card;
}

void AxisymShellPropTable::store(std::uint32_t row, const AxisymShellPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_thickness1.set(row, card.thickness1);
    m_thickness2.set(row, card.thickness2);
    m_analysisType.set(row, static_cast<std::uint8_t>(card.analysisType));
}

void AxisymShellPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_thickness1.keepRows(rows);
    m_thickness2.keepRows(rows);
    m_analysisType.keepRows(rows);
}

void AxisymShellPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The material of the element: a structural one, or a MAT4 or MAT5 for heat transfer (the Material target looks
    // for the thermal materials when no structural one has the ID).
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
