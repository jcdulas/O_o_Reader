#include "oor/model/cards/thermal/radiationCavity/RadiationCavityTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int MAX_SET_PAIRS = 30;

} // namespace

RadiationCavityTable::RadiationCavityTable()
    : m_shadowing("Yes"), m_viewFactorOutput(NO_VIEW_FACTOR_OUTPUT), m_subdivisionAreaRatio(0.1), m_componentCount(32)
{
}

std::optional<RadiationCavityCard> RadiationCavityFormat::read(FieldReader& reader)
{
    RadiationCavityCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ambientElemId = reader.integer(2, "ambientElemId", 0);
    card.shadowing = readKeyword(reader, 3, "shadowing", {"Yes", "No"}, "Yes");
    card.scaleFactor = reader.real(4, "scaleFactor", 0.0);
    card.viewFactorOutput = reader.optionalInteger(5, "viewFactorOutput");
    card.viewFactorMethod = readKeyword(reader, 6, "viewFactorMethod", {"FiniteDifferenceMethod", "Contour"}, "");
    card.subdivisionAreaRatio = reader.real(7, "subdivisionAreaRatio", 0.1);
    card.componentCount = reader.integer(8, "componentCount", 32);
    // The SET pairs run from field 9, over as many lines as the card has.
    const int last = reader.lastUsedField();
    for (int field = 9; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        card.sets.push_back(reader.requiredInteger(field, "pairSetId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationCavityFormat::check(const RadiationCavityCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the cavity ID must be a positive integer", "id"};
    if (card.ambientElemId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive element ID (or blank)",
                           "ambientElemId"};
    if (const auto problem = checkKeyword(card.shadowing, "shadowing", {"Yes", "No"}))
        return problem;
    if (!(card.scaleFactor >= 0.0 && card.scaleFactor <= 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.0 and 1.0",
                           "scaleFactor"};
    if (card.viewFactorOutput && (*card.viewFactorOutput < 0 || *card.viewFactorOutput > 5))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2, 3, 4 or 5 (or blank)",
                           "viewFactorOutput"};
    if (!card.viewFactorMethod.empty())
    {
        if (const auto problem =
                checkKeyword(card.viewFactorMethod, "viewFactorMethod", {"FiniteDifferenceMethod", "Contour"}))
            return problem;
    }
    if (!(card.subdivisionAreaRatio >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "subdivisionAreaRatio"};
    if (card.componentCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1", "componentCount"};
    if (card.sets.size() % 2 != 0)
        return CardProblem{"card.bad-value",
                           std::string("the ") + std::string(CardProblem::FIELD) + " IDs come in pairs (" +
                               std::string(CardProblem::FIELD) + "i1, " + std::string(CardProblem::FIELD) + "i2)",
                           "pairSetId"};
    if (card.sets.size() > 2 * MAX_SET_PAIRS)
        return CardProblem{"card.bad-value",
                           std::string("the card has at most 30 pairs of ") + std::string(CardProblem::FIELD) + " IDs",
                           "pairSetId"};
    for (const base::EntityId set : card.sets)
    {
        if (set <= 0)
            return CardProblem{"card.bad-id",
                               std::string("a ") + std::string(CardProblem::FIELD) + " ID must be a positive integer",
                               "pairSetId"};
    }
    return std::nullopt;
}

void RadiationCavityFormat::write(const RadiationCavityCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.ambientElemId, 0);
    writer.textUnlessDefault(3, card.shadowing, "Yes");
    writer.realUnlessDefault(4, card.scaleFactor, 0.0);
    if (card.viewFactorOutput)
        writer.integer(5, *card.viewFactorOutput);
    if (!card.viewFactorMethod.empty())
        writer.text(6, card.viewFactorMethod);
    writer.realUnlessDefault(7, card.subdivisionAreaRatio, 0.1);
    writer.integerUnlessDefault(8, card.componentCount, 32);
    for (std::size_t index = 0; index < card.sets.size(); ++index)
        writer.integer(9 + static_cast<int>(index), card.sets[index]);
}

std::uint32_t RadiationCavityTable::append(const RadiationCavityCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ambientElemId.push(card.ambientElemId);
    m_shadowing.push(card.shadowing);
    m_scaleFactor.push(card.scaleFactor);
    m_viewFactorOutput.push(card.viewFactorOutput.value_or(NO_VIEW_FACTOR_OUTPUT));
    m_viewFactorMethod.push(card.viewFactorMethod);
    m_subdivisionAreaRatio.push(card.subdivisionAreaRatio);
    m_componentCount.push(card.componentCount);
    m_sets.push(card.sets);
    return row;
}

RadiationCavityCard RadiationCavityTable::card(std::uint32_t row) const
{
    RadiationCavityCard card;
    card.id = m_id.get(row);
    card.ambientElemId = m_ambientElemId.get(row);
    card.shadowing = m_shadowing.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    if (const std::int32_t viewFactorOutput = m_viewFactorOutput.get(row); viewFactorOutput != NO_VIEW_FACTOR_OUTPUT)
        card.viewFactorOutput = viewFactorOutput;
    card.viewFactorMethod = m_viewFactorMethod.get(row);
    card.subdivisionAreaRatio = m_subdivisionAreaRatio.get(row);
    card.componentCount = m_componentCount.get(row);
    const auto sets = m_sets.get(row);
    card.sets.assign(sets.begin(), sets.end());
    return card;
}

void RadiationCavityTable::store(std::uint32_t row, const RadiationCavityCard& card)
{
    m_id.set(row, card.id);
    m_ambientElemId.set(row, card.ambientElemId);
    m_shadowing.set(row, card.shadowing);
    m_scaleFactor.set(row, card.scaleFactor);
    m_viewFactorOutput.set(row, card.viewFactorOutput.value_or(NO_VIEW_FACTOR_OUTPUT));
    m_viewFactorMethod.set(row, card.viewFactorMethod);
    m_subdivisionAreaRatio.set(row, card.subdivisionAreaRatio);
    m_componentCount.set(row, card.componentCount);
    m_sets.set(row, card.sets);
}

void RadiationCavityTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ambientElemId.keepRows(rows);
    m_shadowing.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_viewFactorOutput.keepRows(rows);
    m_viewFactorMethod.keepRows(rows);
    m_subdivisionAreaRatio.keepRows(rows);
    m_componentCount.keepRows(rows);
    m_sets.keepRows(rows);
}

void RadiationCavityTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId ambientElemId = m_ambientElemId.get(row); ambientElemId > 0)
        m_ambientElemId.set(row, visitor.visit("ambientElemId", ReferenceTarget::SurfaceElement, ambientElemId));
    const auto current = m_sets.get(row);
    std::vector<base::EntityId> sets(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& set : sets)
    {
        if (set <= 0)
            continue;
        const base::EntityId updated = visitor.visit("pairSetId", ReferenceTarget::Set, set);
        changed = changed || updated != set;
        set = updated;
    }
    if (changed)
        m_sets.set(row, sets);
}

} // namespace oor::model
