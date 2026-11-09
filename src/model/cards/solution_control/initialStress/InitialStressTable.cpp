#include "oor/model/cards/solution_control/initialStress/InitialStressTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// STRESS1 and STRESS2 end the first line (fields 7 and 8); STRESS3 to STRESS7 start the second (fields 9 to 13). The
// COORD word and its CID are the first two fields of the third line.
constexpr int FIRST_STRESS_FIELD = 7;
constexpr int SECOND_LINE_FIRST_STRESS_FIELD = 9;
constexpr int COORDINATE_FIELD = 17;
constexpr int FRAME_ID_FIELD = 18;
constexpr std::string_view COORD_WORD = "Coordinates";

constexpr std::array<const char*, INITIAL_STRESS_COMPONENTS> STRESS_NAMES{"stress1", "stress2", "stress3", "stress4",
                                                                          "stress5", "stress6", "stress7"};

// The field of the component number `index` (0 to 6).
constexpr int stressField(std::size_t index) noexcept
{
    return index < 2 ? FIRST_STRESS_FIELD + static_cast<int>(index)
                     : SECOND_LINE_FIRST_STRESS_FIELD + static_cast<int>(index) - 2;
}

} // namespace

std::optional<InitialStressCard> InitialStressFormat::read(FieldReader& reader)
{
    InitialStressCard card;
    card.range = readInitialStateRange(reader);
    for (std::size_t index = 0; index < INITIAL_STRESS_COMPONENTS; ++index)
        card.stress[index] = reader.real(stressField(index), STRESS_NAMES[index], 0.0);
    if (!reader.isBlank(COORDINATE_FIELD))
    {
        if (!(reader.text(COORDINATE_FIELD) == COORD_WORD))
            reader.error("card.bad-value",
                         "field " + std::to_string(COORDINATE_FIELD) + " of an " + std::string(reader.cardName()) +
                             " must be " + std::string("coordinate"),
                         "coordinate");
        card.frameId = reader.optionalInteger(FRAME_ID_FIELD, "frameId");
    }
    else if (!reader.isBlank(FRAME_ID_FIELD))
    {
        reader.error("card.bad-value",
                     std::string("frameId") + " needs the word " + std::string("coordinate") + " in field " +
                         std::to_string(COORDINATE_FIELD),
                     "coordinate");
    }
    // The cells the format draws blank: fields 14 to 16, 19 to 24 and the end of the card.
    requireUnusedBlank(reader, {14, 15, 16, 19, 20, 21, 22, 23, 24}, 24);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialStressFormat::check(const InitialStressCard& card)
{
    if (const auto problem = checkInitialStateRange(card.range))
        return problem;
    for (std::size_t index = 0; index < INITIAL_STRESS_COMPONENTS; ++index)
    {
        if (const auto problem = checkReal(card.stress[index], STRESS_NAMES[index], RealRule::Finite))
            return problem;
    }
    // The format gives the two systems and no number for a coordinate system card: -1 is the element system and 0 the
    // basic one.
    if (card.frameId && *card.frameId != -1 && *card.frameId != 0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be -1 (the element system) or 0 (the basic system)",
                           "frameId"};
    return std::nullopt;
}

void InitialStressFormat::write(const InitialStressCard& card, FieldWriter& writer)
{
    writeInitialStateRange(writer, card.range);
    // A blank component is 0.0, so a zero is left blank.
    for (std::size_t index = 0; index < INITIAL_STRESS_COMPONENTS; ++index)
        writer.realUnlessDefault(stressField(index), card.stress[index], 0.0);
    if (card.frameId)
    {
        writer.text(COORDINATE_FIELD, COORD_WORD);
        writer.integer(FRAME_ID_FIELD, *card.frameId);
    }
}

std::uint32_t InitialStressTable::append(const InitialStressCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void InitialStressTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitInitialStateRange(visitor, m_cards.edit(row).range);
}

} // namespace oor::model
