#include "oor/model/cards/superelements/superelementDesignResp2/SuperelementDesignResp2Table.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 9;

bool isOneOf(std::string_view word, std::span<const std::string_view> words) noexcept
{
    return std::find(words.begin(), words.end(), word) != words.end();
}

} // namespace

std::optional<SuperelementDesignResp2Card> SuperelementDesignResp2Format::read(FieldReader& reader)
{
    SuperelementDesignResp2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    // EQID or FUNC: an integer is the equation, anything else the name of a function.
    const std::string_view third = reader.text(3);
    if (third.empty())
        reader.error("card.missing-field",
                     std::string("equationId") + " or " + std::string("function") + " is required", "equationId");
    else if (base::NumberParser::parseInteger(third))
        card.equationId = reader.requiredInteger(3, "equationId");
    else
        card.function = std::string(third);
    card.regionId = reader.integer(4, "regionId", 0);
    card.methodName = std::string(reader.text(5));
    card.constant1 = reader.real(6, "constant1", SuperelementDesignResp2Card::DEFAULT_CONSTANT1);
    card.constant2 = reader.real(7, "constant2", SuperelementDesignResp2Card::DEFAULT_CONSTANT2);
    card.constant3 = reader.real(8, "constant3", SuperelementDesignResp2Card::DEFAULT_CONSTANT3);
    card.groups = readSuperelementGroups(reader, FIRST_GROUP_FIELD, SUPERELEMENT_DESIGN_RESP2_GROUPS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementDesignResp2Format::check(const SuperelementDesignResp2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (card.function.empty())
    {
        if (const auto problem = checkRequiredId(card.equationId, "equationId"))
            return problem;
    }
    else
    {
        if (card.equationId != 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("function") +
                                   " are one field: give an equation or a function, not both",
                               "equationId"};
        if (!isOneOf(card.function, DESIGN_RESP2_FUNCTIONS))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be " + keywordList(DESIGN_RESP2_FUNCTIONS) +
                                   ", not " + card.function,
                               "function"};
    }
    if (const auto problem = checkOptionalId(card.regionId, "regionId"))
        return problem;
    if (!card.methodName.empty() && !isOneOf(card.methodName, DESIGN_RESP2_METHODS))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " + keywordList(DESIGN_RESP2_METHODS) +
                               ", not " + card.methodName,
                           "methodName"};
    if (const auto problem = checkReal(card.constant1, "constant1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.constant2, "constant2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.constant3, "constant3", RealRule::Finite))
        return problem;
    return checkSuperelementGroups(card.groups, SUPERELEMENT_DESIGN_RESP2_GROUPS);
}

void SuperelementDesignResp2Format::write(const SuperelementDesignResp2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    if (card.function.empty())
        writer.integer(3, card.equationId);
    else
        writer.text(3, card.function);
    if (card.regionId != 0)
        writer.integer(4, card.regionId);
    writer.text(5, card.methodName);
    writer.realUnlessDefault(6, card.constant1, SuperelementDesignResp2Card::DEFAULT_CONSTANT1);
    writer.realUnlessDefault(7, card.constant2, SuperelementDesignResp2Card::DEFAULT_CONSTANT2);
    writer.realUnlessDefault(8, card.constant3, SuperelementDesignResp2Card::DEFAULT_CONSTANT3);
    writeSuperelementGroups(writer, FIRST_GROUP_FIELD, card.groups, SUPERELEMENT_DESIGN_RESP2_GROUPS);
}

std::uint32_t SuperelementDesignResp2Table::append(const SuperelementDesignResp2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SuperelementDesignResp2Table::store(std::uint32_t row, const SuperelementDesignResp2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SuperelementDesignResp2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementDesignResp2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementDesignResp2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementDesignResp2Card& card = m_cards.edit(row);
    visitId(visitor, "equationId", ReferenceTarget::DesignEquation, card.equationId);
    visitSuperelementGroups(visitor, card.groups, SUPERELEMENT_DESIGN_RESP2_GROUPS);
}

} // namespace oor::model
