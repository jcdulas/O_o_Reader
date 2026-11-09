#include "oor/model/cards/properties/NonStructMassFields.hpp"
#include "oor/model/cards/properties/nonStructMass/NonStructMassTable.hpp"
#include "oor/model/cards/properties/nonStructMass1/NonStructMass1Table.hpp"
#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedTable.hpp"
#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Table.hpp"
#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>

namespace oor::query
{

namespace
{

// One target of a non-structural mass card.
struct Target
{
    bool isElement = false;
    std::int32_t id = 0;
};

// The measures of the elements of each property and of each element: the sum of areas and of lengths.
struct Measures
{
    std::unordered_map<std::int32_t, std::pair<double, double>> byProperty; // pid -> (area, length)
    std::unordered_map<std::int32_t, std::pair<double, double>> byElement;  // eid -> (area, length)
};

bool isAreaType(const std::string& type)
{
    return type == "ShellProp" || type == "CompProp" || type == "CompPropGlobal" || type == "ShearPanelProp" ||
           type == "ConicalShellProp" || type == "CrackTipProp2D";
}

// The properties a TYPE and ALL stand for: the cards of that type, and the ones that count as it.
std::vector<std::int32_t> propertiesOfType(const model::Model& model, const std::string& type)
{
    std::vector<std::string> names{type};
    if (type == "ShellProp")
        names = {"ShellProp", "CompProp", "CompPropGlobal"};
    else if (type == "BarProp")
        names = {"BarProp", "BarShapeProp"};
    else if (type == "BeamProp")
        names = {"BeamProp", "BeamShapeProp", "BeamLumpedAreaProp"};
    std::vector<std::int32_t> ids;
    for (const std::uint16_t cardType : typesInSpace(model::CardCategory::Property, base::IdSpace::Property))
    {
        // The TYPE of an NSM card is the neutral name of a card type.
        if (std::find(names.begin(), names.end(), typeName(cardType)) == names.end())
            continue;
        const model::CardTable* table = model.findTable(cardType);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                ids.push_back(table->idAt(row));
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

template <typename TableT>
void collectSets(const model::Model& model, std::set<base::EntityId>& found)
{
    if (const TableT* table = model.findTable<TableT>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                found.insert(table->idAt(row));
        }
    }
}

// The sets of cards a set ID stands for: an NSMADD gives the sets it adds (and theirs), else the set itself.
void expandSet(const model::Model& model, base::EntityId set, int depth, std::vector<base::EntityId>& out)
{
    bool added = false;
    if (const model::NonStructMassUnionTable* table = model.findTable<model::NonStructMassUnionTable>();
        table != nullptr && depth < 8)
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || table->idAt(row) != set)
                continue;
            added = true;
            for (const base::EntityId member : table->card(row).sets)
                expandSet(model, member, depth + 1, out);
        }
    }
    if (!added)
        out.push_back(set);
}

// nonStructMassTable: the non-structural mass cards, one row for each property or element they name.
class NonStructMassTableQuery final : public QueryBase
{
public:
    NonStructMassTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // The sets to report: the ones asked for, else every set of the model.
        std::vector<base::EntityId> sets;
        if (const auto ids = sortedIds(run.arguments.ids("setIds")))
        {
            for (const std::int64_t id : *ids)
                sets.push_back(static_cast<base::EntityId>(id));
        }
        else
        {
            std::set<base::EntityId> found;
            collectSets<model::NonStructMassTable>(model, found);
            collectSets<model::NonStructMass1Table>(model, found);
            collectSets<model::NonStructMassLumpedTable>(model, found);
            collectSets<model::NonStructMassLumped1Table>(model, found);
            collectSets<model::NonStructMassUnionTable>(model, found);
            sets.assign(found.begin(), found.end());
        }
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        Measures measures;
        if (elements.size() > 0)
        {
            ElementData data(run.model, elements, run.context);
            const std::vector<std::int32_t>& pids = data.properties();
            const std::vector<double>& areas = data.areas();
            const std::vector<double>& lengths = data.lengths();
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                const double area = std::isfinite(areas[index]) ? areas[index] : 0.0;
                const double length = std::isfinite(lengths[index]) ? lengths[index] : 0.0;
                measures.byElement[elements.ids[index]] = {area, length};
                auto& total = measures.byProperty[pids[index]];
                total.first += area;
                total.second += length;
            }
        }

        std::vector<std::int32_t> sid;
        std::vector<std::string> card;
        std::vector<std::string> targetType;
        std::vector<std::int32_t> targetId;
        std::vector<std::string> kind;
        std::vector<double> value;
        std::vector<double> mass;
        for (const base::EntityId set : sets)
        {
            // An NSMADD takes precedence over cards of the same ID: the set is the sets it adds.
            std::vector<base::EntityId> members;
            expandSet(model, set, 0, members);
            for (const base::EntityId member : members)
                appendRows(model, measures, set, member, sid, card, targetType, targetId, kind, value, mass);
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.dictionary("card", card);
        builder.dictionary("targetType", targetType);
        builder.int32("targetId", std::move(targetId));
        builder.dictionary("kind", kind);
        builder.real("value", std::move(value));
        builder.real("mass", std::move(mass));
        return builder.finish();
    }

private:
    static void appendRows(const model::Model& model, const Measures& measures, base::EntityId asked,
                           base::EntityId member, std::vector<std::int32_t>& sid, std::vector<std::string>& card,
                           std::vector<std::string>& targetType, std::vector<std::int32_t>& targetId,
                           std::vector<std::string>& kind, std::vector<double>& value, std::vector<double>& mass)
    {
        const auto measureOf = [&](const Target& target, const std::string& type, bool& perArea) -> double
        {
            const auto& table = target.isElement ? measures.byElement : measures.byProperty;
            const auto found = table.find(target.id);
            const double area = found == table.end() ? 0.0 : found->second.first;
            const double length = found == table.end() ? 0.0 : found->second.second;
            perArea = target.isElement ? !(length > 0.0) : isAreaType(type);
            return perArea ? area : length;
        };
        const auto emit = [&](const char* name, const std::string& type, const std::vector<Target>& targets,
                              const std::vector<double>& values, bool lumped, bool shared)
        {
            double total = 0.0;
            std::vector<double> measure(targets.size());
            std::vector<bool> area(targets.size());
            for (std::size_t index = 0; index < targets.size(); ++index)
            {
                bool perArea = false;
                measure[index] = measureOf(targets[index], type, perArea);
                area[index] = perArea;
                total += measure[index];
            }
            for (std::size_t index = 0; index < targets.size(); ++index)
            {
                sid.push_back(asked);
                card.emplace_back(name);
                targetType.emplace_back(targets[index].isElement ? "element" : "property");
                targetId.push_back(targets[index].id);
                kind.emplace_back(lumped ? "lumped" : (area[index] ? "perArea" : "perLength"));
                const double given = values[index];
                value.push_back(given);
                if (!lumped)
                    mass.push_back(given * measure[index]);
                else if (shared)
                    mass.push_back(total > 0.0 ? given * measure[index] / total : 0.0);
                else
                    mass.push_back(measure[index] > 0.0 ? given : 0.0);
            }
        };
        const auto pairsToTargets = [&](const std::string& type, const std::vector<model::NonStructMassPair>& pairs,
                                        std::vector<Target>& targets, std::vector<double>& values)
        {
            for (const model::NonStructMassPair& pair : pairs)
            {
                targets.push_back(Target{model::nonStructMassTypeIsElement(type), pair.id});
                values.push_back(pair.value);
            }
        };
        const auto idsToTargets = [&](const std::string& type, const model::NonStructMassIdList& ids, double given,
                                      std::vector<Target>& targets, std::vector<double>& values)
        {
            const bool isElement = model::nonStructMassTypeIsElement(type);
            std::vector<std::int32_t> list;
            if (ids.all)
            {
                if (isElement && type == model::NON_STRUCT_MASS_ELEMENT)
                {
                    // every element
                    for (const auto& entry : measures.byElement)
                        list.push_back(entry.first);
                    std::sort(list.begin(), list.end());
                }
                else if (!isElement)
                    list = propertiesOfType(model, type);
            }
            else
                list = model::expandNonStructMassIds(ids);
            for (const std::int32_t id : list)
            {
                targets.push_back(Target{isElement, id});
                values.push_back(given);
            }
        };
        if (const auto* table = model.findTable<model::NonStructMassTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row) || table->idAt(row) != member)
                    continue;
                const model::NonStructMassCard data = table->card(row);
                std::vector<Target> targets;
                std::vector<double> values;
                pairsToTargets(data.type, data.pairs, targets, values);
                emit("NonStructMass", data.type, targets, values, false, false);
            }
        }
        if (const auto* table = model.findTable<model::NonStructMass1Table>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row) || table->idAt(row) != member)
                    continue;
                const model::NonStructMass1Card data = table->card(row);
                std::vector<Target> targets;
                std::vector<double> values;
                idsToTargets(data.type, data.ids, data.nonStructMass, targets, values);
                emit("NonStructMass1", data.type, targets, values, false, false);
            }
        }
        if (const auto* table = model.findTable<model::NonStructMassLumpedTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row) || table->idAt(row) != member)
                    continue;
                const model::NonStructMassLumpedCard data = table->card(row);
                std::vector<Target> targets;
                std::vector<double> values;
                pairsToTargets(data.type, data.pairs, targets, values);
                emit("NonStructMassLumped", data.type, targets, values, true, false);
            }
        }
        if (const auto* table = model.findTable<model::NonStructMassLumped1Table>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row) || table->idAt(row) != member)
                    continue;
                const model::NonStructMassLumped1Card data = table->card(row);
                std::vector<Target> targets;
                std::vector<double> values;
                idsToTargets(data.type, data.ids, data.nonStructMass, targets, values);
                emit("NonStructMassLumped1", data.type, targets, values, true, true);
            }
        }
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "nonStructMassTable";
        info.family = "T7";
        info.description =
            "The non-structural mass cards (NSM, NSM1, NSML, NSML1; NSMADD expanded) of the sets asked for, one row "
            "for each "
            "property or element they name: the kind (perArea for shells, perLength for rods, bars and beams, lumped "
            "for "
            "NSML and NSML1), the value of the card and the mass it puts on the target (a per-unit value times the "
            "area or "
            "length of its elements; NSML the value itself; NSML1 the value shared between the targets in proportion "
            "to "
            "their area or length). setId is the set asked for. ALL stands for every property of the type (or every "
            "element).";
        info.parameters = {
            parameter("setIds", ParameterKind::IdList, "None", "the NSM sets to list (default: every set)")};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId")
            .dictionary("card")
            .dictionary("targetType")
            .int32("targetId")
            .dictionary("kind")
            .real("value")
            .real("mass");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNonStructMassTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NonStructMassTableQuery>());
}

} // namespace oor::query
