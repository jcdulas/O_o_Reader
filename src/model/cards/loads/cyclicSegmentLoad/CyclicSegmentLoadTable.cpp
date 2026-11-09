#include "oor/model/cards/loads/cyclicSegmentLoad/CyclicSegmentLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <string>

namespace oor::model
{

std::optional<CyclicSegmentLoadCard> CyclicSegmentLoadFormat::read(FieldReader& reader)
{
    CyclicSegmentLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scaleFactor = reader.requiredReal(2, "scaleFactor");
    card.segmentId = reader.requiredInteger(3, "segmentId");
    if (const std::string_view type = reader.text(4); !type.empty())
    {
        if ((type == "Right"))
            card.segmentType = SegmentType::Right;
        else if ((type == "Left"))
            card.segmentType = SegmentType::Left;
        else
            reader.error("card.bad-value",
                         std::string("segmentType") + " must be R, L or blank, found \"" + std::string(type) + "\"",
                         "segmentType");
    }
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

std::optional<CardProblem> CyclicSegmentLoadFormat::check(const CyclicSegmentLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
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

void CyclicSegmentLoadFormat::write(const CyclicSegmentLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scaleFactor);
    writer.integer(3, card.segmentId);
    if (card.segmentType != SegmentType::Both)
        writer.text(4, card.segmentType == SegmentType::Right ? "Right" : "Left");
    writer.real(5, card.scaleFactor1);
    writer.integer(6, card.loadSetId1);
    if (card.hasSecond())
    {
        writer.real(7, card.scaleFactor2);
        writer.integer(8, card.loadSetId2);
    }
}

std::uint32_t CyclicSegmentLoadTable::append(const CyclicSegmentLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_scaleFactor.push(card.scaleFactor);
    m_segmentId.push(card.segmentId);
    m_segmentType.push(static_cast<std::uint8_t>(card.segmentType));
    m_scaleFactor1.push(card.scaleFactor1);
    m_loadSetId1.push(card.loadSetId1);
    m_scaleFactor2.push(card.scaleFactor2);
    m_loadSetId2.push(card.loadSetId2);
    return row;
}

CyclicSegmentLoadCard CyclicSegmentLoadTable::card(std::uint32_t row) const
{
    CyclicSegmentLoadCard card;
    card.id = m_id.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    card.segmentId = m_segmentId.get(row);
    card.segmentType = static_cast<SegmentType>(m_segmentType.get(row));
    card.scaleFactor1 = m_scaleFactor1.get(row);
    card.loadSetId1 = m_loadSetId1.get(row);
    card.scaleFactor2 = m_scaleFactor2.get(row);
    card.loadSetId2 = m_loadSetId2.get(row);
    return card;
}

void CyclicSegmentLoadTable::store(std::uint32_t row, const CyclicSegmentLoadCard& card)
{
    m_id.set(row, card.id);
    m_scaleFactor.set(row, card.scaleFactor);
    m_segmentId.set(row, card.segmentId);
    m_segmentType.set(row, static_cast<std::uint8_t>(card.segmentType));
    m_scaleFactor1.set(row, card.scaleFactor1);
    m_loadSetId1.set(row, card.loadSetId1);
    m_scaleFactor2.set(row, card.scaleFactor2);
    m_loadSetId2.set(row, card.loadSetId2);
}

void CyclicSegmentLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_segmentId.keepRows(rows);
    m_segmentType.keepRows(rows);
    m_scaleFactor1.keepRows(rows);
    m_loadSetId1.keepRows(rows);
    m_scaleFactor2.keepRows(rows);
    m_loadSetId2.keepRows(rows);
}

void CyclicSegmentLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // L1 and L2 name any static or dynamic loading entry (a set of the Load or DynamicLoad category).
    m_loadSetId1.set(row, visitor.visit("loadSetId1", ReferenceTarget::ExcitationSet, m_loadSetId1.get(row)));
    if (const base::EntityId loadSetId2 = m_loadSetId2.get(row); loadSetId2 > 0)
        m_loadSetId2.set(row, visitor.visit("loadSetId2", ReferenceTarget::ExcitationSet, loadSetId2));
}

} // namespace oor::model
