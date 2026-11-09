#include "oor/model/cards/loads/metadataRealPairs/MetadataRealPairsTable.hpp"

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

std::optional<MetadataRealPairsCard> MetadataRealPairsFormat::read(FieldReader& reader)
{
    MetadataRealPairsCard card;
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
        MetadataRealPairsPair pair;
        pair.name = std::string(reader.text(field));
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field", "the name " + pair.name + " needs its value in the next field",
                         "entryValue");
            break;
        }
        pair.value = reader.requiredReal(field + 1, "entryValue");
        card.pairs.push_back(std::move(pair));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MetadataRealPairsFormat::check(const MetadataRealPairsCard& card)
{
    if (const auto problem = checkRequiredId(card.metadataGroupId, "metadataGroupId"))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("entryValue") + " pair is required",
                           "entryName"};
    for (const MetadataRealPairsPair& pair : card.pairs)
    {
        if (const auto problem = metadataNameProblem(pair.name))
            return problem;
        if (const auto problem = checkReal(pair.value, "entryValue", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void MetadataRealPairsFormat::write(const MetadataRealPairsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.metadataGroupId);
    int field = FIRST_PAIR_FIELD;
    for (const MetadataRealPairsPair& pair : card.pairs)
    {
        writer.text(field, pair.name);
        writer.real(field + 1, pair.value);
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t MetadataRealPairsTable::append(const MetadataRealPairsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_metadataGroupId.push(card.metadataGroupId);
    m_cards.push(card);
    return row;
}

void MetadataRealPairsTable::store(std::uint32_t row, const MetadataRealPairsCard& card)
{
    m_metadataGroupId.set(row, card.metadataGroupId);
    m_cards.set(row, card);
}

void MetadataRealPairsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_metadataGroupId.keepRows(rows);
    m_cards.keepRows(rows);
}

void MetadataRealPairsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_metadataGroupId.set(row, id);
    m_cards.edit(row).metadataGroupId = id;
}

void MetadataRealPairsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the group the pairs belong to: one of the ID of the ABSTRCT cards. The column follows the card.
    MetadataRealPairsCard& card = m_cards.edit(row);
    visitMetadataGroup(visitor, card.metadataGroupId);
    m_metadataGroupId.set(row, card.metadataGroupId);
}

} // namespace oor::model
