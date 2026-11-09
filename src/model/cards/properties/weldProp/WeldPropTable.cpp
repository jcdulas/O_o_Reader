#include "oor/model/cards/properties/weldProp/WeldPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cctype>

namespace oor::model
{

namespace
{

// A word of the fields 5 and 6 of the older PWELD layout: letters only (FF, CLAMP, ...), never a number.
bool isOlderLayoutWord(std::string_view text) noexcept
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](char character)
                                        { return std::isalpha(static_cast<unsigned char>(character)) != 0; });
}

} // namespace

std::optional<WeldPropCard> WeldPropFormat::read(FieldReader& reader)
{
    WeldPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.diameter = reader.requiredReal(3, "diameter");
    // An older layout of PWELD had connection words in fields 5 and 6 (for example `FF` and `CLAMP`), which the
    // current layout leaves blank; a solver ignores them. A word there is accepted and dropped (written back blank,
    // in the current layout); a number there is still an error.
    for (const int field : {4, 5})
    {
        if (!reader.isBlank(field) && !isOlderLayoutWord(reader.text(field)))
            reader.error("card.bad-value",
                         "fields 5 and 6 of a " + std::string(reader.cardName()) +
                             " are blank (or a word of the older layout), found \"" + std::string(reader.text(field)) +
                             "\"",
                         "diameter");
    }
    if (!reader.isBlank(7))
        reader.error("card.bad-value",
                     "field 8 of a " + std::string(reader.cardName()) + " is blank, found \"" +
                         std::string(reader.text(7)) + "\"",
                     "eliminateConstraints");
    if (const std::string_view mset = reader.text(6); (mset == "On"))
        card.eliminateConstraints = true;
    else if (!mset.empty() && !(mset == "Off"))
        reader.error(
            "card.bad-value",
            std::string("eliminateConstraints") + " must be ON, OFF or blank, found \"" + std::string(mset) + "\"",
            "eliminateConstraints");
    if (const std::string_view type = reader.text(8); (type == "SpotWeld"))
        card.spot = true;
    else if (!type.empty())
        reader.error("card.bad-value",
                     std::string("kind") + " must be SPOT or blank, found \"" + std::string(type) + "\"", "kind");
    card.minLengthRatio = reader.real(9, "minLengthRatio", WeldPropCard::DEFAULT_MIN_LENGTH_RATIO);
    card.maxLengthRatio = reader.real(10, "maxLengthRatio", WeldPropCard::DEFAULT_MAX_LENGTH_RATIO);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has ten fields", "maxLengthRatio");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WeldPropFormat::check(const WeldPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (!(card.diameter > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "diameter"};
    if (!(card.minLengthRatio > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "minLengthRatio"};
    if (!(card.maxLengthRatio > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "maxLengthRatio"};
    return std::nullopt;
}

void WeldPropFormat::write(const WeldPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.diameter);
    if (card.eliminateConstraints)
        writer.text(6, "On");
    if (card.spot)
        writer.text(8, "SpotWeld");
    writer.realUnlessDefault(9, card.minLengthRatio, WeldPropCard::DEFAULT_MIN_LENGTH_RATIO);
    writer.realUnlessDefault(10, card.maxLengthRatio, WeldPropCard::DEFAULT_MAX_LENGTH_RATIO);
}

std::uint32_t WeldPropTable::append(const WeldPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_diameter.push(card.diameter);
    m_eliminateConstraints.push(card.eliminateConstraints ? 1 : 0);
    m_spot.push(card.spot ? 1 : 0);
    m_minLengthRatio.push(card.minLengthRatio);
    m_maxLengthRatio.push(card.maxLengthRatio);
    return row;
}

WeldPropCard WeldPropTable::card(std::uint32_t row) const
{
    WeldPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.diameter = m_diameter.get(row);
    card.eliminateConstraints = m_eliminateConstraints.get(row) != 0;
    card.spot = m_spot.get(row) != 0;
    card.minLengthRatio = m_minLengthRatio.get(row);
    card.maxLengthRatio = m_maxLengthRatio.get(row);
    return card;
}

void WeldPropTable::store(std::uint32_t row, const WeldPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_diameter.set(row, card.diameter);
    m_eliminateConstraints.set(row, card.eliminateConstraints ? 1 : 0);
    m_spot.set(row, card.spot ? 1 : 0);
    m_minLengthRatio.set(row, card.minLengthRatio);
    m_maxLengthRatio.set(row, card.maxLengthRatio);
}

void WeldPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_diameter.keepRows(rows);
    m_eliminateConstraints.keepRows(rows);
    m_spot.keepRows(rows);
    m_minLengthRatio.keepRows(rows);
    m_maxLengthRatio.keepRows(rows);
}

void WeldPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
