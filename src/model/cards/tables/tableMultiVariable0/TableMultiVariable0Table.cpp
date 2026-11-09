#include "oor/model/cards/tables/tableMultiVariable0/TableMultiVariable0Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

namespace oor::model
{

namespace
{

// The pairs start on the first continuation line.
constexpr int FIRST_PAIR_FIELD = 9;

} // namespace

std::optional<TableMultiVariable0Card> TableMultiVariable0Format::read(FieldReader& reader)
{
    TableMultiVariable0Card card;
    card.id = reader.requiredInteger(1, "id");
    // The text of the format reference lists KIND right after ITID, but the layout it draws leaves field 2 empty: a
    // deck that puts KIND there is refused rather than read with its fields moved.
    if (!reader.isBlank(2))
        reader.error(
            "card.bad-value",
            "field 2 of a " + std::string(reader.cardName()) + " is not used (" + std::string("kind") + " is field 3)",
            "kind");
    card.kind = reader.requiredInteger(3, "kind");
    card.extrapolation = reader.integer(4, "extrapolation", TABLE_DEFAULT_EXTRAPOLATION);
    card.belowRangeTableId = reader.integer(5, "belowRangeTableId", 0);
    card.aboveRangeTableId = reader.integer(6, "aboveRangeTableId", 0);
    card.smoothing = reader.integer(7, "smoothing", 0);
    if (!reader.isBlank(8))
        reader.error("card.bad-value", "field 8 of a " + std::string(reader.cardName()) + " is not used", "smoothing");
    const int last = reader.lastUsedField();
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding: a line that stops short of its end
        const double x = reader.requiredReal(field, "abscissa");
        const double y = reader.requiredReal(field + 1, "ordinate");
        if (reader.failed())
            break;
        card.abscissa.push_back(x);
        card.ordinate.push_back(y);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableMultiVariable0Format::check(const TableMultiVariable0Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkIndependentVariableKind(card.kind, "kind"))
        return problem;
    if (const auto problem = checkExtrapolationFlag(card.extrapolation, "extrapolation"))
        return problem;
    if (const auto problem = checkSmoothingFlag(card.smoothing, "smoothing"))
        return problem;
    if (card.abscissa.size() != card.ordinate.size())
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("abscissa") + " and {field} must have the same number of values",
                           "ordinate"};
    if (card.abscissa.empty())
        return CardProblem{"card.missing-field",
                           "the table needs at least one ({field}, " + CardProblem::fieldNamed("ordinate") + ") pair",
                           "abscissa"};
    if (const auto problem = checkFiniteTableValues(card.abscissa, "abscissa"))
        return problem;
    return checkFiniteTableValues(card.ordinate, "ordinate");
}

void TableMultiVariable0Format::write(const TableMultiVariable0Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(3, card.kind);
    writer.integerUnlessDefault(4, card.extrapolation, TABLE_DEFAULT_EXTRAPOLATION);
    writer.integerUnlessDefault(5, card.belowRangeTableId, 0);
    writer.integerUnlessDefault(6, card.aboveRangeTableId, 0);
    writer.integerUnlessDefault(7, card.smoothing, 0);
    int field = FIRST_PAIR_FIELD;
    for (std::size_t index = 0; index < card.abscissa.size() && index < card.ordinate.size(); ++index)
    {
        writer.real(field, card.abscissa[index]);
        writer.real(field + 1, card.ordinate[index]);
        field += 2;
    }
}

std::uint32_t TableMultiVariable0Table::append(const TableMultiVariable0Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TableMultiVariable0Table::store(std::uint32_t row, const TableMultiVariable0Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TableMultiVariable0Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TableMultiVariable0Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
