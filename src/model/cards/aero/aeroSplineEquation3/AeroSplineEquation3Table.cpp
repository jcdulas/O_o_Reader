#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int USAGE_FIELD = 8;
constexpr int FIRST_POINT_FIELD = 5;
constexpr int POINT_STRIDE = 4;
constexpr std::int32_t LAST_COMPONENT = 6;

// The first field of the point `index`: 5, 9, 13, ... The field after its three (8, 12, 16, ...) is USAGE for the first
// point and unused for the others.
int pointField(std::size_t index) noexcept
{
    return FIRST_POINT_FIELD + POINT_STRIDE * static_cast<int>(index);
}

// The names of the fields of a point in the diagnostics: the format reference numbers the first group (G1, C1, A1) and
// then says
// "-etc."; a CardProblem keeps the name as a pointer, so these are literals and the later groups share one name.
const char* nodeName(std::size_t index) noexcept
{
    return index == 0 ? "node1" : "nodes";
}

const char* componentName(std::size_t index) noexcept
{
    return index == 0 ? "components1" : "components";
}

const char* coefficientName(std::size_t index) noexcept
{
    return index == 0 ? "coefficient1" : "coefficients";
}

// The padding of a line: none of the three fields of a group has a value.
bool isBlankPoint(const FieldReader& reader, int field)
{
    return reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2);
}

} // namespace

std::optional<AeroSplineEquation3Card> AeroSplineEquation3Format::read(FieldReader& reader)
{
    AeroSplineEquation3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.boxId = reader.requiredInteger(3, "boxId");
    card.component = reader.requiredInteger(4, "component");
    card.usage = readKeyword<SplineUsage>(reader, USAGE_FIELD, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; pointField(index) <= last; ++index)
    {
        const int field = pointField(index);
        if (index > 0 && isBlankPoint(reader, field))
            continue;
        AeroSplineEquation3Point point;
        point.node = reader.requiredInteger(field, nodeName(index));
        point.component = reader.requiredInteger(field + 1, componentName(index));
        point.coefficient = reader.requiredReal(field + 2, coefficientName(index));
        card.points.push_back(point);
    }
    // The fourth field of every group after the first is a blank cell of the layout.
    for (int field = USAGE_FIELD + POINT_STRIDE; field <= last; field += POINT_STRIDE)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSplineEquation3Format::check(const AeroSplineEquation3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroPanelId, "aeroPanelId"))
        return problem;
    if (const auto problem = checkRequiredId(card.boxId, "boxId"))
        return problem;
    if (card.component < 1 || card.component > LAST_COMPONENT)
        return CardProblem{"card.bad-value", "{field} must be an integer from 1 to 6", "component"};
    if (card.points.empty())
        return CardProblem{"card.missing-field",
                           "at least one independent point ({field}, {field:components1}, {field:coefficient1}) is "
                           "required",
                           "node1"};
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const AeroSplineEquation3Point& point = card.points[index];
        if (const auto problem = checkRequiredId(point.node, nodeName(index)))
            return problem;
        if (point.component < 0 || point.component > LAST_COMPONENT)
            return CardProblem{"card.bad-value", "{field} must be 0 for a scalar point or a component from 1 to 6",
                               componentName(index)};
        if (!std::isfinite(point.coefficient))
            return CardProblem{"card.bad-value", "{field} must be a finite number", coefficientName(index)};
    }
    return std::nullopt;
}

void AeroSplineEquation3Format::write(const AeroSplineEquation3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.integer(3, card.boxId);
    writer.integer(4, card.component);
    writeKeywordUnlessDefault(writer, USAGE_FIELD, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const int field = pointField(index);
        const AeroSplineEquation3Point& point = card.points[index];
        writer.integer(field, point.node);
        writer.integer(field + 1, point.component); // a scalar point is written as 0, never left blank
        writer.real(field + 2, point.coefficient);
    }
}

std::uint32_t AeroSplineEquation3Table::append(const AeroSplineEquation3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroSplineEquation3Table::store(std::uint32_t row, const AeroSplineEquation3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroSplineEquation3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroSplineEquation3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroSplineEquation3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroSplineEquation3Card& card = m_cards.edit(row);
    visitId(visitor, "aeroPanelId", ReferenceTarget::AeroElement, card.aeroPanelId);
    visitId(visitor, "boxId", ReferenceTarget::AeroBox, card.boxId);
    // The same name for every point: a visitor may keep the view of the name, so no name is built here.
    for (AeroSplineEquation3Point& point : card.points)
        visitId(visitor, "nodes", ReferenceTarget::ScalarOrGridPoint, point.node);
}

} // namespace oor::model
