#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_AREA_FIELD = 17;
constexpr int FIELDS_PER_AREA = 8; // one lumped area per line: Y Z C MID and four blank fields

} // namespace

std::optional<BeamLumpedAreaPropCard> BeamLumpedAreaPropFormat::read(FieldReader& reader)
{
    BeamLumpedAreaPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.area = reader.requiredReal(3, "area");
    card.bendingInertia1 = reader.real(4, "bendingInertia1", 0.0);
    card.bendingInertia2 = reader.real(5, "bendingInertia2", 0.0);
    card.productInertia12 = reader.real(6, "productInertia12", 0.0);
    card.torsionConstant = reader.real(7, "torsionConstant", 0.0);
    card.nonStructMass = reader.real(8, "nonStructMass", 0.0);
    card.shearFactor1 = reader.real(9, "shearFactor1", 1.0);
    card.shearFactor2 = reader.real(10, "shearFactor2", 1.0);
    card.nsmCenterY = reader.real(11, "nsmCenterY", 0.0);
    card.nsmCenterZ = reader.real(12, "nsmCenterZ", 0.0);
    card.neutralAxisY = reader.real(13, "neutralAxisY", 0.0);
    card.neutralAxisZ = reader.real(14, "neutralAxisZ", 0.0);
    card.symmetricOption = reader.integer(15, "symmetricOption", 0);
    if (!reader.isBlank(16))
        reader.error("card.bad-value", "field 16 of a " + std::string(reader.cardName()) + " is blank",
                     "symmetricOption");

    // The lumped areas, one per line from field 17; a line with nothing in it is padding.
    for (int field = FIRST_AREA_FIELD; field <= reader.lastUsedField(); field += FIELDS_PER_AREA)
    {
        bool blank = true;
        for (int offset = 0; offset < FIELDS_PER_AREA; ++offset)
            blank = blank && reader.isBlank(field + offset);
        if (blank)
            continue;
        BeamLumpedAreaPropArea area;
        area.y = reader.requiredReal(field, "areaY");
        area.z = reader.requiredReal(field + 1, "areaZ");
        area.fraction = reader.real(field + 2, "areaFraction", 0.0);
        area.matId = reader.integer(field + 3, "areaMatId", card.matId); // a blank MIDi is MID
        for (int offset = 4; offset < FIELDS_PER_AREA; ++offset)
        {
            if (!reader.isBlank(field + offset))
            {
                reader.error(
                    "card.bad-value",
                    "fields 5 to 8 of a lumped area line of a " + std::string(reader.cardName()) + " must be blank",
                    "areaMatId");
                break;
            }
        }
        card.areas.push_back(area);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamLumpedAreaPropFormat::check(const BeamLumpedAreaPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (!(card.area > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "area"};
    if (card.bendingInertia1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "bendingInertia1"};
    if (card.bendingInertia2 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "bendingInertia2"};
    if (card.torsionConstant < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "torsionConstant"};
    if (card.shearFactor1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor1"};
    if (card.shearFactor2 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor2"};
    if (card.symmetricOption < 0 || card.symmetricOption > 5)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 to 5", "symmetricOption"};
    if (card.areas.size() > BeamLumpedAreaPropCard::MAX_AREAS)
        return CardProblem{"card.bad-value", "the card has at most 20 lumped areas", "areaY"};
    for (const BeamLumpedAreaPropArea& area : card.areas)
    {
        if (area.fraction < 0.0)
            return CardProblem{"card.bad-value",
                               "the fraction of area " + std::string(CardProblem::FIELD) + " must not be negative",
                               "areaFraction"};
        if (area.matId <= 0)
            return CardProblem{"card.bad-id", "a lumped area needs a material ID", "areaMatId"};
    }
    return std::nullopt;
}

void BeamLumpedAreaPropFormat::write(const BeamLumpedAreaPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.area);
    writer.realUnlessDefault(4, card.bendingInertia1, 0.0);
    writer.realUnlessDefault(5, card.bendingInertia2, 0.0);
    writer.realUnlessDefault(6, card.productInertia12, 0.0);
    writer.realUnlessDefault(7, card.torsionConstant, 0.0);
    writer.realUnlessDefault(8, card.nonStructMass, 0.0);
    writer.realUnlessDefault(9, card.shearFactor1, 1.0);
    writer.realUnlessDefault(10, card.shearFactor2, 1.0);
    writer.realUnlessDefault(11, card.nsmCenterY, 0.0);
    writer.realUnlessDefault(12, card.nsmCenterZ, 0.0);
    writer.realUnlessDefault(13, card.neutralAxisY, 0.0);
    writer.realUnlessDefault(14, card.neutralAxisZ, 0.0);
    writer.integerUnlessDefault(15, card.symmetricOption, 0);
    int field = FIRST_AREA_FIELD;
    for (const BeamLumpedAreaPropArea& area : card.areas)
    {
        writer.real(field, area.y);
        writer.real(field + 1, area.z);
        writer.realUnlessDefault(field + 2, area.fraction, 0.0);
        writer.integer(field + 3, area.matId); // a blank MIDi is MID: always written
        field += FIELDS_PER_AREA;
    }
}

namespace
{

// The four lists of a card's lumped areas, as the table stores them.
struct AreaLists
{
    std::vector<double> y;
    std::vector<double> z;
    std::vector<double> c;
    std::vector<base::EntityId> matIds;
};

AreaLists listsOf(const std::vector<BeamLumpedAreaPropArea>& areas)
{
    AreaLists lists;
    for (const BeamLumpedAreaPropArea& area : areas)
    {
        lists.y.push_back(area.y);
        lists.z.push_back(area.z);
        lists.c.push_back(area.fraction);
        lists.matIds.push_back(area.matId);
    }
    return lists;
}

} // namespace

std::uint32_t BeamLumpedAreaPropTable::append(const BeamLumpedAreaPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_section.push({card.area, card.bendingInertia1, card.bendingInertia2, card.productInertia12, card.torsionConstant,
                    card.nonStructMass});
    m_shearFactor.push({card.shearFactor1, card.shearFactor2});
    m_nsmCenter.push({card.nsmCenterY, card.nsmCenterZ});
    m_neutralAxis.push({card.neutralAxisY, card.neutralAxisZ});
    m_symmetricOption.push(static_cast<std::uint8_t>(card.symmetricOption));
    const AreaLists lists = listsOf(card.areas);
    m_areaY.push(lists.y);
    m_areaZ.push(lists.z);
    m_areaFraction.push(lists.c);
    m_areaMatId.push(lists.matIds);
    return row;
}

BeamLumpedAreaPropCard BeamLumpedAreaPropTable::card(std::uint32_t row) const
{
    BeamLumpedAreaPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    const std::array<double, 6> section = m_section.get(row);
    card.area = section[0];
    card.bendingInertia1 = section[1];
    card.bendingInertia2 = section[2];
    card.productInertia12 = section[3];
    card.torsionConstant = section[4];
    card.nonStructMass = section[5];
    const std::array<double, 2> shear = m_shearFactor.get(row);
    card.shearFactor1 = shear[0];
    card.shearFactor2 = shear[1];
    const std::array<double, 2> mass = m_nsmCenter.get(row);
    card.nsmCenterY = mass[0];
    card.nsmCenterZ = mass[1];
    const std::array<double, 2> neutral = m_neutralAxis.get(row);
    card.neutralAxisY = neutral[0];
    card.neutralAxisZ = neutral[1];
    card.symmetricOption = m_symmetricOption.get(row);
    const auto y = m_areaY.get(row);
    const auto z = m_areaZ.get(row);
    const auto c = m_areaFraction.get(row);
    const auto matIds = m_areaMatId.get(row);
    for (std::size_t index = 0; index < y.size(); ++index)
        card.areas.push_back(BeamLumpedAreaPropArea{y[index], z[index], c[index], matIds[index]});
    return card;
}

void BeamLumpedAreaPropTable::store(std::uint32_t row, const BeamLumpedAreaPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_section.set(row, {card.area, card.bendingInertia1, card.bendingInertia2, card.productInertia12,
                        card.torsionConstant, card.nonStructMass});
    m_shearFactor.set(row, {card.shearFactor1, card.shearFactor2});
    m_nsmCenter.set(row, {card.nsmCenterY, card.nsmCenterZ});
    m_neutralAxis.set(row, {card.neutralAxisY, card.neutralAxisZ});
    m_symmetricOption.set(row, static_cast<std::uint8_t>(card.symmetricOption));
    const AreaLists lists = listsOf(card.areas);
    m_areaY.set(row, lists.y);
    m_areaZ.set(row, lists.z);
    m_areaFraction.set(row, lists.c);
    m_areaMatId.set(row, lists.matIds);
}

void BeamLumpedAreaPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_section.keepRows(rows);
    m_shearFactor.keepRows(rows);
    m_nsmCenter.keepRows(rows);
    m_neutralAxis.keepRows(rows);
    m_symmetricOption.keepRows(rows);
    m_areaY.keepRows(rows);
    m_areaZ.keepRows(rows);
    m_areaFraction.keepRows(rows);
    m_areaMatId.keepRows(rows);
}

void BeamLumpedAreaPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
    const auto current = m_areaMatId.get(row);
    std::vector<base::EntityId> matIds(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& matId : matIds)
    {
        const base::EntityId updated = visitor.visit("areaMatId", ReferenceTarget::Material, matId);
        changed = changed || updated != matId;
        matId = updated;
    }
    if (changed)
        m_areaMatId.set(row, matIds);
}

} // namespace oor::model
