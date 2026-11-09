#include "oor/model/cards/solution_control/arcLengthParams/ArcLengthParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

const char* typeWord(ArcLengthType type)
{
    switch (type)
    {
    case ArcLengthType::Cris:
        return "Crisfield";
    case ArcLengthType::Riks:
        return "Riks";
    case ArcLengthType::Mriks:
        return "ModifiedRiks";
    }
    return "Crisfield";
}

} // namespace

std::optional<ArcLengthParamsCard> ArcLengthParamsFormat::read(FieldReader& reader)
{
    ArcLengthParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view typeText = reader.text(2);
    if (typeText.empty() || (typeText == "Crisfield"))
        card.type = ArcLengthType::Cris;
    else if ((typeText == "Riks"))
        card.type = ArcLengthType::Riks;
    else if ((typeText == "ModifiedRiks"))
        card.type = ArcLengthType::Mriks;
    else
        reader.error(
            "card.bad-value",
            std::string("constraintKind") + " must be CRIS, RIKS or MRIKS, found \"" + std::string(typeText) + "\"",
            "constraintKind");
    card.minArcLengthRatio = reader.real(3, "minArcLengthRatio", 0.25);
    card.maxArcLengthRatio = reader.real(4, "maxArcLengthRatio", 4.0);
    card.scaleFactor = reader.real(5, "scaleFactor", 0.0);
    if (!reader.isBlank(6))
        reader.error("card.bad-value", "field 6 of an " + std::string(reader.cardName()) + " is not used",
                     "scaleFactor");
    card.designIteration = reader.integer(7, "designIteration", 12);
    card.maxIncrements = reader.integer(8, "maxIncrements", 20);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has no field beyond " + std::string("maxIncrements"),
                     "maxIncrements");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ArcLengthParamsFormat::check(const ArcLengthParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "id"};
    if (!(card.minArcLengthRatio > 0.0 && card.minArcLengthRatio <= 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 and at most 1.0",
                           "minArcLengthRatio"};
    if (!(card.maxArcLengthRatio >= 1.0) || !std::isfinite(card.maxArcLengthRatio))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1.0",
                           "maxArcLengthRatio"};
    if (!(card.scaleFactor >= 0.0) || !std::isfinite(card.scaleFactor))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "scaleFactor"};
    if (card.designIteration < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "designIteration"};
    if (card.maxIncrements < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "maxIncrements"};
    return std::nullopt;
}

void ArcLengthParamsFormat::write(const ArcLengthParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, typeWord(card.type), "Crisfield");
    writer.realUnlessDefault(3, card.minArcLengthRatio, 0.25);
    writer.realUnlessDefault(4, card.maxArcLengthRatio, 4.0);
    writer.realUnlessDefault(5, card.scaleFactor, 0.0);
    writer.integerUnlessDefault(7, card.designIteration, 12);
    writer.integerUnlessDefault(8, card.maxIncrements, 20);
}

std::uint32_t ArcLengthParamsTable::append(const ArcLengthParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(static_cast<std::uint8_t>(card.type));
    m_minArcLengthRatio.push(card.minArcLengthRatio);
    m_maxArcLengthRatio.push(card.maxArcLengthRatio);
    m_scaleFactor.push(card.scaleFactor);
    m_designIteration.push(card.designIteration);
    m_maxIncrements.push(card.maxIncrements);
    return row;
}

ArcLengthParamsCard ArcLengthParamsTable::card(std::uint32_t row) const
{
    ArcLengthParamsCard card;
    card.id = m_id.get(row);
    card.type = static_cast<ArcLengthType>(m_type.get(row));
    card.minArcLengthRatio = m_minArcLengthRatio.get(row);
    card.maxArcLengthRatio = m_maxArcLengthRatio.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    card.designIteration = m_designIteration.get(row);
    card.maxIncrements = m_maxIncrements.get(row);
    return card;
}

void ArcLengthParamsTable::store(std::uint32_t row, const ArcLengthParamsCard& card)
{
    m_id.set(row, card.id);
    m_type.set(row, static_cast<std::uint8_t>(card.type));
    m_minArcLengthRatio.set(row, card.minArcLengthRatio);
    m_maxArcLengthRatio.set(row, card.maxArcLengthRatio);
    m_scaleFactor.set(row, card.scaleFactor);
    m_designIteration.set(row, card.designIteration);
    m_maxIncrements.set(row, card.maxIncrements);
}

void ArcLengthParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_minArcLengthRatio.keepRows(rows);
    m_maxArcLengthRatio.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_designIteration.keepRows(rows);
    m_maxIncrements.keepRows(rows);
}

} // namespace oor::model
