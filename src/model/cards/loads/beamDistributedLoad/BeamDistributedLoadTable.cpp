#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/BlankReal.hpp"

#include <string>

namespace oor::model
{

namespace
{

struct NamedType
{
    std::string_view name;
    BarLoadType type;
};
constexpr NamedType TYPES[] = {{"ForceX", BarLoadType::FX},
                               {"ForceY", BarLoadType::FY},
                               {"ForceZ", BarLoadType::FZ},
                               {"ForceXAlongLength", BarLoadType::FXE},
                               {"ForceYAlongLength", BarLoadType::FYE},
                               {"ForceZAlongLength", BarLoadType::FZE},
                               {"MomentX", BarLoadType::MX},
                               {"MomentY", BarLoadType::MY},
                               {"MomentZ", BarLoadType::MZ},
                               {"MomentXAlongLength", BarLoadType::MXE},
                               {"MomentYAlongLength", BarLoadType::MYE},
                               {"MomentZAlongLength", BarLoadType::MZE}};

struct NamedScale
{
    std::string_view name;
    BarLoadScale scale;
};
constexpr NamedScale SCALES[] = {{"ActualLength", BarLoadScale::LE},
                                 {"FractionalLength", BarLoadScale::FR},
                                 {"ProjectedLength", BarLoadScale::LEPR},
                                 {"FractionalProjectedLength", BarLoadScale::FRPR}};

} // namespace

std::string_view BeamDistributedLoadFormat::typeName(BarLoadType type) noexcept
{
    for (const NamedType& entry : TYPES)
    {
        if (entry.type == type)
            return entry.name;
    }
    return "ForceX";
}

std::optional<BarLoadType> BeamDistributedLoadFormat::typeFromName(std::string_view name) noexcept
{
    for (const NamedType& entry : TYPES)
    {
        if ((name == entry.name))
            return entry.type;
    }
    return std::nullopt;
}

std::string_view BeamDistributedLoadFormat::scaleName(BarLoadScale scale) noexcept
{
    for (const NamedScale& entry : SCALES)
    {
        if (entry.scale == scale)
            return entry.name;
    }
    return "ActualLength";
}

std::optional<BarLoadScale> BeamDistributedLoadFormat::scaleFromName(std::string_view name) noexcept
{
    for (const NamedScale& entry : SCALES)
    {
        if ((name == entry.name))
            return entry.scale;
    }
    return std::nullopt;
}

std::optional<BeamDistributedLoadCard> BeamDistributedLoadFormat::read(FieldReader& reader)
{
    BeamDistributedLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    if (const auto type = typeFromName(reader.text(3)))
        card.kind = *type;
    else
        reader.error("card.bad-value",
                     std::string("kind") + " must be FX, FY, FZ, FXE, FYE, FZE, MX, MY, MZ, MXE, MYE or MZE", "kind");
    if (const auto scale = scaleFromName(reader.text(4)))
        card.scaleFactor = *scale;
    else
        reader.error("card.bad-value", std::string("scaleFactor") + " must be LE, FR, LEPR or FRPR", "scaleFactor");
    card.xCoord1 = reader.requiredReal(5, "xCoord1");
    card.pressure1 = reader.requiredReal(6, "pressure1");
    // a blank X2 makes a concentrated load. A blank P2 stays blank (BeamDistributedLoadCard: the solver reads
    // it as 0 for a distributed load).
    card.xCoord2 = reader.real(7, "xCoord2", card.xCoord1);
    card.pressure2 = reader.optionalReal(8, "pressure2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamDistributedLoadFormat::check(const BeamDistributedLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.elemId <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "elemId"};
    if (card.xCoord1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "xCoord1"};
    if (card.xCoord2 < card.xCoord1)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " cannot be below " + CardProblem::fieldNamed("xCoord1"),
                           "xCoord2"};
    // with SCALE = FR (and FRPR) the positions are fractions of the length.
    if (card.isFractional() && card.xCoord2 > 1.0)
        return CardProblem{
            "card.bad-value",
            "a fractional position (" + CardProblem::fieldNamed("scaleFactor") + " FR or FRPR) cannot be above 1.0",
            "xCoord2"};
    return std::nullopt;
}

void BeamDistributedLoadFormat::write(const BeamDistributedLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    writer.text(3, typeName(card.kind));
    writer.text(4, scaleName(card.scaleFactor));
    writer.real(5, card.xCoord1);
    writer.real(6, card.pressure1);
    // X2 defaults to X1: always written. A blank P2 is written blank.
    writer.real(7, card.xCoord2);
    if (card.pressure2)
        writer.real(8, *card.pressure2);
}

std::uint32_t BeamDistributedLoadTable::append(const BeamDistributedLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_elemId.push(card.elemId);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_scaleFactor.push(static_cast<std::uint8_t>(card.scaleFactor));
    m_xCoord1.push(card.xCoord1);
    m_pressure1.push(card.pressure1);
    m_xCoord2.push(card.xCoord2);
    m_pressure2.push(realToColumn(card.pressure2));
    return row;
}

BeamDistributedLoadCard BeamDistributedLoadTable::card(std::uint32_t row) const
{
    BeamDistributedLoadCard card;
    card.id = m_id.get(row);
    card.elemId = m_elemId.get(row);
    card.kind = static_cast<BarLoadType>(m_kind.get(row));
    card.scaleFactor = static_cast<BarLoadScale>(m_scaleFactor.get(row));
    card.xCoord1 = m_xCoord1.get(row);
    card.pressure1 = m_pressure1.get(row);
    card.xCoord2 = m_xCoord2.get(row);
    card.pressure2 = realFromColumn(m_pressure2.get(row));
    return card;
}

void BeamDistributedLoadTable::store(std::uint32_t row, const BeamDistributedLoadCard& card)
{
    m_id.set(row, card.id);
    m_elemId.set(row, card.elemId);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_scaleFactor.set(row, static_cast<std::uint8_t>(card.scaleFactor));
    m_xCoord1.set(row, card.xCoord1);
    m_pressure1.set(row, card.pressure1);
    m_xCoord2.set(row, card.xCoord2);
    m_pressure2.set(row, realToColumn(card.pressure2));
}

void BeamDistributedLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_elemId.keepRows(rows);
    m_kind.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_xCoord1.keepRows(rows);
    m_pressure1.keepRows(rows);
    m_xCoord2.keepRows(rows);
    m_pressure2.keepRows(rows);
}

void BeamDistributedLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::Element, m_elemId.get(row)));
}

} // namespace oor::model
