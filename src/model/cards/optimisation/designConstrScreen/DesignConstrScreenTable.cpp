#include "oor/model/cards/optimisation/designConstrScreen/DesignConstrScreenTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 3;

} // namespace

std::optional<DesignConstrScreenCard> DesignConstrScreenFormat::read(FieldReader& reader)
{
    DesignConstrScreenCard card;
    card.responseKind = std::string(reader.text(1));
    if (card.responseKind.empty())
        reader.error("card.missing-field", std::string("responseKind") + " is required", "responseKind");
    card.threshold = reader.real(2, "threshold", DesignConstrScreenCard::DEFAULT_THRESHOLD);
    card.maxRetained = reader.integer(3, "maxRetained", DesignConstrScreenCard::DEFAULT_MAX_RETAINED);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignConstrScreenFormat::check(const DesignConstrScreenCard& card)
{
    if (const auto problem = checkNeutralName(card.responseKind, "responseKind"))
        return problem;
    if (const auto problem = checkReal(card.threshold, "threshold", RealRule::Finite))
        return problem;
    if (card.maxRetained <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "maxRetained"};
    return std::nullopt;
}

void DesignConstrScreenFormat::write(const DesignConstrScreenCard& card, FieldWriter& writer)
{
    writer.text(1, card.responseKind);
    writer.realUnlessDefault(2, card.threshold, DesignConstrScreenCard::DEFAULT_THRESHOLD);
    writer.integerUnlessDefault(3, card.maxRetained, DesignConstrScreenCard::DEFAULT_MAX_RETAINED);
}

std::uint32_t DesignConstrScreenTable::append(const DesignConstrScreenCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_responseKind.push(card.responseKind);
    m_threshold.push(card.threshold);
    m_maxRetained.push(card.maxRetained);
    return row;
}

DesignConstrScreenCard DesignConstrScreenTable::card(std::uint32_t row) const
{
    return DesignConstrScreenCard{m_responseKind.get(row), m_threshold.get(row), m_maxRetained.get(row)};
}

void DesignConstrScreenTable::store(std::uint32_t row, const DesignConstrScreenCard& card)
{
    m_responseKind.set(row, card.responseKind);
    m_threshold.set(row, card.threshold);
    m_maxRetained.set(row, card.maxRetained);
}

void DesignConstrScreenTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_responseKind.keepRows(rows);
    m_threshold.keepRows(rows);
    m_maxRetained.keepRows(rows);
}

} // namespace oor::model
