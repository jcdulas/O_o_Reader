#include "oor/query/providers/quantities/MonitorPointLoads.hpp"

#include "../../named/topology/t10_aero/AeroQuerySupport.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Table.hpp"
#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"
#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Table.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/providers/quantities/NodePointForceSet.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace oor::query
{

bool MonitorRequest::wants(const std::string& name, const std::string& card) const
{
    const auto listed = [](const std::vector<std::string>& list, const std::string& value)
    {
        return list.empty() || std::any_of(list.begin(), list.end(), [&](const std::string& item)
                                           { return ((base::trimAscii(item)) == (value)); });
    };
    return listed(names, name) && listed(cards, card);
}

void MonitorLoadSet::beginProvider(std::int16_t index, const ProviderInfo& info)
{
    m_current = index;
    if (m_providers.size() <= static_cast<std::size_t>(index))
        m_providers.resize(static_cast<std::size_t>(index) + 1);
    m_providers[static_cast<std::size_t>(index)] = info;
}

bool MonitorLoadSet::covered(std::size_t set, const std::string& name, std::int32_t subcase,
                             std::int32_t designCycle) const
{
    const auto found = m_owner.find(Key{set, name, subcase, designCycle});
    return found != m_owner.end() && found->second != m_current;
}

void MonitorLoadSet::add(MonitorLoadRow&& row)
{
    const Key key{row.set, row.name, row.subcase, row.designCycle};
    const auto [found, inserted] = m_owner.emplace(key, m_current);
    if (!inserted && found->second != m_current)
        return;
    row.provider = m_current;
    m_rows.push_back(std::move(row));
}

namespace
{

// The digits of an AXES or a component field (123456, 345) as the components 0 to 5.
std::vector<std::size_t> digitsOf(std::int64_t components)
{
    std::vector<std::size_t> digits;
    for (const char character : std::to_string(components))
        if (character >= '1' && character <= '6')
            digits.push_back(static_cast<std::size_t>(character - '1'));
    return digits;
}

// The card type of a card the stored table names: the table names it by its keyword ("MONPNT1"), the query by its
// card type ("MonitorIntegratedLoad1"), like the other queries.
std::string cardTypeOf(const std::string& keyword)
{
    return keyword;
}

// The monitor point cards of the model by name, with their card type.
std::map<std::string, std::string> monitorCardsOf(const model::Model& model)
{
    std::map<std::string, std::string> cards;
    const auto addAll = [&](const auto* table, std::string_view card)
    {
        if (table == nullptr)
            return;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            if (table->rows().isAlive(row))
                cards.emplace(table->card(row).name, std::string(card));
    };
    addAll(model.findTable<model::MonitorIntegratedLoad1Table>(), model::MonitorIntegratedLoad1Table::NAME);
    addAll(model.findTable<model::MonitorElemLoad2Table>(), model::MonitorElemLoad2Table::NAME);
    addAll(model.findTable<model::MonitorNodeForceSum3Table>(), model::MonitorNodeForceSum3Table::NAME);
    return cards;
}

// A row of a MONPNT3 load matrix that has no directory: the card and the component.
struct MonitorNodeForceSum3Row
{
    const model::MonitorNodeForceSum3Card* card = nullptr;
    std::size_t component = 0; // 0 to 5
};

// The rows of a MONPNT3 matrix: the MONPNT3 cards in the order of the deck, as many rows per card as its AXES has
// digits (the order the solver lists them in, an assumption that no result with several MONPNT3 has confirmed yet).
std::vector<MonitorNodeForceSum3Row> monitorNodeForceSum3Rows(const std::vector<model::MonitorNodeForceSum3Card>& cards)
{
    std::vector<MonitorNodeForceSum3Row> rows;
    for (const model::MonitorNodeForceSum3Card& card : cards)
        for (const std::size_t digit : digitsOf(card.outputAxes))
            rows.push_back(MonitorNodeForceSum3Row{&card, digit});
    return rows;
}

std::vector<model::MonitorNodeForceSum3Card> monitorNodeForceSum3Cards(const model::Model& model)
{
    std::vector<model::MonitorNodeForceSum3Card> cards;
    if (const auto* table = model.findTable<model::MonitorNodeForceSum3Table>())
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            if (table->rows().isAlive(row))
                cards.push_back(table->card(row));
    return cards;
}

// The step values of a named result that the call keeps (`freqs` for a frequency response, `times` for a transient
// run): the values given (to a relative 1e-6: the steps are single precision), or a slice of the positions of its
// distinct steps in increasing order.
std::set<std::uint64_t> selectedSteps(const std::vector<double>& steps, const std::optional<StepSelection>& selection)
{
    std::vector<double> distinct;
    for (const double value : steps)
        if (!std::isnan(value))
            distinct.push_back(value);
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    std::set<std::uint64_t> kept;
    if (!selection)
    {
        for (const double value : distinct)
            kept.insert(stepKey(value));
        return kept;
    }
    if (selection->isSlice)
    {
        const auto count = static_cast<std::int64_t>(distinct.size());
        const auto bound = [&](std::optional<std::int64_t> value, std::int64_t fallback)
        {
            if (!value)
                return fallback;
            const std::int64_t position = *value < 0 ? *value + count : *value;
            return std::clamp<std::int64_t>(position, -1, count);
        };
        const std::int64_t stride = selection->step == 0 ? 1 : selection->step;
        if (stride > 0)
        {
            for (std::int64_t position = std::max<std::int64_t>(bound(selection->start, 0), 0);
                 position < bound(selection->stop, count); position += stride)
                kept.insert(stepKey(distinct[static_cast<std::size_t>(position)]));
        }
        else
        {
            for (std::int64_t position = std::min(bound(selection->start, count - 1), count - 1);
                 position > bound(selection->stop, -1) && position >= 0; position += stride)
                kept.insert(stepKey(distinct[static_cast<std::size_t>(position)]));
        }
        return kept;
    }
    for (const double value : distinct)
        if (std::any_of(selection->values.begin(), selection->values.end(), [&](double wanted)
                        { return std::fabs(value - wanted) <= 1.0e-6 * std::max(1.0, std::fabs(wanted)); }))
            kept.insert(stepKey(value));
    return kept;
}

// ---- the stored monitor point table
// --------------------------------------------------------------------------------------

// The named result `monitorPointLoads`: a row of six components per point, side, contribution, subcase and step. The
// totals are added: TOTAL AERO = flexible increment + gust, and on the structure TOTAL = inertial + external + flexible
// increment + gust.
class StoredMonitorProvider final : public MonitorProvider
{
public:
    [[nodiscard]] std::string name() const override { return "stored monitor point table"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const MonitorRequest& request,
                                  MonitorLoadSet& sink) const override
    {
        const std::vector<ResultQueryContext::NamedRef> named = context.named("monitorPointLoads");
        if (named.empty())
            return {0, "the results hold no monitor point table (the named result `monitorPointLoads`)"};
        Outcome outcome;
        for (const ResultQueryContext::NamedRef& ref : named)
            outcome.rows += addResult(context, ref, request, sink);
        if (outcome.rows == 0)
            outcome.reason = "the monitor point table has no point the call asks for";
        return outcome;
    }

private:
    struct Group
    {
        MonitorLoadRow row;
        std::string label;
    };

    static std::int64_t addResult(const ResultQueryContext& context, const ResultQueryContext::NamedRef& ref,
                                  const MonitorRequest& request, MonitorLoadSet& sink)
    {
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* subcase = data->find("subcase");
        const results::ResultArray* step = data->find("step");
        const results::ResultArray* component = data->find("component");
        const results::ResultArray* positionFrame = data->find("positionFrameId");
        const results::ResultArray* point = data->find("point");
        const results::ResultArray* displacementFrame = data->find("displacementFrameId");
        const results::ResultArray* value = data->find("value");
        const results::ResultArray* matrixRow = data->find("matrixRow");
        const results::ResultText* names = data->findText("name");
        const results::ResultText* cards = data->findText("card");
        const results::ResultText* sides = data->findText("side");
        const results::ResultText* contributions = data->findText("contribution");
        if (subcase == nullptr || component == nullptr || value == nullptr || names == nullptr ||
            contributions == nullptr)
            return 0;
        const std::size_t rows = subcase->valueCount();
        const bool complexValues =
            value->type == results::ArrayType::Complex64 || value->type == results::ArrayType::Complex128;
        const bool single = value->type == results::ArrayType::Float32 || value->type == results::ArrayType::Complex64;
        const auto number = [&](std::size_t index)
        { return single ? static_cast<double>(value->values<float>()[index]) : value->values<double>()[index]; };
        const auto integer = [](const results::ResultArray* array, std::size_t row) -> std::int64_t
        {
            if (array == nullptr)
                return 0;
            return array->type == results::ArrayType::Int64 ? array->values<std::int64_t>()[row]
                                                            : array->values<std::int32_t>()[row];
        };
        const auto real = [](const results::ResultArray* array, std::size_t index) -> double
        {
            if (array == nullptr)
                return std::nan("");
            return array->type == results::ArrayType::Float32 ? static_cast<double>(array->values<float>()[index])
                                                              : array->values<double>()[index];
        };
        const model::Model* model = context.modelOf(ref.set);
        const std::map<std::string, std::string> modelCards =
            model != nullptr ? monitorCardsOf(*model) : std::map<std::string, std::string>();
        // the names of the rows of a MONPNT3 matrix whose directory the file does not hold, from the deck
        const std::vector<model::MonitorNodeForceSum3Card> deckCards =
            model != nullptr ? monitorNodeForceSum3Cards(*model) : std::vector<model::MonitorNodeForceSum3Card>();
        const std::vector<MonitorNodeForceSum3Row> unnamedRows = monitorNodeForceSum3Rows(deckCards);
        std::optional<analysis::CoordinateTransform> transform;
        if (model != nullptr)
            transform.emplace(*model);
        const bool transient = ref.result->meta().analysisCode == 6;
        std::vector<double> stepValues(rows);
        for (std::size_t row = 0; row < rows; ++row)
            stepValues[row] = real(step, row);
        const std::set<std::uint64_t> kept =
            selectedSteps(stepValues, transient ? context.selection().times : context.selection().freqs);

        std::map<std::tuple<std::string, std::string, std::string, std::int32_t, std::uint64_t>, Group> groups;
        std::vector<std::tuple<std::string, std::string, std::string, std::int32_t, std::uint64_t>> order;
        for (std::size_t row = 0; row < rows; ++row)
        {
            std::string name = names->at(row);
            auto slot = static_cast<std::size_t>(std::clamp<std::int64_t>(integer(component, row) - 1, 0, 5));
            std::string card = cards != nullptr ? cardTypeOf(cards->at(row)) : std::string();
            const double at = stepValues[row];
            if (!std::isnan(at) && !kept.contains(stepKey(at)))
                continue;
            const model::MonitorNodeForceSum3Card* deckCard = nullptr;
            if (name.empty() && matrixRow != nullptr)
            {
                // a MONPNT3 row of the matrix, named by the deck when its MONPNT3 cards give as many rows
                const auto position = static_cast<std::size_t>(integer(matrixRow, row));
                if (position < unnamedRows.size() && unnamedRows.size() == rowsOfUnnamed(*matrixRow, *names))
                {
                    deckCard = unnamedRows[position].card;
                    name = deckCard->name;
                    slot = unnamedRows[position].component;
                }
            }
            if (const auto found = modelCards.find(name); found != modelCards.end())
                card = found->second;
            if (!request.wants(name, card))
                continue;
            const std::string side = sides != nullptr ? sides->at(row) : std::string();
            const std::string contribution = contributions->at(row);
            const auto key = std::make_tuple(name, side, contribution, static_cast<std::int32_t>(integer(subcase, row)),
                                             stepKey(at));
            auto found = groups.find(key);
            if (found == groups.end())
            {
                Group group;
                MonitorLoadRow& load = group.row;
                load.set = ref.set;
                load.analysis = context.sets()[ref.set].analysis;
                load.subcase = static_cast<std::int32_t>(integer(subcase, row));
                load.designCycle = ref.result->meta().designCycle;
                load.step = at;
                load.stepKind = std::isnan(at) ? results::StepKind::None
                                               : (ref.result->meta().analysisCode == 6 ? results::StepKind::Time
                                                                                       : results::StepKind::Frequency);
                load.precision = single ? Precision::Single : Precision::Double;
                load.isComplex = complexValues;
                load.name = name;
                load.card = card;
                load.side = side;
                load.contribution = contribution;
                load.displacementFrameId = deckCard != nullptr
                                               ? static_cast<std::int32_t>(deckCard->displacementFrameId)
                                               : static_cast<std::int32_t>(integer(displacementFrame, row));
                const auto pointCp = deckCard != nullptr ? deckCard->positionFrameId
                                                         : static_cast<base::EntityId>(integer(positionFrame, row));
                const base::Vec3 local =
                    deckCard != nullptr
                        ? base::Vec3{deckCard->xCoord, deckCard->yCoord, deckCard->zCoord}
                        : base::Vec3{real(point, row * 3), real(point, row * 3 + 1), real(point, row * 3 + 2)};
                if (pointCp == 0)
                    load.point = {local.x, local.y, local.z};
                else if (transform && transform->hasSystem(pointCp))
                {
                    const base::Vec3 basic = transform->toBasic(pointCp, local);
                    load.point = {basic.x, basic.y, basic.z};
                }
                load.status = std::string(results::resultStatusName(ref.result->meta().status));
                found = groups.emplace(key, std::move(group)).first;
                order.push_back(key);
            }
            MonitorLoadRow& load = found->second.row;
            load.real[slot] = number(complexValues ? 2 * row : row);
            load.imaginary[slot] = complexValues ? number(2 * row + 1) : 0.0;
        }
        addTotals(groups, order);
        std::int64_t added = 0;
        for (const auto& key : order)
        {
            MonitorLoadRow& load = groups.at(key).row;
            if (sink.covered(load.set, load.name, load.subcase, load.designCycle))
                continue;
            sink.add(std::move(load));
            ++added;
        }
        return added;
    }

    // The number of rows of a matrix without directory (the largest matrix row named, plus one).
    static std::size_t rowsOfUnnamed(const results::ResultArray& matrixRow, const results::ResultText& names)
    {
        std::int64_t largest = -1;
        const bool wide = matrixRow.type == results::ArrayType::Int64;
        for (std::size_t row = 0; row < names.size(); ++row)
            if (names.at(row).empty())
                largest = std::max<std::int64_t>(
                    largest, wide ? matrixRow.values<std::int64_t>()[row] : matrixRow.values<std::int32_t>()[row]);
        return static_cast<std::size_t>(largest + 1);
    }

    // TOTAL AERO and TOTAL of each point and step, after its contributions.
    template <typename Groups, typename Order>
    static void addTotals(Groups& groups, Order& order)
    {
        std::vector<typename Order::value_type> added;
        std::set<std::tuple<std::string, std::string, std::int32_t, std::uint64_t>> done;
        for (const auto& key : Order(order))
        {
            const auto& [name, side, contribution, subcase, step] = key;
            if (!done.emplace(name, side, subcase, step).second)
                continue;
            const auto part = [&](const char* which) -> const MonitorLoadRow*
            {
                const auto found = groups.find(std::make_tuple(name, side, std::string(which), subcase, step));
                return found == groups.end() ? nullptr : &found->second.row;
            };
            const MonitorLoadRow* flexible = part("flexible");
            const MonitorLoadRow* gust = part("gust");
            if (flexible == nullptr && gust == nullptr)
                continue;
            const auto sum = [&](std::initializer_list<const MonitorLoadRow*> parts, const char* which)
            {
                const MonitorLoadRow* first = nullptr;
                for (const MonitorLoadRow* item : parts)
                    if (item != nullptr && first == nullptr)
                        first = item;
                MonitorLoadRow total = *first;
                total.contribution = which;
                for (std::size_t slot = 0; slot < 6; ++slot)
                {
                    double real = 0.0;
                    double imaginary = 0.0;
                    bool any = false;
                    for (const MonitorLoadRow* item : parts)
                        if (item != nullptr && !std::isnan(item->real[slot]))
                        {
                            real += item->real[slot];
                            imaginary += item->imaginary[slot];
                            any = true;
                        }
                    total.real[slot] = any ? real : std::nan("");
                    total.imaginary[slot] = any ? imaginary : 0.0;
                }
                const auto totalKey = std::make_tuple(name, side, std::string(which), subcase, step);
                groups.emplace(totalKey, Group{total, {}});
                added.push_back(totalKey);
            };
            sum({flexible, gust}, "totalAero");
            if (side == "structural")
                sum({part("inertial"), part("external"), flexible, gust}, "total");
        }
        order.insert(order.end(), added.begin(), added.end());
    }
};

// ---- a MONPNT3 computed from the grid point forces
// -----------------------------------------------------------------------

// A MONPNT3 as the format reference computes it (MONPNT3): at the grid points of GRIDSET, the grid point forces of the
// elements of ELEMSET (none when it is blank) and the loads XFLAG does not exclude (S the
// singlePointConstrForce rows, M the multiPointConstrForce rows, A, L or P the appliedLoad rows, D every other type),
// summed about (X, Y, Z) of CP and given along the axes of CD at that point. With ELEMSET the elements of one side and
// XFLAG blank, the sum is the internal load A + B (the other side's elements reversed, at equilibrium).
class ComputedMonitorNodeForceSum3Provider final : public MonitorProvider
{
public:
    [[nodiscard]] std::string name() const override { return "grid point forces of the MONPNT3"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const MonitorRequest& request,
                                  MonitorLoadSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the MONPNT3 cards are in the model)"};
        const model::Model& model = *context.modelPointer();
        const auto* table = model.findTable<model::MonitorNodeForceSum3Table>();
        if (table == nullptr)
            return {0, "the model has no MONPNT3"};
        const std::shared_ptr<const NodePointForceSet> forces = NodePointForceSet::read(context);
        if (forces->empty())
            return {0, "the result file has no grid point force table (output of GPFORCE was not requested)"};
        analysis::CoordinateTransform transform(model);
        const NodePositions positions(model, context.exec().pool());
        Outcome outcome;
        for (std::uint32_t cardRow = 0; cardRow < table->rows().rowCount(); ++cardRow)
        {
            if (!table->rows().isAlive(cardRow))
                continue;
            const model::MonitorNodeForceSum3Card card = table->card(cardRow);
            if (!request.wants(card.name, std::string(model::MonitorNodeForceSum3Table::NAME)))
                continue;
            const std::optional<aero::IdRuns> grids = aero::setMembers(model, card.nodeSetId);
            if (!grids)
                continue;
            std::vector<base::EntityId> gridIds = grids->expand("the GRIDSET of MONPNT3 " + card.name);
            std::set<std::int32_t> elements;
            if (card.elementSet > 0)
                if (const std::optional<aero::IdRuns> listed = aero::setMembers(model, card.elementSet))
                    for (const base::EntityId element : listed->expand("the ELEMSET of MONPNT3 " + card.name))
                        elements.insert(static_cast<std::int32_t>(element));
            const auto excluded = [&](char letter) { return card.excluded.find(letter) != std::string::npos; };
            const auto counts = [&](NodePointForceKind kind, std::int32_t element)
            {
                switch (kind)
                {
                case NodePointForceKind::Element:
                    return elements.contains(element);
                case NodePointForceKind::AppliedLoad:
                    return !excluded('A') && !excluded('L') && !excluded('P');
                case NodePointForceKind::Spc:
                    return !excluded('S');
                case NodePointForceKind::Mpc:
                    return !excluded('M');
                case NodePointForceKind::Total:
                    return false;
                case NodePointForceKind::Other:
                    return !excluded('D');
                }
                return false;
            };
            const base::Vec3 about =
                transform.toBasic(card.positionFrameId, base::Vec3{card.xCoord, card.yCoord, card.zCoord});
            const base::Mat3 axes = transform.displacementRotation(card.displacementFrameId, about).transposed();
            const std::vector<std::size_t> monitored = digitsOf(card.outputAxes);
            for (const NodePointForceTable& forceTable : forces->tables())
            {
                if (sink.covered(forceTable.set, card.name, forceTable.subcase, forceTable.designCycle))
                    continue;
                bool anyRow = false;
                const std::size_t steps = forceTable.stepCount();
                std::vector<std::array<double, 6>> real(steps, std::array<double, 6>{});
                std::vector<std::array<double, 6>> imaginary(steps, std::array<double, 6>{});
                for (const base::EntityId grid : gridIds)
                {
                    const auto node = static_cast<std::int32_t>(grid);
                    const base::Vec3 at = positions.position(grid);
                    const base::Vec3 arm = at - about;
                    for (const std::uint32_t row : forceTable.rowsAt(node))
                    {
                        if (!counts(forceTable.layout->kind[row], forceTable.layout->element[row]))
                            continue;
                        anyRow = true;
                        for (std::size_t step = 0; step < steps; ++step)
                        {
                            addMoment(real[step], forceTable, step, row, arm, false);
                            if (forceTable.isComplex)
                                addMoment(imaginary[step], forceTable, step, row, arm, true);
                        }
                    }
                }
                if (!anyRow)
                    continue;
                for (std::size_t step = 0; step < steps; ++step)
                {
                    MonitorLoadRow load;
                    load.set = forceTable.set;
                    load.analysis = forceTable.analysis;
                    load.subcase = forceTable.subcase;
                    load.designCycle = forceTable.designCycle;
                    load.stepKind = forceTable.stepKind;
                    load.step = forceTable.steps[step];
                    load.precision = forceTable.precision;
                    load.isComplex = forceTable.isComplex;
                    load.name = card.name;
                    load.card = std::string(model::MonitorNodeForceSum3Table::NAME);
                    load.side = "structural";
                    load.contribution = "resultant";
                    load.displacementFrameId = static_cast<std::int32_t>(card.displacementFrameId);
                    load.point = {about.x, about.y, about.z};
                    const std::array<double, 6> turnedReal = turned(axes, real[step]);
                    const std::array<double, 6> turnedImaginary = turned(axes, imaginary[step]);
                    for (const std::size_t slot : monitored)
                    {
                        load.real[slot] = turnedReal[slot];
                        load.imaginary[slot] = turnedImaginary[slot];
                    }
                    sink.add(std::move(load));
                    ++outcome.rows;
                }
            }
        }
        if (outcome.rows == 0)
            outcome.reason = "the grid point forces have no row at the grid points of the MONPNT3 asked for";
        return outcome;
    }

private:
    static void addMoment(std::array<double, 6>& sum, const NodePointForceTable& table, std::size_t step,
                          std::size_t row, const base::Vec3& arm, bool imaginaryPart)
    {
        std::array<double, 6> value{};
        for (std::size_t component = 0; component < 6; ++component)
            value[component] =
                imaginaryPart ? table.imaginaryAt(step, row, component) : table.realAt(step, row, component);
        sum[0] += value[0];
        sum[1] += value[1];
        sum[2] += value[2];
        sum[3] += value[3] + arm.y * value[2] - arm.z * value[1];
        sum[4] += value[4] + arm.z * value[0] - arm.x * value[2];
        sum[5] += value[5] + arm.x * value[1] - arm.y * value[0];
    }

    static std::array<double, 6> turned(const base::Mat3& axes, const std::array<double, 6>& value)
    {
        const base::Vec3 force = axes * base::Vec3{value[0], value[1], value[2]};
        const base::Vec3 moment = axes * base::Vec3{value[3], value[4], value[5]};
        return {force.x, force.y, force.z, moment.x, moment.y, moment.z};
    }
};

} // namespace

const MonitorChain& monitorChain()
{
    static const MonitorChain CHAIN = []
    {
        MonitorChain chain("monitor point loads");
        chain.add(std::make_unique<StoredMonitorProvider>());
        chain.add(std::make_unique<ComputedMonitorNodeForceSum3Provider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
