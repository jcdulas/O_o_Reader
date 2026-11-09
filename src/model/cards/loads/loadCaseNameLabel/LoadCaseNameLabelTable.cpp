#include "oor/model/cards/loads/loadCaseNameLabel/LoadCaseNameLabelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <utility>

namespace oor::model
{

namespace
{

// The items start on the first continuation line. An item takes three lines of eight fields: the scale factor in the
// first field of the first, the name of the load over the whole second and the name of the part over the whole third.
constexpr int FIRST_ITEM_FIELD = 9;
constexpr int FIELDS_PER_ITEM = 24;
constexpr int LOAD_NAME_OFFSET = 8;
constexpr int PART_NAME_OFFSET = 16;

} // namespace

std::optional<LoadCaseNameLabelCard> LoadCaseNameLabelFormat::read(FieldReader& reader)
{
    LoadCaseNameLabelCard card;
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
        LoadCaseNameLabelItem item;
        item.scale = reader.requiredReal(first, "scaleFactors");
        item.loadName = readPaaName(reader, first + LOAD_NAME_OFFSET, "loadNames");
        item.partName = readPaaName(reader, first + PART_NAME_OFFSET, "partNames");
        card.items.push_back(std::move(item));
        usedFields.push_back(first);
        for (int field = first + LOAD_NAME_OFFSET; field < first + FIELDS_PER_ITEM; ++field)
            usedFields.push_back(field);
    }
    if (!reader.failed())
        requireOnlyFields(reader, usedFields);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LoadCaseNameLabelFormat::check(const LoadCaseNameLabelCard& card)
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
                               CardProblem::fieldNamed("loadNames") + ", " + CardProblem::fieldNamed("partNames") + ")",
                           "scaleFactor1"};
    for (const LoadCaseNameLabelItem& item : card.items)
    {
        if (const auto problem = checkReal(item.scale, "scaleFactors", RealRule::Finite))
            return problem;
        if (item.scale == 0.0)
            return CardProblem{
                "card.bad-value",
                "the scale factor " + std::string(CardProblem::FIELD) + " must be a number other than zero",
                "scaleFactors"};
        if (const auto problem = checkPaaName(item.loadName, "loadNames"))
            return problem;
        if (const auto problem = checkPaaName(item.partName, "partNames"))
            return problem;
    }
    return std::nullopt;
}

void LoadCaseNameLabelFormat::write(const LoadCaseNameLabelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scale);
    int first = FIRST_ITEM_FIELD;
    for (const LoadCaseNameLabelItem& item : card.items)
    {
        writer.real(first, item.scale);
        writePaaName(writer, first + LOAD_NAME_OFFSET, item.loadName);
        writePaaName(writer, first + PART_NAME_OFFSET, item.partName);
        first += FIELDS_PER_ITEM;
    }
}

std::uint32_t LoadCaseNameLabelTable::append(const LoadCaseNameLabelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void LoadCaseNameLabelTable::store(std::uint32_t row, const LoadCaseNameLabelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void LoadCaseNameLabelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void LoadCaseNameLabelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
