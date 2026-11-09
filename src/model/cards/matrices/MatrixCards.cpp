#include "oor/model/cards/matrices/MatrixCards.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"
#include "oor/model/cards/matrices/directMatrix/DirectMatrixTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroJ/DirectMatrixAeroJTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroJInterference/DirectMatrixAeroJInterferenceTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroK/DirectMatrixAeroKTable.hpp"
#include "oor/model/cards/matrices/directMatrixAxisym/DirectMatrixAxisymTable.hpp"
#include "oor/model/cards/matrices/directMatrixNodal/DirectMatrixNodalTable.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <utility>

namespace oor::model
{

namespace
{

// A degree of freedom of a matrix: a point, a component and, for DMIAX, a harmonic (0 for the other cards). The order
// is the one the degrees of freedom are numbered in.
struct Dof
{
    base::EntityId point = 0;
    std::int32_t component = 0;
    std::int32_t harmonic = 0;

    [[nodiscard]] auto operator<=>(const Dof&) const noexcept = default;
};

template <typename TableT>
std::vector<std::string> headerNames(const Model& model)
{
    std::vector<std::string> names;
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return names;
    std::set<std::string> seen;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        auto card = table->card(row);
        if (card.isHeader() && seen.insert(card.entryName).second)
            names.push_back(std::move(card.entryName));
    }
    return names;
}

// The position of `value` in the sorted list `list`.
std::int32_t positionOf(const std::vector<Dof>& list, const Dof& value)
{
    const auto found = std::lower_bound(list.begin(), list.end(), value);
    return static_cast<std::int32_t>(found - list.begin());
}

std::vector<Dof> sortedUnique(std::vector<Dof> list)
{
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    return list;
}

// The terms of a matrix of the DMIG family (DMIG, DMIJ, DMIJI, DMIK: one layout, one term type) from its header and
// its column cards in the order of the deck: an amplitude and phase matrix converted to real and imaginary parts.
template <typename CardT>
LabelledTerms pointTermsOfCards(const CardT& header, const std::vector<CardT>& columns)
{
    LabelledTerms terms;
    terms.form = header.matrixShape;
    terms.inputType = header.inputType;
    terms.outputType = header.outputType;
    terms.polar = header.polarFlag;
    terms.columnCount = header.columnCount;
    terms.complex = header.inputType >= 3;
    for (const CardT& card : columns)
    {
        for (const DirectMatrixNodalTerm& term : card.terms)
        {
            terms.columnId.push_back(card.nodeJ);
            terms.columnComponent.push_back(card.componentsJ);
            terms.rowId.push_back(term.point);
            terms.rowComponent.push_back(term.component);
            double real = term.real;
            double imag = term.imaginary;
            if (terms.complex && terms.polar > 0)
            {
                const double radians = term.imaginary * std::numbers::pi / 180.0;
                real = term.real * std::cos(radians);
                imag = term.real * std::sin(radians);
            }
            terms.real.push_back(real);
            if (terms.complex)
                terms.imag.push_back(imag);
        }
    }
    return terms;
}

// The terms of the matrix `requested` of a table of the DMIG family: the first header of that name, then every column
// card of that name in the order of the deck.
template <typename TableT>
std::optional<LabelledTerms> pointTermsOf(const Model& model, std::string_view requested)
{
    using CardT = typename TableT::CardType;
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return std::nullopt;
    const std::string name = matrixNameText(requested);
    std::optional<CardT> header;
    std::vector<CardT> columns;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        CardT card = table->card(row);
        if (card.entryName != name)
            continue;
        if (!card.isHeader())
            columns.push_back(std::move(card));
        else if (!header)
            header = std::move(card);
    }
    if (!header)
        return std::nullopt;
    return pointTermsOfCards(*header, columns);
}

// The coordinate form of labelled terms, for any matrix of points. IFO 1 and 6 are square: rows and columns on one
// list. IFO 9 with NCOL numbers the columns 1 to NCOL by GJ. The others sort rows and columns apart.
CooMatrix pointCoo(std::string_view cardType, std::string_view name, const LabelledTerms& terms)
{
    CooMatrix matrix;
    matrix.name = matrixNameText(name);
    matrix.card = std::string(cardType);
    matrix.form = terms.form;
    matrix.inputType = terms.inputType;
    matrix.outputType = terms.outputType;
    matrix.complex = terms.complex;
    matrix.polar = terms.polar > 0;
    const std::size_t count = terms.termCount();
    const bool harmonics = !terms.rowHarmonic.empty() || !terms.columnHarmonic.empty();
    std::vector<Dof> rowDofs;
    std::vector<Dof> columnDofs;
    rowDofs.reserve(count);
    columnDofs.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        rowDofs.push_back({terms.rowId[index], terms.rowComponent[index], harmonics ? terms.rowHarmonic[index] : 0});
        columnDofs.push_back(
            {terms.columnId[index], terms.columnComponent[index], harmonics ? terms.columnHarmonic[index] : 0});
    }
    const bool square = terms.form == 1 || terms.form == 6;
    const bool numberedColumns = terms.form == 9 && terms.columnCount > 0;
    std::vector<Dof> rows;
    std::vector<Dof> columns;
    if (square)
    {
        std::vector<Dof> both = rowDofs;
        both.insert(both.end(), columnDofs.begin(), columnDofs.end());
        rows = sortedUnique(std::move(both));
        columns = rows;
    }
    else
    {
        rows = sortedUnique(rowDofs);
        if (numberedColumns)
        {
            std::int32_t highest = terms.columnCount;
            for (const Dof& dof : columnDofs)
                highest = std::max(highest, dof.point);
            if (highest > terms.columnCount)
                throw base::UsageError(std::string(cardType) + " " + matrix.name + ": the column " +
                                       std::to_string(highest) + " is above " + std::string("columnCount") + " = " +
                                       std::to_string(terms.columnCount));
            for (std::int32_t column = 1; column <= terms.columnCount; ++column)
                columns.push_back({column, 0, 0});
        }
        else
        {
            columns = sortedUnique(columnDofs);
        }
    }
    matrix.rowCount = static_cast<std::int64_t>(rows.size());
    matrix.columnCount = static_cast<std::int64_t>(columns.size());
    matrix.row.reserve(count);
    matrix.column.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        matrix.row.push_back(positionOf(rows, rowDofs[index]));
        matrix.column.push_back(numberedColumns ? columnDofs[index].point - 1 : positionOf(columns, columnDofs[index]));
    }
    matrix.real = terms.real;
    matrix.imag = terms.imag;
    for (const Dof& dof : rows)
    {
        matrix.rowId.push_back(dof.point);
        matrix.rowComponent.push_back(dof.component);
        if (harmonics)
            matrix.rowHarmonic.push_back(dof.harmonic);
    }
    for (const Dof& dof : columns)
    {
        matrix.columnId.push_back(dof.point);
        matrix.columnComponent.push_back(dof.component);
        if (harmonics)
            matrix.columnHarmonic.push_back(dof.harmonic);
    }
    return matrix;
}

[[noreturn]] void refuse(std::string_view cardType, std::string_view name, const std::vector<std::string>& problems)
{
    std::string message = "cannot store " + std::string(cardType) + " " + std::string(name) + ":";
    for (const std::string& problem : problems)
        message += "\n  " + problem;
    throw base::UsageError(message);
}

// The cards of a matrix of the DMIG family for `terms`: the header, then one column card for each column in the order
// the columns first appear, with its terms in the order given.
template <typename CardT, typename FormatT>
std::vector<CardT> makePointCards(std::string_view cardType, std::string_view requested, const LabelledTerms& terms)
{
    const std::string name = matrixNameText(requested);
    std::vector<std::string> problems;
    const std::size_t count = terms.termCount();
    if (terms.columnId.size() != count || terms.columnComponent.size() != count || terms.rowId.size() != count ||
        terms.rowComponent.size() != count)
        problems.push_back("the labels of the rows and columns and the values must have one entry per term");
    if (terms.complex ? terms.imag.size() != count : !terms.imag.empty())
        problems.push_back(terms.complex ? "a complex matrix needs one imaginary part per term"
                                         : "a real matrix has no imaginary parts");
    if (terms.polar != 0)
        problems.push_back("a matrix stored from real and imaginary parts has " + std::string("polarFlag") + " blank");
    if (terms.complex ? (terms.inputType < 3 || terms.inputType > 4) : (terms.inputType < 1 || terms.inputType > 2))
        problems.push_back(std::string("inputType") +
                           (terms.complex ? " of a complex matrix is 3 or 4" : " of a real matrix is 1 or 2"));
    if (terms.complex ? (terms.outputType != 0 && terms.outputType != 3 && terms.outputType != 4)
                      : (terms.outputType != 0 && terms.outputType != 1 && terms.outputType != 2))
        problems.push_back(std::string("outputType") +
                           (terms.complex ? " of a complex matrix is 0, 3 or 4" : " of a real matrix is 0, 1 or 2"));
    if (!problems.empty())
        refuse(cardType, name, problems);
    CardT header;
    header.entryName = name;
    header.matrixShape = terms.form;
    header.inputType = terms.inputType;
    header.outputType = terms.outputType;
    header.columnCount = terms.columnCount;
    std::vector<CardT> cards;
    cards.push_back(header);
    std::map<Dof, std::size_t> columnPosition;
    std::set<std::pair<Dof, Dof>> seen;
    for (std::size_t index = 0; index < count; ++index)
    {
        const Dof column{terms.columnId[index], terms.columnComponent[index], 0};
        const Dof row{terms.rowId[index], terms.rowComponent[index], 0};
        const std::pair<Dof, Dof> key =
            terms.form == 6 && row < column ? std::pair{column, row} : std::pair{row, column};
        if (!seen.insert(key).second)
            problems.push_back("the element (" + std::to_string(row.point) + "," + std::to_string(row.component) +
                               ") (" + std::to_string(column.point) + "," + std::to_string(column.component) +
                               ") is given twice" +
                               (terms.form == 6 ? " (a symmetric matrix has each element once, "
                                                  "above or below the diagonal)"
                                                : ""));
        auto [position, inserted] = columnPosition.try_emplace(column, cards.size());
        if (inserted)
        {
            CardT card;
            card.entryName = name;
            card.nodeJ = column.point;
            card.componentsJ = column.component;
            card.imaginary = terms.complex;
            cards.push_back(std::move(card));
        }
        cards[position->second].terms.push_back(DirectMatrixNodalTerm{row.point, row.component, terms.real[index],
                                                                      terms.complex ? terms.imag[index] : 0.0});
    }
    for (const CardT& card : cards)
    {
        if (const auto problem = FormatT::check(card))
        {
            problems.push_back(std::string(problem->field) + ": " + problem->text());
            break;
        }
    }
    if (!problems.empty())
        refuse(cardType, name, problems);
    return cards;
}

template <typename TableT, typename FormatT>
CardLocation addPointCards(Model& model, std::string_view cardType, std::string_view name, const LabelledTerms& terms)
{
    const std::vector<std::string> existing = headerNames<TableT>(model);
    const std::string wanted = matrixNameText(name);
    if (std::find(existing.begin(), existing.end(), wanted) != existing.end())
        throw base::UsageError("the model has a " + std::string(cardType) + " matrix named " + wanted +
                               " already (remove the cards of that matrix first)");
    const auto cards = makePointCards<typename TableT::CardType, FormatT>(cardType, name, terms);
    std::optional<CardLocation> header;
    for (const auto& card : cards)
    {
        const CardLocation location = model.add<TableT>(card);
        if (!header)
            header = location;
    }
    return *header;
}

template <typename TableT, typename NameOf>
std::size_t removeCards(Model& model, std::string_view requested, NameOf nameOf)
{
    TableT* table = model.findTable(typeIndexOf<TableT>()) != nullptr ? &model.table<TableT>() : nullptr;
    if (table == nullptr)
        return 0;
    const std::string name = matrixNameText(requested);
    std::size_t removed = 0;
    const std::uint32_t rows = static_cast<std::uint32_t>(table->rows().rowCount());
    for (std::uint32_t row = 0; row < rows; ++row)
    {
        if (table->rows().isAlive(row) && nameOf(*table, row) == name)
        {
            model.remove(CardLocation{typeIndexOf<TableT>(), row});
            ++removed;
        }
    }
    return removed;
}

template <typename TableT>
std::size_t removeMatrixCards(Model& model, std::string_view name)
{
    return removeCards<TableT>(model, name,
                               [](TableT& table, std::uint32_t row) { return table.entryNameColumn().get(row); });
}

// The header card and the column cards of every matrix of a table, in the order of the headers: the first header of a
// name, its columns in the order of the deck (before or after the header); the columns of a name without a header are
// left out. One pass over the cards.
template <typename TableT>
std::vector<std::pair<typename TableT::CardType, std::vector<typename TableT::CardType>>> groupedByName(
    const Model& model)
{
    using CardT = typename TableT::CardType;
    std::vector<std::pair<CardT, std::vector<CardT>>> groups;
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return groups;
    std::map<std::string, std::vector<CardT>> columnsOf;
    std::map<std::string, std::size_t> groupOf;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        CardT card = table->card(row);
        if (!card.isHeader())
            columnsOf[card.entryName].push_back(std::move(card));
        else if (groupOf.try_emplace(card.entryName, groups.size()).second)
            groups.emplace_back(std::move(card), std::vector<CardT>());
    }
    for (auto& [header, columns] : groups)
    {
        if (const auto found = columnsOf.find(header.entryName); found != columnsOf.end())
            columns = std::move(found->second);
    }
    return groups;
}

// Every matrix of a table of the DMIG family in the coordinate form of pointCoo, in the order of the headers.
// True when `names` (upper case) is empty or holds `name`.
bool isAsked(const std::vector<std::string>& names, const std::string& name)
{
    return names.empty() || std::find(names.begin(), names.end(), name) != names.end();
}

template <typename TableT>
std::vector<CooMatrix> pointMatricesOf(const Model& model, const std::vector<std::string>& names)
{
    std::vector<CooMatrix> matrices;
    for (const auto& [header, columns] : groupedByName<TableT>(model))
    {
        if (isAsked(names, header.entryName))
            matrices.push_back(pointCoo(TableT::NAME, header.entryName, pointTermsOfCards(header, columns)));
    }
    return matrices;
}

// The coordinate form of a DMI matrix from its header and its column cards in the order of the deck (directMatrixCoo).
CooMatrix directMatrixOfCards(const DirectMatrixCard& header, const std::vector<DirectMatrixCard>& columns)
{
    const std::string& name = header.entryName;
    CooMatrix matrix;
    matrix.name = name;
    matrix.card = std::string(DirectMatrixTable::NAME);
    matrix.form = header.matrixForm;
    matrix.inputType = header.inputType;
    matrix.outputType = header.outputType;
    matrix.complex = header.inputType >= 3;
    matrix.rowCount = header.rows;
    matrix.columnCount = header.matrixForm == 3 ? 1 : (header.matrixForm == 8 ? header.rows : header.columns);
    const int step = matrix.complex ? 2 : 1;
    const auto fail = [&](const std::string& what)
    { throw base::UsageError(std::string(DirectMatrixTable::NAME) + " " + name + ": " + what); };
    std::set<std::int32_t> given;
    for (const DirectMatrixCard& card : columns)
    {
        const std::string column = "column " + std::to_string(card.columnIndex);
        if (card.columnIndex > matrix.columnCount)
            fail("the column " + std::to_string(card.columnIndex) + " is beyond " + std::string("columnCount") + " = " +
                 std::to_string(matrix.columnCount));
        if (!given.insert(card.columnIndex).second)
            fail(column + " is given on more than one card (a column is one logical entry)");
        std::int32_t current = card.firstRow; // the row of the next element
        std::vector<double> pending;
        bool haveLast = false;
        double lastReal = 0.0;
        double lastImag = 0.0;
        const auto emit = [&](double real, double imag)
        {
            if (current > header.rows)
                fail("the row " + std::to_string(current) + " of " + column + " is beyond " + std::string("rowCount") +
                     " = " + std::to_string(header.rows));
            matrix.row.push_back(current - 1);
            matrix.column.push_back(card.columnIndex - 1);
            matrix.real.push_back(real);
            if (matrix.complex)
                matrix.imag.push_back(imag);
            ++current;
        };
        for (const DirectMatrixToken& token : card.tokens)
        {
            switch (token.kind)
            {
            case DirectMatrixTokenKind::Value:
                pending.push_back(token.value);
                if (static_cast<int>(pending.size()) == step)
                {
                    lastReal = pending[0];
                    lastImag = matrix.complex ? pending[1] : 0.0;
                    haveLast = true;
                    emit(lastReal, lastImag);
                    pending.clear();
                }
                break;
            case DirectMatrixTokenKind::Row:
                if (!pending.empty())
                    fail(column + " gives the real part of an element and no imaginary part");
                if (token.row < current)
                    fail("the rows of " + column + " go back to row " + std::to_string(token.row) + " where row " +
                         std::to_string(current) + " is next (they are given in increasing order)");
                current = token.row;
                break;
            case DirectMatrixTokenKind::Thru:
                if (!haveLast || !pending.empty())
                    fail("THRU in " + column + " does not follow a complete element");
                if (token.row < current - 1)
                    fail("THRU in " + column + " ends at row " + std::to_string(token.row) +
                         ", above the row of its element " + std::to_string(current - 1));
                while (current <= token.row)
                    emit(lastReal, lastImag);
                break;
            }
        }
        if (!pending.empty())
            fail(column + " ends with the real part of an element and no imaginary part");
    }
    return matrix;
}

} // namespace

std::vector<std::string> directMatrixNodalNames(const Model& model)
{
    return headerNames<DirectMatrixNodalTable>(model);
}

std::vector<std::string> directMatrixNames(const Model& model)
{
    return headerNames<DirectMatrixTable>(model);
}

std::optional<LabelledTerms> directMatrixNodalTerms(const Model& model, std::string_view requested)
{
    return pointTermsOf<DirectMatrixNodalTable>(model, requested);
}

CooMatrix directMatrixNodalCoo(std::string_view name, const LabelledTerms& terms)
{
    return pointCoo(DirectMatrixNodalTable::NAME, name, terms);
}

std::optional<CooMatrix> directMatrixCoo(const Model& model, std::string_view requested)
{
    const DirectMatrixTable* table = model.findTable<DirectMatrixTable>();
    if (table == nullptr)
        return std::nullopt;
    const std::string name = matrixNameText(requested);
    std::optional<DirectMatrixCard> header;
    std::vector<DirectMatrixCard> columns;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        DirectMatrixCard card = table->card(row);
        if (card.entryName != name)
            continue;
        if (!card.isHeader())
            columns.push_back(std::move(card));
        else if (!header)
            header = std::move(card);
    }
    if (!header)
        return std::nullopt;
    return directMatrixOfCards(*header, columns);
}

std::vector<CooMatrix> directMatrices(const Model& model, const std::vector<std::string>& names)
{
    std::vector<CooMatrix> matrices;
    for (const auto& [header, columns] : groupedByName<DirectMatrixTable>(model))
    {
        if (isAsked(names, header.entryName))
            matrices.push_back(directMatrixOfCards(header, columns));
    }
    return matrices;
}

std::vector<CooMatrix> pointMatrices(const Model& model, PointMatrixCard kind, const std::vector<std::string>& names)
{
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return pointMatricesOf<DirectMatrixNodalTable>(model, names);
    case PointMatrixCard::AeroJ:
        return pointMatricesOf<DirectMatrixAeroJTable>(model, names);
    case PointMatrixCard::AeroJInterference:
        return pointMatricesOf<DirectMatrixAeroJInterferenceTable>(model, names);
    case PointMatrixCard::AeroK:
        return pointMatricesOf<DirectMatrixAeroKTable>(model, names);
    }
    return {};
}

std::string_view matrixFormName(std::int32_t form, std::string_view card) noexcept
{
    if (card != DirectMatrixTable::NAME)
    {
        // the matrices at points (DMIG, DMIJ, DMIJI, DMIK, DMIAX): IFO 1 square, 6 symmetric, 2 or 9 rectangular
        switch (form)
        {
        case 1:
            return "square";
        case 6:
            return "symmetric";
        default:
            return "rectangular";
        }
    }
    switch (form)
    {
    case 1:
        return "square";
    case 2:
        return "rectangular";
    case 3:
        return "diagonal";
    case 4:
        return "lower triangular factor";
    case 5:
        return "upper triangular factor";
    case 6:
        return "symmetric";
    case 8:
        return "identity";
    default:
        return "unknown";
    }
}

CooMatrix madeMatrix(const CooMatrix& stored)
{
    CooMatrix matrix = stored;
    if (stored.card == DirectMatrixTable::NAME)
    {
        if (stored.form == 3)
        {
            // a diagonal matrix is given as its column of M elements (format reference DMI: FORM 3 has N = 1, made a
            // FORM 6 one)
            matrix.columnCount = stored.rowCount;
            matrix.column = stored.row;
        }
        else if (stored.form == 8)
        {
            // an identity has no element on its cards: its M ones
            const auto size = static_cast<std::int32_t>(stored.rowCount);
            matrix.columnCount = stored.rowCount;
            matrix.row.clear();
            matrix.column.clear();
            matrix.real.assign(static_cast<std::size_t>(size), 1.0);
            matrix.imag.assign(stored.complex ? static_cast<std::size_t>(size) : 0U, 0.0);
            for (std::int32_t index = 0; index < size; ++index)
            {
                matrix.row.push_back(index);
                matrix.column.push_back(index);
            }
        }
        return matrix;
    }
    if (stored.form != 6)
        return matrix;
    // a symmetric matrix at points: either triangle is given, never one element both
    // above and below the diagonal; the mirror of each term off the diagonal follows the terms given
    std::set<std::pair<std::int32_t, std::int32_t>> elements;
    for (std::size_t index = 0; index < stored.termCount(); ++index)
    {
        const std::int32_t row = stored.row[index];
        const std::int32_t column = stored.column[index];
        if (row == column)
            continue;
        if (!elements.insert(row < column ? std::pair{column, row} : std::pair{row, column}).second)
            throw base::UsageError(std::string(stored.card) + " " + stored.name +
                                   ": the element of the degrees of freedom " + std::to_string(row) + " and " +
                                   std::to_string(column) +
                                   " (from 0) is given twice, above and below the diagonal of a symmetric matrix");
    }
    for (std::size_t index = 0; index < stored.termCount(); ++index)
    {
        if (stored.row[index] == stored.column[index])
            continue;
        matrix.row.push_back(stored.column[index]);
        matrix.column.push_back(stored.row[index]);
        matrix.real.push_back(stored.real[index]);
        if (stored.complex)
            matrix.imag.push_back(stored.imag[index]);
    }
    return matrix;
}

std::vector<DirectMatrixNodalCard> makeDirectMatrixNodalCards(std::string_view requested, const LabelledTerms& terms)
{
    return makePointCards<DirectMatrixNodalCard, DirectMatrixNodalFormat>(DirectMatrixNodalTable::NAME, requested,
                                                                          terms);
}

CardLocation addDirectMatrixNodal(Model& model, std::string_view name, const LabelledTerms& terms)
{
    return addPointCards<DirectMatrixNodalTable, DirectMatrixNodalFormat>(model, DirectMatrixNodalTable::NAME, name,
                                                                          terms);
}

std::size_t removeDirectMatrixNodal(Model& model, std::string_view name)
{
    return removeMatrixCards<DirectMatrixNodalTable>(model, name);
}

std::size_t removeDirectMatrix(Model& model, std::string_view name)
{
    return removeMatrixCards<DirectMatrixTable>(model, name);
}

std::string_view pointMatrixCardName(PointMatrixCard kind) noexcept
{
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return DirectMatrixNodalTable::NAME;
    case PointMatrixCard::AeroJ:
        return DirectMatrixAeroJTable::NAME;
    case PointMatrixCard::AeroJInterference:
        return DirectMatrixAeroJInterferenceTable::NAME;
    case PointMatrixCard::AeroK:
        return DirectMatrixAeroKTable::NAME;
    }
    return DirectMatrixNodalTable::NAME;
}

std::vector<std::string> pointMatrixNames(const Model& model, PointMatrixCard kind)
{
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return headerNames<DirectMatrixNodalTable>(model);
    case PointMatrixCard::AeroJ:
        return headerNames<DirectMatrixAeroJTable>(model);
    case PointMatrixCard::AeroJInterference:
        return headerNames<DirectMatrixAeroJInterferenceTable>(model);
    case PointMatrixCard::AeroK:
        return headerNames<DirectMatrixAeroKTable>(model);
    }
    return {};
}

std::optional<LabelledTerms> pointMatrixTerms(const Model& model, PointMatrixCard kind, std::string_view name)
{
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return pointTermsOf<DirectMatrixNodalTable>(model, name);
    case PointMatrixCard::AeroJ:
        return pointTermsOf<DirectMatrixAeroJTable>(model, name);
    case PointMatrixCard::AeroJInterference:
        return pointTermsOf<DirectMatrixAeroJInterferenceTable>(model, name);
    case PointMatrixCard::AeroK:
        return pointTermsOf<DirectMatrixAeroKTable>(model, name);
    }
    return std::nullopt;
}

CooMatrix pointMatrixCoo(PointMatrixCard kind, std::string_view name, const LabelledTerms& terms)
{
    return pointCoo(pointMatrixCardName(kind), name, terms);
}

CardLocation addPointMatrix(Model& model, PointMatrixCard kind, std::string_view name, const LabelledTerms& terms)
{
    const std::string_view cardName = pointMatrixCardName(kind);
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return addPointCards<DirectMatrixNodalTable, DirectMatrixNodalFormat>(model, cardName, name, terms);
    case PointMatrixCard::AeroJ:
        return addPointCards<DirectMatrixAeroJTable, DirectMatrixAeroJFormat>(model, cardName, name, terms);
    case PointMatrixCard::AeroJInterference:
        return addPointCards<DirectMatrixAeroJInterferenceTable, DirectMatrixAeroJInterferenceFormat>(model, cardName,
                                                                                                      name, terms);
    case PointMatrixCard::AeroK:
        return addPointCards<DirectMatrixAeroKTable, DirectMatrixAeroKFormat>(model, cardName, name, terms);
    }
    throw base::UsageError("unknown matrix card");
}

std::size_t removePointMatrix(Model& model, PointMatrixCard kind, std::string_view name)
{
    switch (kind)
    {
    case PointMatrixCard::Nodal:
        return removeMatrixCards<DirectMatrixNodalTable>(model, name);
    case PointMatrixCard::AeroJ:
        return removeMatrixCards<DirectMatrixAeroJTable>(model, name);
    case PointMatrixCard::AeroJInterference:
        return removeMatrixCards<DirectMatrixAeroJInterferenceTable>(model, name);
    case PointMatrixCard::AeroK:
        return removeMatrixCards<DirectMatrixAeroKTable>(model, name);
    }
    return 0;
}

std::vector<std::string> directMatrixAxisymNames(const Model& model)
{
    return headerNames<DirectMatrixAxisymTable>(model);
}

std::optional<LabelledTerms> directMatrixAxisymTerms(const Model& model, std::string_view requested)
{
    const DirectMatrixAxisymTable* table = model.findTable<DirectMatrixAxisymTable>();
    if (table == nullptr)
        return std::nullopt;
    const std::string name = matrixNameText(requested);
    std::optional<LabelledTerms> terms;
    for (std::uint32_t row = 0; row < table->rows().rowCount() && !terms; ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const DirectMatrixAxisymCard card = table->card(row);
        if (card.entryName != name || !card.isHeader())
            continue;
        LabelledTerms found;
        found.form = card.matrixShape;
        found.inputType = card.inputType;
        found.outputType = card.outputType;
        found.complex = card.inputType >= 3;
        terms = std::move(found);
    }
    if (!terms)
        return std::nullopt;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const DirectMatrixAxisymCard card = table->card(row);
        if (card.entryName != name || card.isHeader())
            continue;
        for (const DirectMatrixAxisymTerm& term : card.terms)
        {
            terms->columnId.push_back(card.nodeJ);
            terms->columnComponent.push_back(card.componentsJ);
            terms->columnHarmonic.push_back(card.harmonicJ);
            terms->rowId.push_back(term.point);
            terms->rowComponent.push_back(term.component);
            terms->rowHarmonic.push_back(term.harmonic);
            terms->real.push_back(term.real);
            if (terms->complex)
                terms->imag.push_back(term.imaginary);
        }
    }
    return terms;
}

CooMatrix directMatrixAxisymCoo(std::string_view name, const LabelledTerms& terms)
{
    return pointCoo(DirectMatrixAxisymTable::NAME, name, terms);
}

std::size_t removeDirectMatrixAxisym(Model& model, std::string_view name)
{
    return removeMatrixCards<DirectMatrixAxisymTable>(model, name);
}

} // namespace oor::model
