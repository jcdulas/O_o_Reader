#include "oor/model/cards/materials/micromechanicsCompositeMat/MicromechanicsCompositeMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

std::optional<MicromechanicsCompositeMatCard> MicromechanicsCompositeMatFormat::read(FieldReader& reader)
{
    MicromechanicsCompositeMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.userDefinedNameId = reader.requiredInteger(2, "userDefinedNameId");
    card.density = reader.optionalReal(3, "density");
    requireBlankFields(reader, {}, 3);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MicromechanicsCompositeMatFormat::check(const MicromechanicsCompositeMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.userDefinedNameId <= 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be the ID of a user defined name, a positive integer",
            "userDefinedNameId"};
    if (card.density && !(*card.density > 0.0 && std::isfinite(*card.density)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero, or blank",
                           "density"};
    return std::nullopt;
}

void MicromechanicsCompositeMatFormat::write(const MicromechanicsCompositeMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.userDefinedNameId);
    if (card.density)
        writer.real(3, *card.density);
}

std::uint32_t MicromechanicsCompositeMatTable::append(const MicromechanicsCompositeMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_userDefinedNameId.push(card.userDefinedNameId);
    m_density.push(card.density.value_or(notGiven()));
    return row;
}

MicromechanicsCompositeMatCard MicromechanicsCompositeMatTable::card(std::uint32_t row) const
{
    MicromechanicsCompositeMatCard card;
    card.id = m_id.get(row);
    card.userDefinedNameId = m_userDefinedNameId.get(row);
    if (const double density = m_density.get(row); !std::isnan(density))
        card.density = density;
    return card;
}

void MicromechanicsCompositeMatTable::store(std::uint32_t row, const MicromechanicsCompositeMatCard& card)
{
    m_id.set(row, card.id);
    m_userDefinedNameId.set(row, card.userDefinedNameId);
    m_density.set(row, card.density.value_or(notGiven()));
}

void MicromechanicsCompositeMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_userDefinedNameId.keepRows(rows);
    m_density.keepRows(rows);
}

void MicromechanicsCompositeMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The UDID names the UDNAME card of the file.
    if (const base::EntityId userDefinedName = m_userDefinedNameId.get(row); userDefinedName > 0)
        m_userDefinedNameId.set(row,
                                visitor.visit("userDefinedNameId", ReferenceTarget::UserDefinedName, userDefinedName));
}

} // namespace oor::model
