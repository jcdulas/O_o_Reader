#include "oor/model/cards/loads/beam3DistributedLoad/Beam3DistributedLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cctype>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FRAME_FIELD = 3;
constexpr int LAST_FIELD = 11;
constexpr double DEFAULT_SCALE = 1.0;

// The CID field: blank, a word (BASIC, LOCAL, ELEMENT, in any case) or the ID of a coordinate system. The integer 0 is
// the basic system, as the format says, so it is stored as the word BASIC; a negative integer is kept as it is for the
// card's check to refuse.
void readFrame(FieldReader& reader, Beam3DistributedLoadCard& card)
{
    const std::string_view text = reader.text(FRAME_FIELD);
    if (text.empty())
        return;
    if (std::isalpha(static_cast<unsigned char>(text.front())) != 0)
    {
        if (const auto frame = parseKeyword<Beam3DistributedLoadFrame>(BEAM3_LOAD_FRAME_WORDS, text))
            card.frame = *frame;
        else
            reader.error("card.bad-value",
                         std::string("frameId") +
                             " must be BASIC, LOCAL, ELEMENT or the ID of a coordinate system, not \"" +
                             std::string(text) + "\"",
                         "frameId");
        return;
    }
    const base::EntityId frameId = reader.integer(FRAME_FIELD, "frameId", 0);
    if (frameId != 0)
    {
        card.frame = Beam3DistributedLoadFrame::Coordinate;
        card.frameId = frameId;
    }
}

void writeFrame(FieldWriter& writer, const Beam3DistributedLoadCard& card)
{
    switch (card.frame)
    {
    case Beam3DistributedLoadFrame::Basic:
        writer.textUnlessDefault(FRAME_FIELD, BEAM3_LOAD_FRAME_WORDS[0], BEAM3_LOAD_FRAME_WORDS[0]);
        break;
    case Beam3DistributedLoadFrame::Local:
    case Beam3DistributedLoadFrame::Element:
        writer.text(FRAME_FIELD, keywordText(BEAM3_LOAD_FRAME_WORDS, card.frame));
        break;
    case Beam3DistributedLoadFrame::Coordinate:
        writer.integer(FRAME_FIELD, card.frameId);
        break;
    }
}

} // namespace

std::optional<Beam3DistributedLoadCard> Beam3DistributedLoadFormat::read(FieldReader& reader)
{
    Beam3DistributedLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    readFrame(reader, card);
    card.n[0] = reader.real(4, "directionX", 0.0);
    card.n[1] = reader.real(5, "directionY", 0.0);
    card.n[2] = reader.real(6, "directionZ", 0.0);
    card.kind = readRequiredKeyword<Beam3DistributedLoadType>(reader, 7, "kind", BEAM3_LOAD_TYPE_WORDS);
    card.scaleFactor = reader.real(8, "scaleFactor", DEFAULT_SCALE);
    card.p[0] = reader.real(9, "pressureA", 0.0);
    card.p[1] = reader.real(10, "pressureB", 0.0);
    card.p[2] = reader.real(11, "pressureC", 0.0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("pressureC"), "pressureC");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Beam3DistributedLoadFormat::check(const Beam3DistributedLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.elemId, "elemId"))
        return problem;
    if (card.frame == Beam3DistributedLoadFrame::Coordinate)
    {
        if (card.frameId <= 0)
            return CardProblem{
                "card.bad-id",
                std::string(CardProblem::FIELD) + " must be BASIC, LOCAL, ELEMENT or a positive coordinate system ID",
                "frameId"};
    }
    else if (card.frameId != 0)
    {
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " holds a coordinate system only when the frame is a system ID",
            "frameId"};
    }
    if (const auto problem = checkReal(card.n[0], "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.n[1], "directionY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.n[2], "directionZ", RealRule::Finite))
        return problem;
    if (card.n[0] == 0.0 && card.n[1] == 0.0 && card.n[2] == 0.0)
        return CardProblem{"card.bad-value",
                           "at least one of " + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("directionY") + " and " + CardProblem::fieldNamed("directionZ") +
                               " must not be zero",
                           "directionX"};
    if (static_cast<std::size_t>(card.kind) >= BEAM3_LOAD_TYPE_WORDS.size())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be FORCE, MOMENT or BIMOMENT",
                           "kind"};
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.p[0], "pressureA", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.p[1], "pressureB", RealRule::Finite))
        return problem;
    return checkReal(card.p[2], "pressureC", RealRule::Finite);
}

void Beam3DistributedLoadFormat::write(const Beam3DistributedLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    writeFrame(writer, card);
    writer.realUnlessDefault(4, card.n[0], 0.0);
    writer.realUnlessDefault(5, card.n[1], 0.0);
    writer.realUnlessDefault(6, card.n[2], 0.0);
    writeKeyword(writer, 7, BEAM3_LOAD_TYPE_WORDS, card.kind);
    writer.realUnlessDefault(8, card.scaleFactor, DEFAULT_SCALE);
    writer.realUnlessDefault(9, card.p[0], 0.0);
    writer.realUnlessDefault(10, card.p[1], 0.0);
    writer.realUnlessDefault(11, card.p[2], 0.0);
}

std::uint32_t Beam3DistributedLoadTable::append(const Beam3DistributedLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void Beam3DistributedLoadTable::store(std::uint32_t row, const Beam3DistributedLoadCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void Beam3DistributedLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void Beam3DistributedLoadTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void Beam3DistributedLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    Beam3DistributedLoadCard& card = m_cards.edit(row);
    visitId(visitor, "elemId", ReferenceTarget::Element, card.elemId);
    if (card.frame == Beam3DistributedLoadFrame::Coordinate)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
