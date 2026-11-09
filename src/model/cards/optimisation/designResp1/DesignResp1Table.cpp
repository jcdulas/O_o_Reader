#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1References.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ATTRIBUTE_FIELD = 8;

std::optional<CardProblem> checkValue(const DesignValue& value, const char* field)
{
    if (const std::string problem = designValueProblem(value); !problem.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + ": " + problem, field};
    return std::nullopt;
}

} // namespace

std::optional<DesignResp1Card> DesignResp1Format::read(FieldReader& reader)
{
    DesignResp1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    card.responseKind = std::string(reader.text(3));
    if (card.responseKind.empty())
        reader.error("card.missing-field", std::string("responseKind") + " is required", "responseKind");
    card.ownerKind = readDesignValue(reader, 4, "ownerKind");
    card.regionId = reader.integer(5, "regionId", 0);
    card.attributeA = readDesignValue(reader, 6, "attributeA");
    card.attributeB = readDesignValue(reader, 7, "attributeB");
    for (int field = FIRST_ATTRIBUTE_FIELD; field <= reader.lastUsedField(); ++field)
        card.attributes.push_back(readDesignValue(reader, field, "attributes"));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignResp1Format::check(const DesignResp1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (const auto problem = checkNeutralName(card.responseKind, "responseKind"))
        return problem;
    if (card.ownerKind.isReal())
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a name or an integer, not a real", "ownerKind"};
    if (const auto problem = checkValue(card.ownerKind, "ownerKind"))
        return problem;
    if (const auto problem = checkOptionalId(card.regionId, "regionId"))
        return problem;
    if (const auto problem = checkValue(card.attributeA, "attributeA"))
        return problem;
    if (const auto problem = checkValue(card.attributeB, "attributeB"))
        return problem;
    for (const DesignValue& value : card.attributes)
    {
        if (const auto problem = checkValue(value, "attributes"))
            return problem;
    }
    return std::nullopt;
}

void DesignResp1Format::write(const DesignResp1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    writer.text(3, card.responseKind);
    writeDesignValue(writer, 4, card.ownerKind);
    if (card.regionId != 0)
        writer.integer(5, card.regionId);
    writeDesignValue(writer, 6, card.attributeA);
    writeDesignValue(writer, 7, card.attributeB);
    int field = FIRST_ATTRIBUTE_FIELD;
    for (const DesignValue& value : card.attributes)
        writeDesignValue(writer, field++, value);
}

std::uint32_t DesignResp1Table::append(const DesignResp1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignResp1Table::store(std::uint32_t row, const DesignResp1Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignResp1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignResp1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignResp1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitDesignResp1References(m_cards.edit(row), visitor);
}

} // namespace oor::model
