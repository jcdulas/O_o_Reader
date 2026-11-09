#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>

// Where an initial state of SOL 400 applies: the first six fields of IPSTRAIN (initial plastic strain) and ISTRESS
// (initial stress) are the same range of elements, integration points and layers. One kind of value used by two cards
// of the family; each card still reads, writes and checks the fields after the range by hand and says what its values
// are.
namespace oor::model
{

// The first and the last integration point when the fields are blank (the format reference: INT1 = 1, INTN = 4).
inline constexpr std::int32_t INITIAL_STATE_FIRST_POINT = 1;
inline constexpr std::int32_t INITIAL_STATE_LAST_POINT = 4;

// format reference fields 1 to 6: EID1  EID2  INT1  INTN  LAY1  LAYN. The elements are the range from EID1 to EID2 (the
// elements of the range that exist: a gap is skipped, as in every THRU range), the integration points INT1 to INTN of
// each, and the layers LAY1 to LAYN of a beam or shell (zero or blank when the model has none).
struct InitialStateRange
{
    base::EntityId firstElemId = 0; // the first element, required
    base::EntityId lastElemId =
        0; // the last element: stored resolved (EID1 when the field is blank), so it is always written
    std::int32_t firstPoint = INITIAL_STATE_FIRST_POINT; // the first integration point
    std::int32_t lastPoint = INITIAL_STATE_LAST_POINT;   // the last integration point
    std::int32_t firstLayer = 0;                         // the first layer, 0 when blank
    std::int32_t lastLayer = 0;                          // the last layer, 0 when blank

    [[nodiscard]] bool operator==(const InitialStateRange&) const noexcept = default;
};

// Reads fields 1 to 6. A zero EID2 is a blank one (a last element of 0 would be below the first).
[[nodiscard]] inline InitialStateRange readInitialStateRange(FieldReader& reader)
{
    InitialStateRange range;
    range.firstElemId = reader.requiredInteger(1, "firstElemId");
    range.lastElemId = reader.integer(2, "lastElemId", range.firstElemId);
    if (range.lastElemId == 0)
        range.lastElemId = range.firstElemId;
    range.firstPoint = reader.integer(3, "firstPoint", INITIAL_STATE_FIRST_POINT);
    range.lastPoint = reader.integer(4, "lastPoint", INITIAL_STATE_LAST_POINT);
    range.firstLayer = reader.integer(5, "firstLayer", 0);
    range.lastLayer = reader.integer(6, "lastLayer", 0);
    return range;
}

// Writes fields 1 to 6: EID2 always (it holds a default that depends on EID1), the others unless they are blank.
inline void writeInitialStateRange(FieldWriter& writer, const InitialStateRange& range)
{
    writer.integer(1, range.firstElemId);
    writer.integer(2, range.lastElemId);
    writer.integerUnlessDefault(3, range.firstPoint, INITIAL_STATE_FIRST_POINT);
    writer.integerUnlessDefault(4, range.lastPoint, INITIAL_STATE_LAST_POINT);
    writer.integerUnlessDefault(5, range.firstLayer, 0);
    writer.integerUnlessDefault(6, range.lastLayer, 0);
}

// The first thing wrong with a range, if anything: the elements, the integration points and the layers each go from the
// first to the last, and the integration points are positive.
[[nodiscard]] inline std::optional<CardProblem> checkInitialStateRange(const InitialStateRange& range)
{
    if (const auto problem = checkRequiredId(range.firstElemId, "firstElemId"))
        return problem;
    if (range.lastElemId < range.firstElemId)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("firstElemId"),
            "lastElemId"};
    if (range.firstPoint < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "firstPoint"};
    if (range.lastPoint < range.firstPoint)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("firstPoint"),
            "lastPoint"};
    if (range.firstLayer < 0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a positive integer, or zero when blank",
                           "firstLayer"};
    if (range.lastLayer < 0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a positive integer, or zero when blank",
                           "lastLayer"};
    if (range.firstLayer != 0 && range.lastLayer != 0 && range.lastLayer < range.firstLayer)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("firstLayer"),
            "lastLayer"};
    return std::nullopt;
}

// The elements of a range are the two ends of a THRU list: each is an element the model may lack.
inline void visitInitialStateRange(ReferenceVisitor& visitor, InitialStateRange& range)
{
    visitId(visitor, "firstElemId", ReferenceTarget::ElementInRange, range.firstElemId);
    visitId(visitor, "lastElemId", ReferenceTarget::ElementInRange, range.lastElemId);
}

} // namespace oor::model
