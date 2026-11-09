#include "oor/model/cards/loads/loadCaseIdLabel/LoadCaseIdLabelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <utility>

namespace oor::model
{

namespace
{

// The items start on the first continuation line. An item takes two lines of eight fields: the scale factor and the
// load ID in the first two fields of the first, the name over the whole second.
constexpr int FIRST_ITEM_FIELD = 9;
constexpr int FIELDS_PER_ITEM = 16;
constexpr int NAME_OFFSET = 8;

} // namespace

std::optional<LoadCaseIdLabelCard> LoadCaseIdLabelFormat::read(FieldReader& reader)
{
    LoadCaseIdLabelCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scale = reader.requiredReal(2, "scaleFactor");
    std::vector<int> usedFields{1, 2};
    const int last = reader.lastUsedField();
    for (int first = FIRST_ITEM_FIELD; first <= last && !reader.failed(); first += FIELDS_PER_ITEM)
    {
        // An item is there when one of its fields is not blank (the lines of an item that was left out are padding).
        bool blank = true;
        for (int field = first; field < first + FIELDS_PER_ITEM && blank; ++field)
            blank = reader.isBlank(field);
        if (blank)
            continue;
        LoadCaseIdLabelItem item;
        item.scale = reader.requiredReal(first, "scaleFactors");
        item.loadCombinationId = reader.requiredInteger(first + 1, "loadIds");
        item.partName = readPaaName(reader, first + NAME_OFFSET, "partNames");
        card.items.push_back(std::move(item));
        usedFields.push_back(first);
        usedFields.push_back(first + 1);
        for (int field = first + NAME_OFFSET; field < first + NAME_OFFSET + PAA_NAME_FIELDS; ++field)
            usedFields.push_back(field);
    }
    if (!reader.failed())
        requireOnlyFields(reader, usedFields);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LoadCaseIdLabelFormat::check(const LoadCaseIdLabelCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.scale, "scaleFactor", RealRule::Finite))
        return problem;
    if (card.scale == 0.0)
        return CardProblem{
            "card.bad-value",
            "the overall scale factor " + std::string(CardProblem::FIELD) + " must be a number other than zero",
            "scaleFactor"};
    if (card.items.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one item (" + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("loadIds") + ", " + CardProblem::fieldNamed("partNames") + ")",
                           "scaleFactor1"};
    for (const LoadCaseIdLabelItem& item : card.items)
    {
        if (const auto problem = checkReal(item.scale, "scaleFactors", RealRule::Finite))
            return problem;
        if (item.scale == 0.0)
            return CardProblem{
                "card.bad-value",
                "the scale factor " + std::string(CardProblem::FIELD) + " must be a number other than zero",
                "scaleFactors"};
        if (const auto problem = checkRequiredId(item.loadCombinationId, "loadIds"))
            return problem;
        if (const auto problem = checkPaaName(item.partName, "partNames"))
            return problem;
    }
    return std::nullopt;
}

void LoadCaseIdLabelFormat::write(const LoadCaseIdLabelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scale);
    int first = FIRST_ITEM_FIELD;
    for (const LoadCaseIdLabelItem& item : card.items)
    {
        writer.real(first, item.scale);
        writer.integer(first + 1, item.loadCombinationId);
        writePaaName(writer, first + NAME_OFFSET, item.partName);
        first += FIELDS_PER_ITEM;
    }
}

std::uint32_t LoadCaseIdLabelTable::append(const LoadCaseIdLabelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void LoadCaseIdLabelTable::store(std::uint32_t row, const LoadCaseIdLabelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void LoadCaseIdLabelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void LoadCaseIdLabelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
