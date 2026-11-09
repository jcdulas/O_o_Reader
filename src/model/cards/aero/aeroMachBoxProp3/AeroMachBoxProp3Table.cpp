#include "oor/model/cards/aero/aeroMachBoxProp3/AeroMachBoxProp3Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <array>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 5;
constexpr int LAST_FIELD = FIRST_POINT_FIELD + 2 * static_cast<int>(AeroMachBoxProp3Card::POINT_COUNT) - 1;
constexpr int BLANK_FIELD = 4;

// Index of the first point of the first control surface (point 7) and of the points a number of surfaces needs: four
// for one surface (7 to 10), six for two (7 to 12).
constexpr std::size_t FIRST_CONTROL_POINT = 2;
constexpr std::array<std::size_t, AeroMachBoxProp3Card::MAX_CONTROL_COUNT + 1> CONTROL_POINT_COUNT{0, 4, 6};

// A CardProblem keeps a pointer to the name of its field, so the names are literals and not built in a loop.
constexpr std::array<const char*, AeroMachBoxProp3Card::POINT_COUNT> X_NAMES{
    "pointX5", "pointX6", "pointX7", "pointX8", "pointX9", "pointX10", "pointX11", "pointX12"};
constexpr std::array<const char*, AeroMachBoxProp3Card::POINT_COUNT> Y_NAMES{
    "pointY5", "pointY6", "pointY7", "pointY8", "pointY9", "pointY10", "pointY11", "pointY12"};

// The field of the x of point `index` (the point number minus 5); its y is the next field.
int xFieldOf(std::size_t index) noexcept
{
    return FIRST_POINT_FIELD + 2 * static_cast<int>(index);
}

// The first problem with a pair of points of a control surface: the Y of `inner` is not below the Y of
// `outer`.
std::optional<CardProblem> checkEdgeOrder(const AeroMachBoxProp3Card& card, std::size_t inner, std::size_t outer)
{
    const std::optional<double>& innerY = card.points[inner].y;
    const std::optional<double>& outerY = card.points[outer].y;
    if (innerY && outerY && *innerY < *outerY)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed(Y_NAMES[inner]) + " cannot be smaller than {field}", Y_NAMES[outer]};
    return std::nullopt;
}

} // namespace

std::optional<AeroMachBoxProp3Card> AeroMachBoxProp3Format::read(FieldReader& reader)
{
    AeroMachBoxProp3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.boxCount = reader.requiredInteger(2, "boxCount");
    card.controlSurfaceCount = reader.requiredInteger(3, "controlSurfaceCount");
    for (std::size_t index = 0; index < AeroMachBoxProp3Card::POINT_COUNT; ++index)
    {
        card.points[index].x = reader.optionalReal(xFieldOf(index), X_NAMES[index]);
        card.points[index].y = reader.optionalReal(xFieldOf(index) + 1, Y_NAMES[index]);
    }
    // Field 4 is a blank cell of the layout, and the card ends with Y12.
    requireBlankFields(reader, {BLANK_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroMachBoxProp3Format::check(const AeroMachBoxProp3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.boxCount < 1 || card.boxCount > AeroMachBoxProp3Card::MAX_BOX_COUNT)
        return CardProblem{"card.bad-value", "{field} must be an integer from 1 to 49", "boxCount"};
    if (card.controlSurfaceCount < 0 || card.controlSurfaceCount > AeroMachBoxProp3Card::MAX_CONTROL_COUNT)
        return CardProblem{"card.bad-value", "{field} must be 0, 1 or 2", "controlSurfaceCount"};
    for (std::size_t index = 0; index < AeroMachBoxProp3Card::POINT_COUNT; ++index)
    {
        if (const auto problem = checkOptionalReal(card.points[index].x, X_NAMES[index], RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.points[index].y, Y_NAMES[index], RealRule::Finite))
            return problem;
    }
    // The continuation lines of the control surfaces are required, with both coordinates of every point they need.
    const std::size_t needed =
        FIRST_CONTROL_POINT + CONTROL_POINT_COUNT[static_cast<std::size_t>(card.controlSurfaceCount)];
    for (std::size_t index = FIRST_CONTROL_POINT; index < needed; ++index)
    {
        const std::string because =
            "{field} is required for {field:controlSurfaceCount} = " + std::to_string(card.controlSurfaceCount);
        if (!card.points[index].x)
            return CardProblem{"card.missing-field", because, X_NAMES[index]};
        if (!card.points[index].y)
            return CardProblem{"card.missing-field", because, Y_NAMES[index]};
    }
    // The pairs of points 7 and 8, 9 and 10, 11 and 12.
    for (std::size_t inner = FIRST_CONTROL_POINT; inner + 1 < AeroMachBoxProp3Card::POINT_COUNT; inner += 2)
    {
        if (const auto problem = checkEdgeOrder(card, inner, inner + 1))
            return problem;
    }
    return std::nullopt;
}

void AeroMachBoxProp3Format::write(const AeroMachBoxProp3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.boxCount);
    writer.integer(3, card.controlSurfaceCount);
    for (std::size_t index = 0; index < AeroMachBoxProp3Card::POINT_COUNT; ++index)
    {
        if (card.points[index].x)
            writer.real(xFieldOf(index), *card.points[index].x);
        if (card.points[index].y)
            writer.real(xFieldOf(index) + 1, *card.points[index].y);
    }
}

std::uint32_t AeroMachBoxProp3Table::append(const AeroMachBoxProp3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroMachBoxProp3Table::store(std::uint32_t row, const AeroMachBoxProp3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroMachBoxProp3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroMachBoxProp3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroMachBoxProp3Table::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The points are coordinates and the numbers are counts: a PAERO3 names no other card.
}

} // namespace oor::model
