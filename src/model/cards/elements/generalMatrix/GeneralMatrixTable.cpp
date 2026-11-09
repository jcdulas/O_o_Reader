#include "oor/model/cards/elements/generalMatrix/GeneralMatrixTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;

// The first field of the line after the one that field `last` is on (the lines start at fields 1, 9, 17, ...).
int nextLineStart(int last)
{
    return ((last - 1) / FIELDS_PER_LINE + 1) * FIELDS_PER_LINE + 1;
}

enum class Part
{
    Ul,
    Ud,
    Matrix,
    S
};

// Writes the pairs of a list, one after the other, from `field`; returns the field after the last one.
int writePairs(FieldWriter& writer, int field, const std::vector<std::array<base::EntityId, 2>>& pairs)
{
    for (const auto& pair : pairs)
    {
        writer.integer(field, pair[0]);
        writer.integer(field + 1, pair[1]);
        field += 2;
    }
    return field;
}

// Writes a marker on a new line, then the values that follow it one after the other; returns the field after the last.
int writeValues(FieldWriter& writer, int markerField, std::string_view marker, const std::vector<double>& values)
{
    writer.text(markerField, marker);
    int field = markerField + 1;
    for (const double value : values)
        writer.real(field++, value);
    return field;
}

} // namespace

std::optional<GeneralMatrixCard> GeneralMatrixFormat::read(FieldReader& reader)
{
    GeneralMatrixCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value", "field 2 of a " + std::string(reader.cardName()) + " is blank",
                     "independentPoint1");
    Part part = Part::Ul;
    bool seenUd = false;
    bool seenMatrix = false;
    bool seenS = false;
    const int last = reader.lastUsedField();
    int field = 3;
    while (field <= last && !reader.failed())
    {
        if (reader.isBlank(field))
        {
            // The padding at the end of a line: a list of pairs goes on at the next line, a matrix at the next field.
            field = part == Part::Ul || part == Part::Ud ? nextLineStart(field) : field + 1;
            continue;
        }
        const std::string_view text = reader.text(field);
        if ((text == "DependentDofs"))
        {
            if (part != Part::Ul || seenUd)
            {
                reader.error(
                    "card.bad-value",
                    "UD must follow the {" + std::string("independentPoint") + "} list, once, before the matrix",
                    "dependentPoint");
                break;
            }
            seenUd = true;
            part = Part::Ud;
            if (!reader.isBlank(field + 1))
                reader.error("card.bad-value", "the field after UD is blank", "dependentPoint1");
            field += 2;
            continue;
        }
        if ((text == "Stiffness") || (text == "Flexibility"))
        {
            if (seenMatrix)
            {
                reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has one K or Z matrix",
                             "stiffness");
                break;
            }
            seenMatrix = true;
            part = Part::Matrix;
            card.isFlexibility = (text == "Flexibility");
            ++field;
            continue;
        }
        if ((text == "Dependency"))
        {
            if (!seenMatrix || seenS)
            {
                reader.error("card.bad-value", "S must follow the K or Z matrix, once", "dependencyMatrix");
                break;
            }
            seenS = true;
            part = Part::S;
            ++field;
            continue;
        }
        switch (part)
        {
        case Part::Ul:
        case Part::Ud:
        {
            const bool inUl = part == Part::Ul;
            const base::EntityId point = reader.requiredInteger(field, inUl ? "independentPoint" : "dependentPoint");
            const base::EntityId component =
                reader.integer(field + 1, inUl ? "independentComponent" : "dependentComponent", 0);
            (inUl ? card.independentDofs : card.dependentDofs).push_back({point, component});
            field += 2;
            break;
        }
        case Part::Matrix:
            card.stiffnessMatrix.push_back(reader.requiredReal(field, "stiffness"));
            ++field;
            break;
        case Part::S:
            card.dependencyMatrix.push_back(reader.requiredReal(field, "dependencyMatrix"));
            ++field;
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeneralMatrixFormat::check(const GeneralMatrixCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.independentDofs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one degree of freedom in the {{field:independentPoint}} list",
                           "independentPoint1"};
    for (const auto& pair : card.independentDofs)
    {
        if (pair[0] <= 0 || pair[1] < 0 || pair[1] > 6)
            return CardProblem{"card.bad-value",
                               "{field} must be a point ID and {field:independentComponent} a component from 0 to 6",
                               "independentPoint"};
    }
    for (const auto& pair : card.dependentDofs)
    {
        if (pair[0] <= 0 || pair[1] < 0 || pair[1] > 6)
            return CardProblem{"card.bad-value",
                               "{field} must be a point ID and {field:dependentComponent} a component from 0 to 6",
                               "dependentPoint"};
    }
    const std::size_t m = card.independentDofs.size();
    if (card.stiffnessMatrix.size() != m * (m + 1) / 2)
        return CardProblem{"card.bad-value",
                           "the K or Z matrix needs the lower triangle of the m x m matrix (m * (m + 1) / 2 values for "
                           "the m degrees of freedom of {{field:independentPoint}})",
                           "stiffness"};
    if (!card.dependencyMatrix.empty())
    {
        if (card.dependentDofs.empty())
            return CardProblem{"card.missing-field", "an S matrix needs a {{field:dependentPoint}} list",
                               "dependentPoint1"};
        if (card.dependencyMatrix.size() != m * card.dependentDofs.size())
            return CardProblem{"card.bad-value",
                               "the S matrix needs m x n values for the m degrees of freedom of "
                               "{{field:independentPoint}} and the n of {{field:dependentPoint}}",
                               "dependencyMatrix"};
    }
    else if (!card.dependentDofs.empty() && card.dependentDofs.size() != 6)
    {
        return CardProblem{"card.bad-value",
                           "a {{field:dependentPoint}} list without an S matrix must hold six degrees of freedom",
                           "dependentPoint1"};
    }
    return std::nullopt;
}

void GeneralMatrixFormat::write(const GeneralMatrixCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = writePairs(writer, 3, card.independentDofs);
    if (!card.dependentDofs.empty())
    {
        // "UD" starts a line, the field after it is blank and the pairs follow.
        const int marker = nextLineStart(field - 1);
        writer.text(marker, "DependentDofs");
        field = writePairs(writer, marker + 2, card.dependentDofs);
    }
    field = writeValues(writer, nextLineStart(field - 1), card.isFlexibility ? "Flexibility" : "Stiffness",
                        card.stiffnessMatrix);
    if (!card.dependencyMatrix.empty())
        writeValues(writer, nextLineStart(field - 1), "Dependency", card.dependencyMatrix);
}

std::uint32_t GeneralMatrixTable::append(const GeneralMatrixCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_independentDofs.push(card.independentDofs);
    m_dependentDofs.push(card.dependentDofs);
    m_flexibility.push(card.isFlexibility ? 1 : 0);
    m_stiffnessMatrix.push(card.stiffnessMatrix);
    m_dependencyMatrix.push(card.dependencyMatrix);
    return row;
}

GeneralMatrixCard GeneralMatrixTable::card(std::uint32_t row) const
{
    GeneralMatrixCard card;
    card.id = m_id.get(row);
    const auto ul = m_independentDofs.get(row);
    card.independentDofs.assign(ul.begin(), ul.end());
    const auto dependentDofs = m_dependentDofs.get(row);
    card.dependentDofs.assign(dependentDofs.begin(), dependentDofs.end());
    card.isFlexibility = m_flexibility.get(row) != 0;
    const auto stiffnessMatrix = m_stiffnessMatrix.get(row);
    card.stiffnessMatrix.assign(stiffnessMatrix.begin(), stiffnessMatrix.end());
    const auto s = m_dependencyMatrix.get(row);
    card.dependencyMatrix.assign(s.begin(), s.end());
    return card;
}

void GeneralMatrixTable::store(std::uint32_t row, const GeneralMatrixCard& card)
{
    m_id.set(row, card.id);
    m_independentDofs.set(row, card.independentDofs);
    m_dependentDofs.set(row, card.dependentDofs);
    m_flexibility.set(row, card.isFlexibility ? 1 : 0);
    m_stiffnessMatrix.set(row, card.stiffnessMatrix);
    m_dependencyMatrix.set(row, card.dependencyMatrix);
}

void GeneralMatrixTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_independentDofs.keepRows(rows);
    m_dependentDofs.keepRows(rows);
    m_flexibility.keepRows(rows);
    m_stiffnessMatrix.keepRows(rows);
    m_dependencyMatrix.keepRows(rows);
}

namespace
{

// Visits the points of a list of degrees of freedom (grid or scalar points); stores the list when one changed.
template <typename Column>
void visitPoints(Column& column, std::uint32_t row, ReferenceVisitor& visitor, std::string_view field)
{
    std::vector<std::array<base::EntityId, 2>> pairs(column.get(row).begin(), column.get(row).end());
    bool changed = false;
    for (auto& pair : pairs)
    {
        if (pair[0] <= 0)
            continue;
        const base::EntityId updated = visitor.visit(field, ReferenceTarget::ScalarOrGridPoint, pair[0]);
        changed = changed || updated != pair[0];
        pair[0] = updated;
    }
    if (changed)
        column.set(row, pairs);
}

} // namespace

void GeneralMatrixTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The points are grid points or scalar points (a scalar point may be one no SPOINT defines).
    visitPoints(m_independentDofs, row, visitor, "independentPoint");
    visitPoints(m_dependentDofs, row, visitor, "dependentPoint");
}

} // namespace oor::model
