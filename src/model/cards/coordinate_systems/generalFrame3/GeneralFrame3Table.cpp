#include "oor/model/cards/coordinate_systems/generalFrame3/GeneralFrame3Table.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace oor::model
{

namespace
{

// The rotations of a method: Euler angles or space-fixed rotations, then the sequence of the three axes as digits
// (EulerAngles313, SpaceFixed321).
constexpr std::array<std::string_view, 2> METHOD_PREFIXES{"EulerAngles", "SpaceFixed"};
constexpr std::size_t SEQUENCE_DIGITS = 3;

bool isMethodName(std::string_view methodName)
{
    for (const std::string_view prefix : METHOD_PREFIXES)
    {
        if (methodName.size() != prefix.size() + SEQUENCE_DIGITS || !methodName.starts_with(prefix))
            continue;
        const std::string_view digits = methodName.substr(prefix.size());
        return std::all_of(digits.begin(), digits.end(), [](char digit) { return digit >= '0' && digit <= '9'; });
    }
    return false;
}

} // namespace

std::optional<GeneralFrame3Card> GeneralFrame3Format::read(FieldReader& reader)
{
    GeneralFrame3Card card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view methodName = reader.text(2); !methodName.empty())
        card.methodName = std::string(methodName);
    if (const std::string_view matrixForm = reader.text(3); !matrixForm.empty())
        card.matrixForm = std::string(matrixForm);
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"angleFunctionId1", "angleFunctionId2", "angleFunctionId3"};
        card.angleFunctionIds[static_cast<std::size_t>(index)] = reader.requiredInteger(4 + index, NAMES[index]);
    }
    card.referenceFrameId = reader.integer(7, "referenceFrameId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeneralFrame3Format::check(const GeneralFrame3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the coordinate system ID must be a positive integer", "id"};
    if (!isMethodName(card.methodName))
        return CardProblem{
            "card.bad-value",
            "{field} must be EulerAngles or SpaceFixed followed by three digits (EulerAngles313, SpaceFixed321)",
            "methodName"};
    if (card.matrixForm != "EquationForm" && card.matrixForm != "Table")
        return CardProblem{"card.bad-value", "{field} must be EquationForm or Table", "matrixForm"};
    for (const base::EntityId id : card.angleFunctionIds)
    {
        if (id <= 0)
            return CardProblem{"card.bad-id",
                               CardProblem::fieldNamed("angleFunctionId1") + ", " +
                                   CardProblem::fieldNamed("angleFunctionId2") + " and " +
                                   CardProblem::fieldNamed("angleFunctionId3") + " must be positive integers",
                               "angleFunctionId1"};
    }
    if (card.referenceFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "referenceFrameId"};
    if (card.referenceFrameId == card.id)
        return CardProblem{"card.bad-value", "a coordinate system cannot be defined with respect to itself",
                           "referenceFrameId"};
    return std::nullopt;
}

void GeneralFrame3Format::write(const GeneralFrame3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.methodName, GeneralFrame3Card::DEFAULT_METHOD_NAME);
    writer.textUnlessDefault(3, card.matrixForm, GeneralFrame3Card::DEFAULT_MATRIX_FORM);
    for (int index = 0; index < 3; ++index)
        writer.integer(4 + index, card.angleFunctionIds[static_cast<std::size_t>(index)]);
    writer.integerUnlessDefault(7, card.referenceFrameId, 0);
}

std::uint32_t GeneralFrame3Table::append(const GeneralFrame3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_methodName.push(card.methodName);
    m_matrixForm.push(card.matrixForm);
    m_angleFunctionIds.push(card.angleFunctionIds);
    m_referenceFrameId.push(card.referenceFrameId);
    return row;
}

GeneralFrame3Card GeneralFrame3Table::card(std::uint32_t row) const
{
    return GeneralFrame3Card{m_id.get(row), m_methodName.get(row), m_matrixForm.get(row), m_angleFunctionIds.get(row),
                             m_referenceFrameId.get(row)};
}

void GeneralFrame3Table::store(std::uint32_t row, const GeneralFrame3Card& card)
{
    m_id.set(row, card.id);
    m_methodName.set(row, card.methodName);
    m_matrixForm.set(row, card.matrixForm);
    m_angleFunctionIds.set(row, card.angleFunctionIds);
    m_referenceFrameId.set(row, card.referenceFrameId);
}

void GeneralFrame3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_methodName.keepRows(rows);
    m_matrixForm.keepRows(rows);
    m_angleFunctionIds.keepRows(rows);
    m_referenceFrameId.keepRows(rows);
}

void GeneralFrame3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId referenceFrameId = m_referenceFrameId.get(row); referenceFrameId > 0)
        m_referenceFrameId.set(row, visitor.visit("referenceFrameId", ReferenceTarget::Coordinate, referenceFrameId));
    // THETAIDi name a DEQATN (FORM EQN) or a TABLE3D (FORM TABLE). A TABLE3D is one of the cards of the category
    // MultiDimensionalTable, which holds the TABL3Di cards too: the target does not tell the two apart, and a THETAID
    // that names a TABL3Di is a check of the model, not of the card.
    const ReferenceTarget target = m_matrixForm.get(row) == GeneralFrame3Card::DEFAULT_MATRIX_FORM
                                       ? ReferenceTarget::DesignEquation
                                       : ReferenceTarget::MultiDimensionalTable;
    std::array<base::EntityId, 3> ids = m_angleFunctionIds.get(row);
    for (std::size_t index = 0; index < ids.size(); ++index)
    {
        if (ids[index] > 0)
            ids[index] = visitor.visit("angleFunctionId" + std::to_string(index + 1), target, ids[index]);
    }
    m_angleFunctionIds.set(row, ids);
}

} // namespace oor::model
