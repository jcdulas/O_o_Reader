#include "oor/model/cards/loads/metadataIntegerPairs/MetadataIntegerPairsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/MetadataPairs.hpp"

namespace oor::model
{

namespace
{

constexpr int BLANK_FIELD = 2;
constexpr int FIRST_PAIR_FIELD = 3;
constexpr int FIELDS_PER_PAIR = 2;

} // namespace

std::optional<MetadataIntegerPairsCard> MetadataIntegerPairsFormat::read(FieldReader& reader)
{
    MetadataIntegerPairsCard card;
    card.metadataGroupId = reader.requiredInteger(1, "metadataGroupId");
    if (!reader.isBlank(BLANK_FIELD))
        reader.error("card.bad-value", "field 3 of the card is blank: the first pair starts in field 4", "entryName");
    const int last = reader.lastUsedField();
    // The pairs sit side by side from field 3. A pair whose two fields are blank is padding.
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += FIELDS_PER_PAIR)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field",
                         "a value needs the " + std::string("entryName") + " of its pair in the field before it",
                         "entryName");
            break;
        }
        MetadataIntegerPairsPair pair;
        pair.name = std::string(reader.text(field));
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field", "the name " + pair.name + " needs its value in the next field",
                         "entryValue");
            break;
        }
        pair.value = reader.requiredInteger(field + 1, "entryValue");
        card.pairs.push_back(std::move(pair));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MetadataIntegerPairsFormat::check(const MetadataIntegerPairsCard& card)
{
    if (const auto problem = checkRequiredId(card.metadataGroupId, "metadataGroupId"))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("entryValue") + " pair is required",
                           "entryName"};
    for (const MetadataIntegerPairsPair& pair : card.pairs)
    {
        if (const auto problem = metadataNameProblem(pair.name))
            return problem;
    }
    return std::nullopt;
}

void MetadataIntegerPairsFormat::write(const MetadataIntegerPairsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.metadataGroupId);
    int field = FIRST_PAIR_FIELD;
    for (const MetadataIntegerPairsPair& pair : card.pairs)
    {
        writer.text(field, pair.name);
        writer.integer(field + 1, pair.value);
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t MetadataIntegerPairsTable::append(const MetadataIntegerPairsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_metadataGroupId.push(card.metadataGroupId);
    m_cards.push(card);
    return row;
}

void MetadataIntegerPairsTable::store(std::uint32_t row, const MetadataIntegerPairsCard& card)
{
    m_metadataGroupId.set(row, card.metadataGroupId);
    m_cards.set(row, card);
}

void MetadataIntegerPairsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_metadataGroupId.keepRows(rows);
    m_cards.keepRows(rows);
}

void MetadataIntegerPairsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_metadataGroupId.set(row, id);
    m_cards.edit(row).metadataGroupId = id;
}

void MetadataIntegerPairsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the group the pairs belong to: one of the ID of the ABSTRCT cards. The column follows the card.
    MetadataIntegerPairsCard& card = m_cards.edit(row);
    visitMetadataGroup(visitor, card.metadataGroupId);
    m_metadataGroupId.set(row, card.metadataGroupId);
}

} // namespace oor::model
