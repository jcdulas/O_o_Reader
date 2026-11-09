#include "oor/model/cards/properties/compPropExplicit/CompPropExplicitTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_FORMULATION = 2;
constexpr int FIELD_SHEAR_FACTOR = 3;
constexpr int FIELD_REFERENCE_SURFACE = 4;
constexpr int FIELD_STRESS_DEFINITION = 5;
constexpr int FIELD_SKIP_FIBER_TIME_STEP = 6;
constexpr int FIELD_STRAIN_OUTPUT = 7;
constexpr int FIELD_LAMINATION_THEORY = 8;
constexpr int FIELD_SPIN_CORRECTION = 9; // the first field of the continuation line

} // namespace

std::optional<CompPropExplicitCard> CompPropExplicitFormat::read(FieldReader& reader)
{
    CompPropExplicitCard card;
    card.id = reader.requiredInteger(1, "id");
    card.formulation = std::string(reader.text(FIELD_FORMULATION));
    card.shearFactor = reader.real(FIELD_SHEAR_FACTOR, "shearFactor", CompPropExplicitCard::DEFAULT_SHEAR_FACTOR);
    card.referenceSurface = readKeyword(reader, FIELD_REFERENCE_SURFACE, "referenceSurface",
                                        COMP_PROP_EXPLICIT_REFERENCE_WORDS, CompPropExplicitReference::Middle);
    card.stressDefinition =
        readKeyword(reader, FIELD_STRESS_DEFINITION, "stressDefinition", COMP_PROP_EXPLICIT_STRESS_DEFINITION_WORDS,
                    CompPropExplicitStressDefinition::Fiber);
    card.skipFiberTimeStep = readKeyword(reader, FIELD_SKIP_FIBER_TIME_STEP, "skipFiberTimeStep",
                                         COMP_PROP_EXPLICIT_YES_NO_WORDS, CompPropExplicitYesNo::No);
    card.strainOutput = readKeyword(reader, FIELD_STRAIN_OUTPUT, "strainOutput", COMP_PROP_EXPLICIT_YES_NO_WORDS,
                                    CompPropExplicitYesNo::Yes);
    card.laminationTheory = reader.integer(FIELD_LAMINATION_THEORY, "laminationTheory", 0);
    card.spinCorrection = readKeyword(reader, FIELD_SPIN_CORRECTION, "spinCorrection", COMP_PROP_EXPLICIT_YES_NO_WORDS,
                                      CompPropExplicitYesNo::No);
    if (reader.lastUsedField() > FIELD_SPIN_CORRECTION)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 9 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CompPropExplicitFormat::check(const CompPropExplicitCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!card.formulation.empty())
    {
        if (const auto problem = checkWordOrName(card.formulation, "formulation"))
            return problem;
    }
    if (const auto problem = checkReal(card.shearFactor, "shearFactor", RealRule::NonNegative))
        return problem;
    if (card.laminationTheory != 0 && card.laminationTheory != 1)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 (integration) or 1 (classical lamination theory)",
            "laminationTheory"};
    return std::nullopt;
}

void CompPropExplicitFormat::write(const CompPropExplicitCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (!card.formulation.empty())
        writer.text(FIELD_FORMULATION, card.formulation);
    writer.realUnlessDefault(FIELD_SHEAR_FACTOR, card.shearFactor, CompPropExplicitCard::DEFAULT_SHEAR_FACTOR);
    writeKeywordUnlessDefault(writer, FIELD_REFERENCE_SURFACE, COMP_PROP_EXPLICIT_REFERENCE_WORDS,
                              card.referenceSurface, CompPropExplicitReference::Middle);
    writeKeywordUnlessDefault(writer, FIELD_STRESS_DEFINITION, COMP_PROP_EXPLICIT_STRESS_DEFINITION_WORDS,
                              card.stressDefinition, CompPropExplicitStressDefinition::Fiber);
    writeKeywordUnlessDefault(writer, FIELD_SKIP_FIBER_TIME_STEP, COMP_PROP_EXPLICIT_YES_NO_WORDS,
                              card.skipFiberTimeStep, CompPropExplicitYesNo::No);
    writeKeywordUnlessDefault(writer, FIELD_STRAIN_OUTPUT, COMP_PROP_EXPLICIT_YES_NO_WORDS, card.strainOutput,
                              CompPropExplicitYesNo::Yes);
    writer.integerUnlessDefault(FIELD_LAMINATION_THEORY, card.laminationTheory, 0);
    writeKeywordUnlessDefault(writer, FIELD_SPIN_CORRECTION, COMP_PROP_EXPLICIT_YES_NO_WORDS, card.spinCorrection,
                              CompPropExplicitYesNo::No);
}

std::uint32_t CompPropExplicitTable::append(const CompPropExplicitCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CompPropExplicitTable::store(std::uint32_t row, const CompPropExplicitCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CompPropExplicitTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CompPropExplicitTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CompPropExplicitTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PCOMP that the card extends; the column follows the value the visitor returns.
    CompPropExplicitCard& card = m_cards.edit(row);
    visitId(visitor, "id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
