#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_ENTRY = 4;

} // namespace

std::optional<DesignFieldConstantsCard> DesignFieldConstantsFormat::read(FieldReader& reader)
{
    DesignFieldConstantsCard card;
    for (int field = 1; field <= reader.lastUsedField(); field += FIELDS_PER_ENTRY)
    {
        bool allBlank = true;
        bool anyBlank = false;
        for (int index = 0; index < FIELDS_PER_ENTRY; ++index)
        {
            allBlank = allBlank && reader.isBlank(field + index);
            anyBlank = anyBlank || reader.isBlank(field + index);
        }
        if (allBlank)
            continue;
        if (anyBlank)
        {
            reader.error("card.missing-field", "an entry needs its label, card name, ID and field name", "label");
            break;
        }
        DesignFieldConstantsEntry entry;
        entry.label = std::string(reader.text(field));
        entry.cardName = std::string(reader.text(field + 1));
        entry.targetId = reader.requiredInteger(field + 2, "targetId");
        entry.field = std::string(reader.text(field + 3));
        card.entries.push_back(std::move(entry));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignFieldConstantsFormat::check(const DesignFieldConstantsCard& card)
{
    if (card.entries.empty())
        return CardProblem{"card.missing-field", "at least one constant is required", "label"};
    for (const DesignFieldConstantsEntry& entry : card.entries)
    {
        if (const auto problem = checkDesignLabel(entry.label, "label"))
            return problem;
        if (const auto problem = checkNeutralName(entry.cardName, "targetCard"))
            return problem;
        if (const auto problem = checkRequiredId(entry.targetId, "targetId"))
            return problem;
        if (const auto problem = checkDesignFieldName(entry.field, "fieldName"))
            return problem;
    }
    return std::nullopt;
}

void DesignFieldConstantsFormat::write(const DesignFieldConstantsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const DesignFieldConstantsEntry& entry : card.entries)
    {
        writer.text(field, entry.label);
        writer.text(field + 1, entry.cardName);
        writer.integer(field + 2, entry.targetId);
        writer.text(field + 3, entry.field);
        field += FIELDS_PER_ENTRY;
    }
}

std::uint32_t DesignFieldConstantsTable::append(const DesignFieldConstantsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void DesignFieldConstantsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (DesignFieldConstantsEntry& entry : m_cards.edit(row).entries)
    {
        if (const auto target = targetOfDesignedCard(entry.cardName))
            visitId(visitor, "targetId", *target, entry.targetId);
    }
}

} // namespace oor::model
