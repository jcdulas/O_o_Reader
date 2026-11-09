#include "oor/model/cards/nodes/moduleMultiPointConstr/ModuleMultiPointConstrTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_TERM = 4;
constexpr int FIRST_UNUSED_FIELD = 6; // the rest of the first line is blank: the next term starts the continuation line
constexpr int LAST_UNUSED_FIELD = 8;
constexpr int FIRST_CONTINUED_FIELD = 9;

// The field of the module of term `index` (its point, component and coefficient follow it): the first term is next to
// the SID, the others follow each other from the first field of the continuation line.
constexpr int firstFieldOf(std::size_t index) noexcept
{
    if (index == 0)
        return 2;
    return FIRST_CONTINUED_FIELD + FIELDS_PER_TERM * (static_cast<int>(index) - 1);
}

} // namespace

std::optional<ModuleMultiPointConstrCard> ModuleMultiPointConstrFormat::read(FieldReader& reader)
{
    ModuleMultiPointConstrCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = FIRST_UNUSED_FIELD; field <= LAST_UNUSED_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "fields 6 to 8 of an " + std::string(reader.cardName()) + " are not used and must be blank",
                         "");
            break;
        }
    }
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int first = firstFieldOf(index);
        if (reader.isBlank(first) && reader.isBlank(first + 1) && reader.isBlank(first + 2) &&
            reader.isBlank(first + 3))
            continue; // padding
        ModuleMultiPointConstrTerm term;
        term.point.module = reader.integer(first, "moduleId", 0);
        term.point.id = reader.requiredInteger(first + 1, "node");
        term.component = reader.integer(first + 2, "components", 0);
        // format reference: Aj is 0.0 when blank, except A1, which must be given and not be zero.
        term.coefficient = card.terms.empty() ? reader.requiredReal(first + 3, "coefficient1")
                                              : reader.real(first + 3, "coefficient", 0.0);
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleMultiPointConstrFormat::check(const ModuleMultiPointConstrCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one term", "node"};
    for (const ModuleMultiPointConstrTerm& term : card.terms)
    {
        if (const auto problem = checkModuleId(term.point, "moduleId", "node"))
            return problem;
        if (term.component < 0 || term.component > 6)
            return CardProblem{"card.bad-value", "{field} must be one of the integers 1 to 6, or 0 for a scalar point",
                               "components"};
        if (const auto problem = checkReal(term.coefficient, "coefficient", RealRule::Finite))
            return problem;
    }
    if (card.terms.front().coefficient == 0.0)
        return CardProblem{"card.bad-value", "the coefficient of the dependent term ({field}) must not be zero",
                           "coefficient1"};
    return std::nullopt;
}

void ModuleMultiPointConstrFormat::write(const ModuleMultiPointConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.terms.size(); ++index)
    {
        const ModuleMultiPointConstrTerm& term = card.terms[index];
        const int first = firstFieldOf(index);
        writeModuleId(writer, first, first + 1, term.point, false);
        writer.integerUnlessDefault(first + 2, term.component, 0);
        writer.real(first + 3, term.coefficient);
    }
}

std::uint32_t ModuleMultiPointConstrTable::append(const ModuleMultiPointConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleMultiPointConstrTable::store(std::uint32_t row, const ModuleMultiPointConstrCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleMultiPointConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleMultiPointConstrTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleMultiPointConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A point is a grid or scalar point (a Node: scalar points share the node IDs); only the main Bulk Data is looked
    // up.
    for (ModuleMultiPointConstrTerm& term : m_cards.edit(row).terms)
        visitModuleId(visitor, "node", ReferenceTarget::Node, term.point);
}

} // namespace oor::model
