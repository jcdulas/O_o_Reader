#include "oor/model/cards/matrices/directMatrixAeroK/DirectMatrixAeroKTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"
#include "oor/model/core/Model.hpp"

#include <cmath>
#include <set>

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 5;
constexpr int FIELDS_PER_TERM = 4;

} // namespace

std::optional<DirectMatrixAeroKCard> DirectMatrixAeroKFormat::read(FieldReader& reader)
{
    DirectMatrixAeroKCard card;
    const std::string_view name = reader.text(1);
    if (!isMatrixName(name))
    {
        reader.error("card.bad-value",
                     std::string("entryName") + " must be a nonempty matrix name without control characters, found \"" +
                         std::string(name) + "\"",
                     "entryName");
        return card;
    }
    card.entryName = matrixNameText(name);
    card.nodeJ = reader.requiredInteger(2, "nodeJ");
    if (card.isHeader())
    {
        card.matrixShape = reader.requiredInteger(3, "matrixShape");
        card.inputType = reader.requiredInteger(4, "inputType");
        card.outputType = reader.integer(5, "outputType", 0);
        card.polarFlag = reader.integer(6, "polarFlag", 0);
        card.columnCount = reader.integer(8, "columnCount", 0);
        if (!reader.failed())
            reader.report(check(card));
        return card;
    }
    card.componentsJ = reader.integer(3, "componentsJ", 0);
    const int last = reader.lastUsedField();
    for (int field = FIRST_TERM_FIELD; field <= last && !reader.failed(); field += FIELDS_PER_TERM)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2) &&
            reader.isBlank(field + 3))
            continue; // padding
        DirectMatrixNodalTerm term;
        term.point = reader.requiredInteger(field, "node");
        term.component = reader.integer(field + 1, "components", 0);
        term.real = reader.real(field + 2, "realValue", 0.0);
        if (!reader.isBlank(field + 3))
        {
            term.imaginary = reader.real(field + 3, "imaginaryValue", 0.0);
            card.imaginary = true;
        }
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DirectMatrixAeroKFormat::check(const DirectMatrixAeroKCard& card)
{
    if (!isMatrixName(card.entryName))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be a nonempty matrix name without control characters",
            "entryName"};
    if (card.isHeader())
    {
        if (card.matrixShape != 1 && card.matrixShape != 2 && card.matrixShape != 6 && card.matrixShape != 9)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 1 (square), 2 or 9 (rectangular) or 6 (symmetric)",
                "matrixShape"};
        if (card.inputType < 1 || card.inputType > 4)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or 4",
                               "inputType"};
        if (card.outputType < 0 || card.outputType > 4)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2, 3 or 4",
                               "outputType"};
        if (card.polarFlag < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "polarFlag"};
        if (card.columnCount < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "columnCount"};
        if (!card.terms.empty() || card.componentsJ != 0)
            return CardProblem{"card.bad-value", "a header card has no column or terms", "nodeJ"};
        return std::nullopt;
    }
    if (card.nodeJ < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "nodeJ"};
    if (card.componentsJ < 0 || card.componentsJ > 6)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar or extra point",
            "componentsJ"};
    for (const DirectMatrixNodalTerm& term : card.terms)
    {
        if (term.point <= 0)
            return CardProblem{"card.bad-id", "a mesh point identification number must be a positive integer", "node"};
        if (term.component < 0 || term.component > 6)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar point",
                "components"};
        if (!std::isfinite(term.real) || !std::isfinite(term.imaginary))
            return CardProblem{"card.bad-value", "a term of the matrix must be a finite number", "realValue"};
        if (!card.imaginary && term.imaginary != 0.0)
            return CardProblem{"card.bad-value", "a column of a real matrix has no imaginary parts (B is blank)",
                               "imaginaryValue"};
    }
    return std::nullopt;
}

void DirectMatrixAeroKFormat::write(const DirectMatrixAeroKCard& card, FieldWriter& writer)
{
    writer.text(1, card.entryName);
    writer.integer(2, card.nodeJ);
    if (card.isHeader())
    {
        writer.integer(3, card.matrixShape);
        writer.integer(4, card.inputType);
        writer.integer(5, card.outputType);
        writer.integerUnlessDefault(6, card.polarFlag, 0);
        writer.integerUnlessDefault(8, card.columnCount, 0);
        return;
    }
    writer.integerUnlessDefault(3, card.componentsJ, 0);
    int field = FIRST_TERM_FIELD;
    for (const DirectMatrixNodalTerm& term : card.terms)
    {
        writer.integer(field, term.point);
        writer.integerUnlessDefault(field + 1, term.component, 0);
        writer.real(field + 2, term.real);
        if (card.imaginary)
            writer.real(field + 3, term.imaginary);
        field += FIELDS_PER_TERM;
    }
}

std::uint32_t DirectMatrixAeroKTable::append(const DirectMatrixAeroKCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_nodeJ.push(card.nodeJ);
    m_componentsJ.push(card.componentsJ);
    m_matrixShape.push(card.matrixShape);
    m_inputType.push(card.inputType);
    m_outputType.push(card.outputType);
    m_polarFlag.push(card.polarFlag);
    m_columnCount.push(card.columnCount);
    m_imaginary.push(card.imaginary ? 1 : 0);
    std::vector<base::EntityId> points;
    std::vector<std::int32_t> components;
    std::vector<double> reals;
    std::vector<double> imaginaries;
    for (const DirectMatrixNodalTerm& term : card.terms)
    {
        points.push_back(term.point);
        components.push_back(term.component);
        reals.push_back(term.real);
        imaginaries.push_back(term.imaginary);
    }
    m_point.push(points);
    m_component.push(components);
    m_real.push(reals);
    m_imaginaryPart.push(imaginaries);
    return row;
}

DirectMatrixAeroKCard DirectMatrixAeroKTable::card(std::uint32_t row) const
{
    DirectMatrixAeroKCard card;
    card.entryName = m_entryName.get(row);
    card.nodeJ = m_nodeJ.get(row);
    card.componentsJ = m_componentsJ.get(row);
    card.matrixShape = m_matrixShape.get(row);
    card.inputType = m_inputType.get(row);
    card.outputType = m_outputType.get(row);
    card.polarFlag = m_polarFlag.get(row);
    card.columnCount = m_columnCount.get(row);
    card.imaginary = m_imaginary.get(row) != 0;
    const auto points = m_point.get(row);
    const auto components = m_component.get(row);
    const auto reals = m_real.get(row);
    const auto imaginaries = m_imaginaryPart.get(row);
    for (std::size_t index = 0; index < points.size(); ++index)
        card.terms.push_back(DirectMatrixNodalTerm{points[index], components[index], reals[index], imaginaries[index]});
    return card;
}

void DirectMatrixAeroKTable::store(std::uint32_t row, const DirectMatrixAeroKCard& card)
{
    m_entryName.set(row, card.entryName);
    m_nodeJ.set(row, card.nodeJ);
    m_componentsJ.set(row, card.componentsJ);
    m_matrixShape.set(row, card.matrixShape);
    m_inputType.set(row, card.inputType);
    m_outputType.set(row, card.outputType);
    m_polarFlag.set(row, card.polarFlag);
    m_columnCount.set(row, card.columnCount);
    m_imaginary.set(row, card.imaginary ? 1 : 0);
    std::vector<base::EntityId> points;
    std::vector<std::int32_t> components;
    std::vector<double> reals;
    std::vector<double> imaginaries;
    for (const DirectMatrixNodalTerm& term : card.terms)
    {
        points.push_back(term.point);
        components.push_back(term.component);
        reals.push_back(term.real);
        imaginaries.push_back(term.imaginary);
    }
    m_point.set(row, points);
    m_component.set(row, components);
    m_real.set(row, reals);
    m_imaginaryPart.set(row, imaginaries);
}

void DirectMatrixAeroKTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_nodeJ.keepRows(rows);
    m_componentsJ.keepRows(rows);
    m_matrixShape.keepRows(rows);
    m_inputType.keepRows(rows);
    m_outputType.keepRows(rows);
    m_polarFlag.keepRows(rows);
    m_columnCount.keepRows(rows);
    m_imaginary.keepRows(rows);
    m_point.keepRows(rows);
    m_component.keepRows(rows);
    m_real.keepRows(rows);
    m_imaginaryPart.keepRows(rows);
}

void DirectMatrixAeroKTable::finishRead(Model&, const ReadContext& context)
{
    const std::string card = std::string(NAME);
    std::set<std::string> headers;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row) || m_nodeJ.get(row) != 0)
            continue;
        const std::string name = m_entryName.get(row);
        if (!headers.insert(name).second && reported.insert(name).second)
            context.sink.error("deck.duplicate-direct-matrix-aero-k",
                               "the " + card + " matrix " + name + " has more than one header card");
    }
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row) || m_nodeJ.get(row) == 0)
            continue;
        const std::string name = m_entryName.get(row);
        if (!headers.contains(name) && reported.insert(name).second)
            context.sink.warning("deck.direct-matrix-aero-k-without-header",
                                 "the " + card + " matrix " + name + " has column cards and no header card");
    }
}

} // namespace oor::model
