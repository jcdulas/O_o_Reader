#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_FORMULATION = 3;
constexpr int FIELD_QUADRATURE = 4;
constexpr int FIELD_INTEGRATION_POINT_COUNT = 5;
constexpr int FIELD_SHEAR_FACTOR = 6;
constexpr int FIELD_REFERENCE_SURFACE = 7;
constexpr int FIELD_SPIN_CORRECTION = 8;
constexpr int FIELD_THICKNESS1 = 9; // T1 to T4: fields 9 to 12, on the continuation line
constexpr int FIELD_TRANSVERSE_SHEAR = 13;
constexpr int FIELD_SHEAR_LOCKING = 14;

// The FORM field: blank (the formulation then depends on the element), a word of the table of the format reference, or
// the CO-TRIA spelling (a letter O) also seen for the same triangle.
std::optional<ShellPropNodalForm> readForm(FieldReader& reader)
{
    if ((reader.text(FIELD_FORMULATION) == "C0Triangle"))
        return ShellPropNodalForm::C0Tria;
    return readOptionalKeyword<ShellPropNodalForm>(reader, FIELD_FORMULATION, "formulation",
                                                   SHELL_PROP_NODAL_FORM_WORDS);
}

} // namespace

std::optional<ShellPropNodalCard> ShellPropNodalFormat::read(FieldReader& reader)
{
    ShellPropNodalCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.formulation = readForm(reader);
    card.quadrature = readKeyword(reader, FIELD_QUADRATURE, "quadrature", SHELL_PROP_NODAL_QUADRATURE_WORDS,
                                  ShellPropNodalQuadrature::Gauss);
    card.integrationPointCount = reader.integer(FIELD_INTEGRATION_POINT_COUNT, "integrationPointCount",
                                                shellPropNodalDefaultPointCount(card.formulation));
    card.shearFactor = reader.real(FIELD_SHEAR_FACTOR, "shearFactor", ShellPropNodalCard::DEFAULT_SHEAR_FACTOR);
    card.referenceSurface = readKeyword(reader, FIELD_REFERENCE_SURFACE, "referenceSurface",
                                        SHELL_PROP_NODAL_REFERENCE_WORDS, ShellPropNodalReference::Middle);
    card.spinCorrection = readKeyword(reader, FIELD_SPIN_CORRECTION, "spinCorrection",
                                      SHELL_PROP_NODAL_SPIN_CORRECTION_WORDS, ShellPropNodalSpinCorrection::No);
    card.thickness1 = reader.real(FIELD_THICKNESS1, "thickness1", 0.0);
    card.thickness2 = reader.real(FIELD_THICKNESS1 + 1, "thickness2", 0.0);
    card.thickness3 = reader.real(FIELD_THICKNESS1 + 2, "thickness3", 0.0);
    card.thickness4 = reader.real(FIELD_THICKNESS1 + 3, "thickness4", 0.0);
    card.transverseShear = readOptionalKeyword<ShellPropNodalTransverseShear>(
        reader, FIELD_TRANSVERSE_SHEAR, "transverseShear", SHELL_PROP_NODAL_TRANSVERSE_SHEAR_WORDS);
    card.shearLocking = readOptionalKeyword<ShellPropNodalShearLock>(reader, FIELD_SHEAR_LOCKING, "shearLocking",
                                                                     SHELL_PROP_NODAL_SHEAR_LOCK_WORDS);
    if (reader.lastUsedField() > FIELD_SHEAR_LOCKING)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 14 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellPropNodalFormat::check(const ShellPropNodalCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.integrationPointCount < 1 || card.integrationPointCount > ShellPropNodalCard::MAX_INTEGRATION_POINT_COUNT)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be from 1 to 5",
                           "integrationPointCount"};
    if (const auto problem = checkReal(card.shearFactor, "shearFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.thickness1, "thickness1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.thickness2, "thickness2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.thickness3, "thickness3", RealRule::Finite))
        return problem;
    return checkReal(card.thickness4, "thickness4", RealRule::Finite);
}

void ShellPropNodalFormat::write(const ShellPropNodalCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writeOptionalKeyword(writer, FIELD_FORMULATION, SHELL_PROP_NODAL_FORM_WORDS, card.formulation);
    writeKeywordUnlessDefault(writer, FIELD_QUADRATURE, SHELL_PROP_NODAL_QUADRATURE_WORDS, card.quadrature,
                              ShellPropNodalQuadrature::Gauss);
    // The default of the number of points depends on FORM: stored resolved, so always written.
    writer.integer(FIELD_INTEGRATION_POINT_COUNT, card.integrationPointCount);
    writer.realUnlessDefault(FIELD_SHEAR_FACTOR, card.shearFactor, ShellPropNodalCard::DEFAULT_SHEAR_FACTOR);
    writeKeywordUnlessDefault(writer, FIELD_REFERENCE_SURFACE, SHELL_PROP_NODAL_REFERENCE_WORDS, card.referenceSurface,
                              ShellPropNodalReference::Middle);
    writeKeywordUnlessDefault(writer, FIELD_SPIN_CORRECTION, SHELL_PROP_NODAL_SPIN_CORRECTION_WORDS,
                              card.spinCorrection, ShellPropNodalSpinCorrection::No);
    writer.realUnlessDefault(FIELD_THICKNESS1, card.thickness1, 0.0);
    writer.realUnlessDefault(FIELD_THICKNESS1 + 1, card.thickness2, 0.0);
    writer.realUnlessDefault(FIELD_THICKNESS1 + 2, card.thickness3, 0.0);
    writer.realUnlessDefault(FIELD_THICKNESS1 + 3, card.thickness4, 0.0);
    writeOptionalKeyword(writer, FIELD_TRANSVERSE_SHEAR, SHELL_PROP_NODAL_TRANSVERSE_SHEAR_WORDS, card.transverseShear);
    writeOptionalKeyword(writer, FIELD_SHEAR_LOCKING, SHELL_PROP_NODAL_SHEAR_LOCK_WORDS, card.shearLocking);
}

std::uint32_t ShellPropNodalTable::append(const ShellPropNodalCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ShellPropNodalTable::store(std::uint32_t row, const ShellPropNodalCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ShellPropNodalTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ShellPropNodalTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ShellPropNodalTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
