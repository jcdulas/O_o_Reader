#include "oor/model/cards/loads/cyclicHarmonicLoad/CyclicHarmonicLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 6> HARMONIC_TYPE_WORDS{"Cosine",        "Sine",        "CyclicStar",
                                                              "SymmetricStar", "GravityLoad", "RotationalForceLoad"};

bool generatesOwnHarmonics(HarmonicType type)
{
    return type == HarmonicType::GravityLoad || type == HarmonicType::RotationalForceLoad;
}

template <std::size_t N>
std::string_view wordOf(const std::array<std::string_view, N>& words, HarmonicType type)
{
    if (type == HarmonicType::All)
        return {};
    return words[static_cast<std::size_t>(type) - 1];
}

template <std::size_t N>
std::optional<HarmonicType> typeOfWord(const std::array<std::string_view, N>& words, std::string_view word)
{
    if (word.empty())
        return HarmonicType::All;
    for (std::size_t index = 0; index < N; ++index)
    {
        if ((word == words[index]))
            return static_cast<HarmonicType>(index + 1);
    }
    return std::nullopt;
}

} // namespace

std::optional<CyclicHarmonicLoadCard> CyclicHarmonicLoadFormat::read(FieldReader& reader)
{
    CyclicHarmonicLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scaleFactor = reader.requiredReal(2, "scaleFactor");
    card.harmonicId = reader.integer(3, "harmonicId", CyclicHarmonicLoadCard::NO_HARMONIC);
    if (const auto type = typeOfWord(HARMONIC_TYPE_WORDS, reader.text(4)))
        card.harmonicType = *type;
    else
        reader.error("card.bad-value",
                     std::string("harmonicType") + " must be blank or " + keywordList(HARMONIC_TYPE_WORDS),
                     "harmonicType");
    card.scaleFactor1 = reader.requiredReal(5, "scaleFactor1");
    card.loadSetId1 = reader.requiredInteger(6, "loadSetId1");
    // The second pair is there as soon as one of its fields is; both are then required.
    if (!reader.isBlank(7) || !reader.isBlank(8))
    {
        card.scaleFactor2 = reader.requiredReal(7, "scaleFactor2");
        card.loadSetId2 = reader.requiredInteger(8, "loadSetId2");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicHarmonicLoadFormat::check(const CyclicHarmonicLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.harmonicId < CyclicHarmonicLoadCard::NO_HARMONIC)
        return CardProblem{"card.bad-value", "the harmonic cannot be negative", "harmonicId"};
    if (card.loadSetId1 <= 0)
        return CardProblem{"card.bad-id",
                           "the load set ID " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "loadSetId1"};
    if (card.loadSetId2 < 0)
        return CardProblem{"card.bad-id", "the load set ID " + std::string(CardProblem::FIELD) + " cannot be negative",
                           "loadSetId2"};
    if (card.loadSetId2 == 0 && card.scaleFactor2 != 0.0)
        return CardProblem{
            "card.missing-field",
            CardProblem::fieldNamed("scaleFactor2") + " needs the load set " + std::string(CardProblem::FIELD),
            "loadSetId2"};
    // with HTYPE GRAV or RFORCE the second pair is blank.
    if (generatesOwnHarmonics(card.harmonicType) && card.hasSecond())
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("loadSetId2") +
                               " must be blank when " + CardProblem::fieldNamed("harmonicType") + " is " +
                               CardProblem::cardNamed("GravityLoad") + " or " +
                               CardProblem::cardNamed("RotationalForceLoad"),
                           "scaleFactor2"};
    // S is not zero, and S1 and S2 are not both zero.
    if (card.scaleFactor == 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "scaleFactor"};
    if (card.scaleFactor1 == 0.0 && card.scaleFactor2 == 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("scaleFactor2") +
                               " must not both be zero",
                           "scaleFactor1"};
    return std::nullopt;
}

void CyclicHarmonicLoadFormat::write(const CyclicHarmonicLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scaleFactor);
    if (card.harmonicId != CyclicHarmonicLoadCard::NO_HARMONIC)
        writer.integer(3, card.harmonicId);
    if (card.harmonicType != HarmonicType::All)
    {
        writer.text(4, wordOf(HARMONIC_TYPE_WORDS, card.harmonicType));
    }
    writer.real(5, card.scaleFactor1);
    writer.integer(6, card.loadSetId1);
    if (card.hasSecond())
    {
        writer.real(7, card.scaleFactor2);
        writer.integer(8, card.loadSetId2);
    }
}

std::uint32_t CyclicHarmonicLoadTable::append(const CyclicHarmonicLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_scaleFactor.push(card.scaleFactor);
    m_harmonicId.push(card.harmonicId);
    m_harmonicType.push(static_cast<std::uint8_t>(card.harmonicType));
    m_scaleFactor1.push(card.scaleFactor1);
    m_loadSetId1.push(card.loadSetId1);
    m_scaleFactor2.push(card.scaleFactor2);
    m_loadSetId2.push(card.loadSetId2);
    return row;
}

CyclicHarmonicLoadCard CyclicHarmonicLoadTable::card(std::uint32_t row) const
{
    CyclicHarmonicLoadCard card;
    card.id = m_id.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    card.harmonicId = m_harmonicId.get(row);
    card.harmonicType = static_cast<HarmonicType>(m_harmonicType.get(row));
    card.scaleFactor1 = m_scaleFactor1.get(row);
    card.loadSetId1 = m_loadSetId1.get(row);
    card.scaleFactor2 = m_scaleFactor2.get(row);
    card.loadSetId2 = m_loadSetId2.get(row);
    return card;
}

void CyclicHarmonicLoadTable::store(std::uint32_t row, const CyclicHarmonicLoadCard& card)
{
    m_id.set(row, card.id);
    m_scaleFactor.set(row, card.scaleFactor);
    m_harmonicId.set(row, card.harmonicId);
    m_harmonicType.set(row, static_cast<std::uint8_t>(card.harmonicType));
    m_scaleFactor1.set(row, card.scaleFactor1);
    m_loadSetId1.set(row, card.loadSetId1);
    m_scaleFactor2.set(row, card.scaleFactor2);
    m_loadSetId2.set(row, card.loadSetId2);
}

void CyclicHarmonicLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_harmonicId.keepRows(rows);
    m_harmonicType.keepRows(rows);
    m_scaleFactor1.keepRows(rows);
    m_loadSetId1.keepRows(rows);
    m_scaleFactor2.keepRows(rows);
    m_loadSetId2.keepRows(rows);
}

void CyclicHarmonicLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // L1 and L2 name any static or dynamic loading entry, GRAV and RFORCE included.
    m_loadSetId1.set(row, visitor.visit("loadSetId1", ReferenceTarget::ExcitationSet, m_loadSetId1.get(row)));
    if (const base::EntityId loadSetId2 = m_loadSetId2.get(row); loadSetId2 > 0)
        m_loadSetId2.set(row, visitor.visit("loadSetId2", ReferenceTarget::ExcitationSet, loadSetId2));
}

} // namespace oor::model
