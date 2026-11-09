#include "oor/query/apply/QueryApply.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace oor::query
{

namespace
{

std::string cardName(std::uint16_t type)
{
    return std::string(model::CardRegistry::instance().entry(type).info.name);
}

std::uint64_t cardId(std::uint16_t type, std::uint32_t key) noexcept
{
    return (static_cast<std::uint64_t>(type) << 32) | key;
}

// A position edit of one grid point: the axes that were changed, and what they were changed to.
struct PositionEdit
{
    std::size_t card = 0;
    std::array<bool, 3> changed = {false, false, false};
    std::array<double, 3> values = {0.0, 0.0, 0.0};
};

std::string describe(const ApplyProblem& problem)
{
    std::string text = problem.card;
    if (problem.id != 0)
        text += " " + std::to_string(problem.id);
    text += " (key " + std::to_string(problem.key) + ")";
    if (!problem.field.empty())
        text += ", " + problem.field;
    return text + ": " + problem.message;
}

[[noreturn]] void fail(const std::vector<ApplyProblem>& problems, bool stale)
{
    std::string message = stale ? "apply: " + std::to_string(problems.size()) +
                                      " card(s) of the table no longer exist; nothing was written"
                                : "apply: " + std::to_string(problems.size()) + " problem(s); nothing was written";
    for (std::size_t index = 0; index < problems.size() && index < 8; ++index)
        message += "\n  " + describe(problems[index]);
    if (problems.size() > 8)
        message += "\n  ... and " + std::to_string(problems.size() - 8) + " more";
    throw ApplyError(message, problems, stale);
}

} // namespace

ApplyPlan QueryApply::plan(const EditableTable& table, const model::Model& model)
{
    ApplyPlan result;
    const ChangeTracker& tracker = table.tracker();
    const std::vector<std::size_t> columns = tracker.changedColumns();
    result.cells = tracker.changeCount();
    if (columns.empty())
        return result;
    if (!table.hasCards())
        throw ApplyError("apply: the table is not made of cards; there is nothing to write the changes to", {}, false);
    const QueryTable& base = *table.base();
    std::vector<ApplyProblem> problems;

    std::unordered_map<std::uint64_t, std::size_t> index; // card -> position in result.cards
    const auto cardFor = [&](std::uint32_t row) -> std::size_t
    {
        const std::uint16_t type = table.cardTypeAt(row);
        const std::uint32_t key = table.cardKeyAt(row);
        const auto [slot, inserted] = index.emplace(cardId(type, key), result.cards.size());
        if (inserted)
        {
            CardWrite card;
            card.type = type;
            card.key = key;
            card.first = row;
            result.cards.push_back(std::move(card));
        }
        else
            result.cards[slot->second].first = std::min(result.cards[slot->second].first, row);
        return slot->second;
    };
    const auto problemOf = [&](const CardWrite& card, const std::string& field, std::string message)
    {
        ApplyProblem problem;
        problem.card = cardName(card.type);
        problem.id = card.id;
        problem.key = card.key;
        problem.field = field;
        problem.message = std::move(message);
        return problem;
    };

    // The changed cells of the columns that are fields of the cards, and the rows with a changed position.
    std::vector<std::size_t> positionColumns;
    for (const std::size_t column : columns)
    {
        if (table.editability(column).position)
        {
            positionColumns.push_back(column);
            continue;
        }
        const ColumnInfo& info = base.info(column);
        for (const std::uint32_t row : tracker.find(column)->changedRows())
        {
            CardWrite& card = result.cards[cardFor(row)];
            CellValue value = tracker.value(column, row);
            const auto existing = std::find_if(card.fields.begin(), card.fields.end(),
                                               [&](const FieldWrite& write) { return write.field == info.field; });
            if (existing == card.fields.end())
                card.fields.push_back(FieldWrite{info.field, std::move(value), info.name});
            else if (!existing->value.sameAs(value))
                problems.push_back(problemOf(card, info.field,
                                             "rows of the table that are the same card give it two values (" +
                                                 existing->value.toText() + " and " + value.toText() + ")"));
        }
    }

    // Positions: one edit per grid point, whichever of x, y, z and xyzGlobal were changed.
    std::vector<PositionEdit> positionEdits;
    if (!positionColumns.empty())
    {
        std::vector<std::uint8_t> marked(table.rowCount(), 0);
        for (const std::size_t column : positionColumns)
        {
            for (const std::uint32_t row : tracker.find(column)->changedRows())
                marked[row] = 1;
        }
        std::array<std::optional<std::size_t>, 3> axisColumns;
        std::optional<std::size_t> globalColumn;
        for (const std::size_t column : positionColumns)
        {
            const std::string& field = base.info(column).field;
            if (field == "x")
                axisColumns[0] = column;
            else if (field == "y")
                axisColumns[1] = column;
            else if (field == "z")
                axisColumns[2] = column;
            else
                globalColumn = column;
        }
        for (std::size_t row = 0; row < marked.size(); ++row)
        {
            if (marked[row] == 0)
                continue;
            PositionEdit edit;
            edit.card = cardFor(static_cast<std::uint32_t>(row));
            bool usable = true;
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                if (axisColumns[axis] && tracker.isChanged(*axisColumns[axis], row))
                {
                    const CellValue value = tracker.value(*axisColumns[axis], row);
                    if (value.isNull() || !value.isNumber())
                    {
                        problems.push_back(problemOf(result.cards[edit.card], base.info(*axisColumns[axis]).name,
                                                     "a position cannot be null"));
                        usable = false;
                        continue;
                    }
                    edit.changed[axis] = true;
                    edit.values[axis] = value.asReal();
                }
            }
            if (globalColumn && tracker.isChanged(*globalColumn, row))
            {
                const CellValue now = tracker.value(*globalColumn, row);
                const CellValue before = tracker.originalValue(*globalColumn, row);
                if (now.kind() != CellValue::Kind::RealList || now.listSize() != 3)
                {
                    problems.push_back(problemOf(result.cards[edit.card], base.info(*globalColumn).name,
                                                 "a position is three numbers"));
                    usable = false;
                }
                else
                {
                    for (std::size_t axis = 0; axis < 3; ++axis)
                    {
                        const bool differs =
                            before.kind() != CellValue::Kind::RealList || before.listSize() != 3 ||
                            !CellValue::ofReal(now.listReal(axis)).sameAs(CellValue::ofReal(before.listReal(axis)));
                        if (!edit.changed[axis] && differs)
                        {
                            edit.changed[axis] = true;
                            edit.values[axis] = now.listReal(axis);
                        }
                    }
                }
            }
            if (usable && (edit.changed[0] || edit.changed[1] || edit.changed[2]))
                positionEdits.push_back(edit);
        }
    }

    // The cards must still be there.
    std::vector<ApplyProblem> stale;
    for (CardWrite& card : result.cards)
    {
        const auto location = model.locateKey(card.type, base::CardKey{card.key});
        if (!location)
        {
            stale.push_back(problemOf(card, {}, "the card was deleted since the table was made"));
            continue;
        }
        card.row = location->row;
        card.id = model.findTable(card.type)->idAt(location->row);
    }
    if (!stale.empty())
        fail(stale, true);

    // The positions, converted to the CP system of each node.
    if (!positionEdits.empty())
    {
        const model::NodeTable* grids = model.findTable<model::NodeTable>();
        const analysis::BasicPositionCache& cache = analysis::BasicPositionCache::of(model);
        analysis::CoordinateTransform transform(model);
        for (const PositionEdit& edit : positionEdits)
        {
            CardWrite& card = result.cards[edit.card];
            const auto conflicting = std::find_if(card.fields.begin(), card.fields.end(), [](const FieldWrite& write)
                                                  { return write.field == "xyz" || write.field == "positionFrameId"; });
            if (conflicting != card.fields.end())
            {
                problems.push_back(
                    problemOf(card, conflicting->column,
                              "the basic position and the " +
                                  std::string(conflicting->field == "xyz" ? "CP coordinates xyz" : "CP system") +
                                  " of the same grid point were both edited: edit one of them"));
                continue;
            }
            try
            {
                base::Vec3 position = cache.position(card.row);
                for (std::size_t axis = 0; axis < 3; ++axis)
                {
                    if (!edit.changed[axis])
                        continue;
                    (axis == 0 ? position.x : axis == 1 ? position.y : position.z) = edit.values[axis];
                }
                const base::EntityId cp = grids->positionFrameId(card.row);
                const base::Vec3 local = transform.fromBasic(cp, position);
                card.fields.push_back(FieldWrite{"xyz", CellValue::ofReals({local.x, local.y, local.z}), "xyzGlobal"});
                ++result.positions;
            }
            catch (const analysis::CoordinateSystemError& error)
            {
                problems.push_back(problemOf(card, "xyzGlobal", error.what()));
            }
        }
    }
    if (!problems.empty())
        fail(problems, false);

    std::sort(result.cards.begin(), result.cards.end(), [](const CardWrite& left, const CardWrite& right)
              { return left.first != right.first ? left.first < right.first : left.key < right.key; });
    for (const CardWrite& card : result.cards)
        result.fieldWrites += card.fields.size();
    return result;
}

} // namespace oor::query
