#include "oor/model/cards/superelements/superelementLoadScale/SuperelementLoadScaleTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// An entry takes two lines of eight fields, from field 9 on: SEIDi, Si and LBLTYPi on the first, the column on the
// second.
constexpr int FIRST_ENTRY_FIELD = 9;
constexpr int FIELDS_PER_LINE = 8;
constexpr int FIELDS_PER_ENTRY = 2 * FIELDS_PER_LINE;
// The label runs over the eight fields of its line.

// The selector a word names: the words of the format reference, and the two the descriptions of the format add.
std::optional<SuperelementLoadScaleSelector> selectorOf(std::string_view word)
{
    if (word.empty() || (word == "Subcase"))
        return SuperelementLoadScaleSelector::Label;
    if ((word == "Column"))
        return SuperelementLoadScaleSelector::ColumnIndex;
    return parseKeyword<SuperelementLoadScaleSelector>(SUPERELEMENT_LOAD_SCALE_SELECTOR_WORDS, word);
}

} // namespace

std::optional<SuperelementLoadScaleCard> SuperelementLoadScaleFormat::read(FieldReader& reader)
{
    SuperelementLoadScaleCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scale = reader.real(2, "scaleFactor", 1.0);
    if (reader.lastUsedField() >= 3 && reader.lastUsedField() < FIRST_ENTRY_FIELD)
        reader.error("card.bad-value",
                     "the entries of an " + std::string(reader.cardName()) + " start on the first continuation line",
                     "superelementId1");
    const int last = reader.lastUsedField();
    for (int first = FIRST_ENTRY_FIELD; first <= last && !reader.failed(); first += FIELDS_PER_ENTRY)
    {
        const int second = first + FIELDS_PER_LINE; // the line of the column
        SuperelementLoadScaleEntry entry;
        entry.superelementId = reader.requiredInteger(first, "superelementId");
        entry.scale = reader.real(first + 1, "scaleFactors", 1.0);
        const std::optional<SuperelementLoadScaleSelector> selector = selectorOf(reader.text(first + 2));
        if (!selector)
        {
            reader.error("card.bad-value",
                         std::string("labelKind") + " must be " + keywordList(SUPERELEMENT_LOAD_SCALE_SELECTOR_WORDS) +
                             ", not \"" + std::string(reader.text(first + 2)) + "\"",
                         "labelKind");
            break;
        }
        entry.selector = *selector;
        if (entry.selector == SuperelementLoadScaleSelector::Label)
            entry.label = reader.spanText(second, second + FIELDS_PER_LINE - 1);
        else
            entry.number = reader.requiredInteger(second, "labelledId");
        card.entries.push_back(std::move(entry));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementLoadScaleFormat::check(const SuperelementLoadScaleCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.scale, "scaleFactor", RealRule::Finite))
        return problem;
    if (card.entries.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one superelement " + std::string(CardProblem::FIELD),
                           "superelementId1"};
    for (const SuperelementLoadScaleEntry& entry : card.entries)
    {
        if (const auto problem = checkSuperelementId(entry.superelementId, "superelementId", false))
            return problem;
        if (const auto problem = checkReal(entry.scale, "scaleFactors", RealRule::Finite))
            return problem;
        if (entry.selector == SuperelementLoadScaleSelector::Label)
        {
            if (entry.label.empty())
                return CardProblem{"card.missing-field",
                                   "an entry with " + CardProblem::fieldNamed("labelKind") + " " +
                                       std::string(CardProblem::FIELD) + " needs its label",
                                   "entryLabel"};
            if (const auto problem = superelementLabelProblem(entry.label, "entryLabel"))
                return problem;
        }
        else if (const auto problem = checkRequiredId(entry.number, "labelledId"))
        {
            return problem;
        }
    }
    return std::nullopt;
}

void SuperelementLoadScaleFormat::write(const SuperelementLoadScaleCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.scale, 1.0);
    int first = FIRST_ENTRY_FIELD;
    for (const SuperelementLoadScaleEntry& entry : card.entries)
    {
        const int second = first + FIELDS_PER_LINE;
        writer.integer(first, entry.superelementId);
        writer.realUnlessDefault(first + 1, entry.scale, 1.0);
        writeKeywordUnlessDefault(writer, first + 2, SUPERELEMENT_LOAD_SCALE_SELECTOR_WORDS, entry.selector,
                                  SuperelementLoadScaleSelector::Label);
        if (entry.selector == SuperelementLoadScaleSelector::Label)
            writer.spanText(second, FIELDS_PER_LINE, entry.label);
        else
            writer.integer(second, entry.number);
        first += FIELDS_PER_ENTRY;
    }
}

std::uint32_t SuperelementLoadScaleTable::append(const SuperelementLoadScaleCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SuperelementLoadScaleTable::store(std::uint32_t row, const SuperelementLoadScaleCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SuperelementLoadScaleTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementLoadScaleTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementLoadScaleTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (SuperelementLoadScaleEntry& entry : m_cards.edit(row).entries)
        visitSuperelement(visitor, "superelementId", entry.superelementId);
}

} // namespace oor::model
