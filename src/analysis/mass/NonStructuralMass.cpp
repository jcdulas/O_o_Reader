#include "oor/analysis/mass/NonStructuralMass.hpp"

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/properties/nonStructMass/NonStructMassTable.hpp"
#include "oor/model/cards/properties/nonStructMass1/NonStructMass1Table.hpp"
#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedTable.hpp"
#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Table.hpp"
#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionTable.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The kinds of element a non-structural mass can be put on, which is also what a property TYPE of the cards means.
enum class Family : std::uint8_t
{
    Shell,      // PSHELL, PCOMP, PCOMPG: the shells, by area
    Shear,      // PSHEAR: CSHEAR, by area
    Bar,        // PBAR, PBARL: CBAR, by length
    Beam,       // PBEAM, PBEAML, PBCOMP: CBEAM, by length
    Rod,        // PROD: CROD
    Tube,       // PTUBE: CTUBE
    Bend,       // PBEND: CBEND
    RodWithProp // Rod2InlineSection (the NSM TYPE CONROD)
};

constexpr std::size_t FAMILY_COUNT = 8;

bool isArea(Family family) noexcept
{
    return family == Family::Shell || family == Family::Shear;
}

// What a property TYPE of the cards stands for (a neutral card name); false for a type with no element in the library
// (ConicalShellProp, CrackTipProp2D).
bool familyOfType(const std::string& type, Family& family) noexcept
{
    if (type == "ShellProp" || type == "CompProp" || type == "CompPropGlobal")
        family = Family::Shell;
    else if (type == "ShearPanelProp")
        family = Family::Shear;
    else if (type == "BarProp" || type == "BarShapeProp")
        family = Family::Bar;
    else if (type == "BeamProp" || type == "BeamShapeProp" || type == "BeamLumpedAreaProp")
        family = Family::Beam;
    else if (type == "RodProp")
        family = Family::Rod;
    else if (type == "TubeProp")
        family = Family::Tube;
    else if (type == "CurvedBeamProp")
        family = Family::Bend;
    else if (type == "Rod2InlineSection")
        family = Family::RodWithProp;
    else
        return false;
    return true;
}

// One element that can carry non-structural mass.
struct Item
{
    std::uint16_t type = 0;
    std::uint32_t row = 0;
    base::EntityId eid = 0;
    base::EntityId pid = 0;
    double measure = 0.0; // the area or the length
    Family family = Family::Shell;
};

// One line of the cards of a set: what it is put on and how. The IDs stay ranges as the card writes them (an NSM or
// NSML pair is a range of one ID).
struct Entry
{
    std::string type;
    bool lumped = false;
    bool all = false;
    std::vector<model::NonStructMassRun> runs;
    double value = 0.0;
};

// A range longer than this is matched by walking the elements of the index rather than by looking its IDs up one by
// one: "1 THRU 99999999", a common way of writing every element, then costs the number of elements, not of IDs (and a
// hostile "1 THRU 2147483647" costs no more).
constexpr std::uint64_t LONG_RUN = 65'536;

bool isLongRun(const model::NonStructMassRun& run)
{
    return base::idRangeCount(run.first, run.last, run.increment) > LONG_RUN;
}

bool inRun(const model::NonStructMassRun& run, base::EntityId id)
{
    const std::int64_t step = std::max<std::int64_t>(run.increment, 1);
    return id >= run.first && id <= run.last && (static_cast<std::int64_t>(id) - run.first) % step == 0;
}

// Calls fn(id) for each ID of a short range, in increasing order (64-bit: a last ID of 2147483647 must not wrap).
template <typename Fn>
void forEachIdOfRun(const model::NonStructMassRun& run, Fn&& fn)
{
    const std::int64_t step = std::max<std::int64_t>(run.increment, 1);
    for (std::int64_t id = run.first; id <= run.last; id += step)
        fn(static_cast<base::EntityId>(id));
}

// The elements of a model that can carry non-structural mass, with the key to find them by element or by property.
class ElementIndex
{
public:
    ElementIndex(const model::Model& model, base::ThreadPool* pool)
    {
        addShells<model::ShellQuad4Table>(model, pool, Family::Shell);
        addShells<model::ShellTria3Table>(model, pool, Family::Shell);
        addShells<model::ShellQuad8Table>(model, pool, Family::Shell);
        addShells<model::ShellTria6Table>(model, pool, Family::Shell);
        addShells<model::ShellQuad4DrillingTable>(model, pool, Family::Shell);
        addShells<model::ShellTria3DrillingTable>(model, pool, Family::Shell);
        addShells<model::PlaneStrainQuadTable>(model, pool, Family::Shell);
        addShells<model::ShearPanel4Table>(model, pool, Family::Shear);
        addLines<model::Rod2Table>(model, pool, Family::Rod);
        addLines<model::Tube2Table>(model, pool, Family::Tube);
        addLines<model::Bar2Table>(model, pool, Family::Bar);
        addLines<model::Beam2Table>(model, pool, Family::Beam);
        addLines<model::Rod2InlineSectionTable>(model, pool, Family::RodWithProp);
        for (std::size_t index = 0; index < m_items.size(); ++index)
        {
            m_byElement.emplace(m_items[index].eid, index);
            m_byFamily[static_cast<std::size_t>(m_items[index].family)].push_back(index);
            if (m_items[index].pid != 0)
                m_byProperty[key(m_items[index].family, m_items[index].pid)].push_back(index);
        }
    }

    [[nodiscard]] const Item& item(std::size_t index) const { return m_items[index]; }
    [[nodiscard]] const std::vector<std::size_t>& family(Family family) const
    {
        return m_byFamily[static_cast<std::size_t>(family)];
    }
    [[nodiscard]] std::size_t count() const noexcept { return m_items.size(); }

    [[nodiscard]] const std::vector<std::size_t>& property(Family family, base::EntityId pid) const
    {
        static const std::vector<std::size_t> NONE;
        const auto found = m_byProperty.find(key(family, pid));
        return found == m_byProperty.end() ? NONE : found->second;
    }

    [[nodiscard]] const Item* element(base::EntityId eid) const
    {
        const auto found = m_byElement.find(eid);
        return found == m_byElement.end() ? nullptr : &m_items[found->second];
    }
    [[nodiscard]] std::size_t indexOf(base::EntityId eid) const { return m_byElement.at(eid); }

private:
    static std::uint64_t key(Family family, base::EntityId pid) noexcept
    {
        return (static_cast<std::uint64_t>(family) << 32) | static_cast<std::uint32_t>(pid);
    }

    // The shells and the shear panels: area of the element, property from the card.
    template <typename TableT>
    void addShells(const model::Model& model, base::ThreadPool* pool, Family family)
    {
        add<TableT>(model, pool, family, Quantity::Area);
    }
    template <typename TableT>
    void addLines(const model::Model& model, base::ThreadPool* pool, Family family)
    {
        add<TableT>(model, pool, family, Quantity::Length);
    }

    template <typename TableT>
    void add(const model::Model& model, base::ThreadPool* pool, Family family, Quantity quantity)
    {
        const TableT* table = model.findTable<TableT>();
        if (table == nullptr)
            return;
        std::vector<std::uint32_t> rows;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                rows.push_back(row);
        }
        const std::uint16_t type = model::typeIndexOf<TableT>();
        const ElementArrays arrays = ElementGeometry::compute(model, type, rows, bit(quantity), pool);
        const std::vector<double>& measures = quantity == Quantity::Area ? arrays.areas : arrays.lengths;
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            Item item;
            item.type = type;
            item.row = rows[index];
            item.eid = table->idAtRow(rows[index]);
            if constexpr (!std::is_same_v<TableT, model::Rod2InlineSectionTable>)
                item.pid = table->card(rows[index]).propId;
            item.measure = measures[index];
            item.family = family;
            m_items.push_back(item);
        }
    }

    std::vector<Item> m_items;
    std::unordered_map<base::EntityId, std::size_t> m_byElement;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> m_byProperty;
    std::array<std::vector<std::size_t>, FAMILY_COUNT> m_byFamily;
};

template <typename TableT>
const TableT* tableOf(const model::Model& model)
{
    return model.findTable<TableT>();
}

// Does a live card of this table have this set ID?
template <typename TableT>
bool hasSet(const model::Model& model, base::EntityId set)
{
    const TableT* table = tableOf<TableT>(model);
    if (table == nullptr)
        return false;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row) && table->idAtRow(row) == set)
            return true;
    }
    return false;
}

// The sets whose cards make up the set `set`: the sets an NSMADD of that ID lists, or the set itself.
std::unordered_set<base::EntityId> memberSets(const model::Model& model, base::EntityId set)
{
    std::unordered_set<base::EntityId> members;
    bool isAdd = false;
    if (const model::NonStructMassUnionTable* table = model.findTable<model::NonStructMassUnionTable>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || table->idAtRow(row) != set)
                continue;
            isAdd = true;
            for (const base::EntityId member : table->card(row).sets)
                members.insert(member);
        }
    }
    if (!isAdd)
        members.insert(set);
    return members;
}

std::vector<Entry> entriesOf(const model::Model& model, const std::unordered_set<base::EntityId>& members)
{
    std::vector<Entry> entries;
    const auto wanted = [&](base::EntityId set) { return members.count(set) != 0; };
    if (const model::NonStructMassTable* table = model.findTable<model::NonStructMassTable>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || !wanted(table->idAtRow(row)))
                continue;
            const model::NonStructMassCard card = table->card(row);
            for (const model::NonStructMassPair& pair : card.pairs)
                entries.push_back(
                    Entry{card.type, false, false, {model::NonStructMassRun{pair.id, pair.id, 1}}, pair.value});
        }
    }
    if (const model::NonStructMassLumpedTable* table = model.findTable<model::NonStructMassLumpedTable>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || !wanted(table->idAtRow(row)))
                continue;
            const model::NonStructMassLumpedCard card = table->card(row);
            for (const model::NonStructMassPair& pair : card.pairs)
                entries.push_back(
                    Entry{card.type, true, false, {model::NonStructMassRun{pair.id, pair.id, 1}}, pair.value});
        }
    }
    if (const model::NonStructMass1Table* table = model.findTable<model::NonStructMass1Table>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || !wanted(table->idAtRow(row)))
                continue;
            const model::NonStructMass1Card card = table->card(row);
            entries.push_back(Entry{card.type, false, card.ids.all, card.ids.runs, card.nonStructMass});
        }
    }
    if (const model::NonStructMassLumped1Table* table = model.findTable<model::NonStructMassLumped1Table>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row) || !wanted(table->idAtRow(row)))
                continue;
            const model::NonStructMassLumped1Card card = table->card(row);
            entries.push_back(Entry{card.type, true, card.ids.all, card.ids.runs, card.nonStructMass});
        }
    }
    return entries;
}

// The elements an entry puts mass on, one index per ID (an element listed twice appears twice).
std::vector<std::size_t> matchedItems(const Entry& entry, const ElementIndex& index)
{
    std::vector<std::size_t> matched;
    if (model::nonStructMassTypeIsElement(entry.type))
    {
        const bool rodsWithPropOnly = entry.type == "Rod2InlineSection";
        if (entry.all)
        {
            for (std::size_t item = 0; item < index.count(); ++item)
            {
                if (!rodsWithPropOnly || index.item(item).family == Family::RodWithProp)
                    matched.push_back(item);
            }
            return matched;
        }
        const auto accepted = [&](const Item& item) { return !rodsWithPropOnly || item.family == Family::RodWithProp; };
        for (const model::NonStructMassRun& run : entry.runs)
        {
            if (!isLongRun(run))
            {
                forEachIdOfRun(run,
                               [&](base::EntityId eid)
                               {
                                   const Item* found = index.element(eid);
                                   if (found != nullptr && accepted(*found))
                                       matched.push_back(index.indexOf(eid));
                               });
                continue;
            }
            // the same items in the same order as the lookups: by element ID
            std::vector<std::size_t> inRange;
            for (std::size_t item = 0; item < index.count(); ++item)
                if (inRun(run, index.item(item).eid) && accepted(index.item(item)))
                    inRange.push_back(item);
            std::sort(inRange.begin(), inRange.end(),
                      [&](std::size_t a, std::size_t b) { return index.item(a).eid < index.item(b).eid; });
            matched.insert(matched.end(), inRange.begin(), inRange.end());
        }
        return matched;
    }
    Family family = Family::Shell;
    if (!familyOfType(entry.type, family))
        return matched; // PCONEAX and PRAC2D: no such element in the library
    if (entry.all)
        return index.family(family);
    for (const model::NonStructMassRun& run : entry.runs)
    {
        if (!isLongRun(run))
        {
            forEachIdOfRun(run,
                           [&](base::EntityId pid)
                           {
                               const std::vector<std::size_t>& items = index.property(family, pid);
                               matched.insert(matched.end(), items.begin(), items.end());
                           });
            continue;
        }
        // the same items in the same order as the lookups: by property ID, each in the order of the index
        std::vector<std::size_t> inRange;
        for (const std::size_t item : index.family(family))
            if (index.item(item).pid != 0 && inRun(run, index.item(item).pid))
                inRange.push_back(item);
        std::stable_sort(inRange.begin(), inRange.end(),
                         [&](std::size_t a, std::size_t b) { return index.item(a).pid < index.item(b).pid; });
        matched.insert(matched.end(), inRange.begin(), inRange.end());
    }
    return matched;
}

} // namespace

std::vector<base::EntityId> NonStructuralMass::setIds(const model::Model& model)
{
    std::vector<base::EntityId> ids;
    const auto collect = [&](const auto* table)
    {
        if (table == nullptr)
            return;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                ids.push_back(table->idAtRow(row));
        }
    };
    collect(model.findTable<model::NonStructMassTable>());
    collect(model.findTable<model::NonStructMass1Table>());
    collect(model.findTable<model::NonStructMassLumpedTable>());
    collect(model.findTable<model::NonStructMassLumped1Table>());
    collect(model.findTable<model::NonStructMassUnionTable>());
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

bool NonStructuralMass::exists(const model::Model& model, base::EntityId set)
{
    return hasSet<model::NonStructMassTable>(model, set) || hasSet<model::NonStructMass1Table>(model, set) ||
           hasSet<model::NonStructMassLumpedTable>(model, set) ||
           hasSet<model::NonStructMassLumped1Table>(model, set) || hasSet<model::NonStructMassUnionTable>(model, set);
}

std::optional<base::EntityId> NonStructuralMass::selectedByCaseControl(const model::Model& model)
{
    const model::CaseControl& control = model.control().caseControl;
    for (const model::Subcase& subcase : control.segments())
    {
        // format reference AUXCASE: what follows selects for an auxiliary model.
        if (subcase.kind == model::SubcaseKind::AuxiliaryCase)
            break;
        const model::CaseEntry* entry = model::CaseControl::ownCommand(subcase, "NonStructMassSelection");
        if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer && entry->value.integer > 0)
            return static_cast<base::EntityId>(entry->value.integer);
    }
    return std::nullopt;
}

NonStructuralMass NonStructuralMass::resolve(const model::Model& model, base::EntityId set, base::ThreadPool* pool)
{
    if (!exists(model, set))
    {
        throw MassError("the non-structural mass set " + std::to_string(set) + " is not defined by any " +
                        std::string("NonStructMass") + ", " + std::string("NonStructMass1") + ", " +
                        std::string("NonStructMassLumped") + ", " + std::string("NonStructMassLumped1") + " or " +
                        std::string("NonStructMassUnion") + " card");
    }
    NonStructuralMass result;
    result.m_set = set;
    const std::vector<Entry> entries = entriesOf(model, memberSets(model, set));
    if (entries.empty())
        return result;

    const ElementIndex index(model, pool);
    result.m_added.assign(model::Model::typeCount(), {});
    const auto add = [&](const Item& item, double mass)
    {
        std::vector<double>& column = result.m_added[item.type];
        if (column.size() <= item.row)
            column.resize(static_cast<std::size_t>(item.row) + 1, 0.0);
        column[item.row] += mass;
        result.m_total += mass;
    };
    for (const Entry& entry : entries)
    {
        const std::vector<std::size_t> matched = matchedItems(entry, index);
        if (!entry.lumped)
        {
            for (const std::size_t position : matched)
            {
                const Item& item = index.item(position);
                if (item.measure == item.measure)
                    add(item, entry.value * item.measure);
            }
            continue;
        }
        // A lumped mass is shared by the elements in proportion to their area or length.
        double measure = 0.0;
        bool areas = false;
        bool lengths = false;
        for (const std::size_t position : matched)
        {
            const Item& item = index.item(position);
            if (item.measure != item.measure)
                continue;
            measure += item.measure;
            (isArea(item.family) ? areas : lengths) = true;
        }
        if (areas && lengths)
        {
            throw MassError(
                "the lumped non-structural mass of set " + std::to_string(set) + " (" + std::string(entry.type) +
                ") is shared by line elements and area elements, which the format reference does not allow");
        }
        if (!(measure > 0.0))
            continue;
        for (const std::size_t position : matched)
        {
            const Item& item = index.item(position);
            if (item.measure == item.measure)
                add(item, entry.value * item.measure / measure);
        }
    }
    return result;
}

} // namespace oor::analysis
