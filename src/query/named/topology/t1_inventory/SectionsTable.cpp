#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/mass/SectionCache.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The properties a task of the pool takes at once: a library section not computed yet costs some milliseconds (the
// finite element formulation of its shear factors), one already in the model's cache a lookup.
constexpr std::size_t SECTIONS_PER_TASK = 4;

// The property types of the table.
std::vector<std::uint16_t> sectionTypes()
{
    return {model::typeIndexOf<model::BarPropTable>(),  model::typeIndexOf<model::BarShapePropTable>(),
            model::typeIndexOf<model::BeamPropTable>(), model::typeIndexOf<model::BeamShapePropTable>(),
            model::typeIndexOf<model::RodPropTable>(),  model::typeIndexOf<model::TubePropTable>()};
}

analysis::ShearFactorRule ruleNamed(const model::Model& model, const std::string& name)
{
    if (name == "deck")
        return analysis::SectionFormulas::ruleOf(model);
    if (name == "solver")
        return analysis::ShearFactorRule::Solver;
    if (name == "beamLibrary")
        return analysis::ShearFactorRule::BeamLibrary;
    throw QueryError("sectionsTable: shearRule must be \"deck\", \"solver\" or \"beamLibrary\", found \"" + name +
                     "\"");
}

// sectionsTable: the section properties of the bar, beam, rod and tube properties, per station.
class SectionsTableQuery final : public QueryBase
{
public:
    SectionsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        std::vector<std::uint16_t> types = sectionTypes();
        if (!typeNames.empty())
        {
            const std::vector<std::uint16_t> named = typesNamed(typeNames, model::CardCategory::Property, "types");
            std::erase_if(
                types, [&](std::uint16_t type) { return std::find(named.begin(), named.end(), type) == named.end(); });
        }
        const analysis::ShearFactorRule rule = ruleNamed(model, run.arguments.textOr("shearRule", "deck"));
        const CardList list = listCards(model, types, run.arguments.ids("propertyIds"));
        // PERFORMANCE: the sections of the properties are computed on the threads of the query, each distinct library
        // section once, then kept in the model's cache (ComponentCache::sections) for its later queries and its copies:
        // a section is a function of its type, dimensions and rule, so the table has the same bits on any number of
        // threads and whether its sections are computed or taken from the cache.
        // Measured: a companion package's demo barrel, its 135 PBARL sized apart: 0.39 s a query before; now 0.13 s the
        // first on 4 threads (0.07 s on the default pool of 12), 0.4 ms a query again and on an edited copy.
        analysis::SectionCache& cache = analysis::ComponentCache::of(model).sections();
        cache.nextBatch();
        std::vector<std::optional<analysis::BeamSection>> sections(list.size());
        run.context.parallel(list.size(), SECTIONS_PER_TASK,
                             [&](base::IndexRange range)
                             {
                                 for (std::size_t index = range.begin; index < range.end; ++index)
                                     sections[index] =
                                         analysis::SectionFormulas::of(model, list.location(index), rule, &cache);
                             });
        cache.trim();
        std::vector<std::int32_t> pid;
        std::vector<std::string> type;
        std::vector<std::string> sectionType;
        std::vector<std::uint8_t> sectionTypeValid;
        std::vector<std::int32_t> station;
        constexpr std::size_t VALUES = 17; // xxb area i1 i2 i12 j nsm k1 k2 c1 c2 d1 d2 e1 e2 f1 f2
        std::array<std::vector<double>, VALUES> values;
        const std::uint16_t rodType = model::typeIndexOf<model::RodPropTable>();
        const std::uint16_t tubeType = model::typeIndexOf<model::TubePropTable>();
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            const std::optional<analysis::BeamSection>& section = sections[index];
            if (!section)
                continue;
            const bool isRod = list.types[index] == rodType || list.types[index] == tubeType;
            for (std::size_t at = 0; at < section->stations.size(); ++at)
            {
                const analysis::SectionProperties& properties = section->stations[at];
                pid.push_back(list.ids[index]);
                type.push_back(section->propertyType);
                sectionType.push_back(section->sectionType);
                sectionTypeValid.push_back(section->sectionType.empty() ? 0 : 1);
                station.push_back(static_cast<std::int32_t>(at));
                // A rod (PROD, PTUBE) carries no shear: no shear factor.
                const std::array<double, VALUES> row{properties.xxb,
                                                     properties.area,
                                                     properties.i1,
                                                     properties.i2,
                                                     properties.i12,
                                                     properties.j,
                                                     properties.nsm,
                                                     isRod ? NOT_A_NUMBER : properties.k1,
                                                     isRod ? NOT_A_NUMBER : properties.k2,
                                                     properties.recovery[0][0],
                                                     properties.recovery[0][1],
                                                     properties.recovery[1][0],
                                                     properties.recovery[1][1],
                                                     properties.recovery[2][0],
                                                     properties.recovery[2][1],
                                                     properties.recovery[3][0],
                                                     properties.recovery[3][1]};
                for (std::size_t item = 0; item < VALUES; ++item)
                    values[item].push_back(row[item]);
            }
        }
        TableBuilder builder;
        builder.int32("propId", std::move(pid));
        builder.dictionary("type", type);
        builder.dictionary("sectionType", sectionType, std::move(sectionTypeValid));
        builder.int32("station", std::move(station));
        static const std::array<const char*, VALUES> NAMES{"xxb", "area", "i1", "i2", "i12", "j",  "nonStructMass",
                                                           "k1",  "k2",   "c1", "c2", "d1",  "d2", "e1",
                                                           "e2",  "f1",   "f2"};
        for (std::size_t item = 0; item < VALUES; ++item)
            builder.real(NAMES[item], std::move(values[item]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "sectionsTable";
        info.family = "T1";
        info.description =
            "The section properties of the PBAR, PBARL, PBEAM, PBEAML, PROD and PTUBE properties, one row per property "
            "and station (station 0 for a property with one section; the stations of a PBEAM or PBEAML in the order "
            "of X/XB, xxb from 0 at end A to 1 at end B): the area, I1 (the integral of y squared, plane 1), I2, "
            "I12, the torsional constant J, the non-structural mass per unit length, the shear factors K1 (shear "
            "along the element y axis) and K2, and the stress recovery points C, D, E, F as (y, z) from the neutral "
            "axis (c1 c2 d1 d2 e1 e2 f1 f2). PBAR, PBEAM, PROD and PTUBE give the values of the card (a blank K1 or "
            "K2 of a PBAR is null: no shear flexibility; a CBAR ignores K1 and K2 when "
            "I12 is not "
            "zero); PBARL and PBEAML the formulas of their section type (sectionType). The shear factors of a "
            "library section are those the solver computes: shearRule \"deck\" follows the deck (MDLPRM,TWBRBML,1 "
            "asks "
            "for the beam library equations, else the solver's finite element formulation), \"solver\" and "
            "\"beamLibrary\" "
            "force one. PROD and PTUBE have no shear factor (a rod carries no shear). The sections are computed on the "
            "threads of the call, each distinct library section once, and kept by the model for its later calls, "
            "across its edits and for its copies (a library section is a function of its type, dimensions and rule: "
            "the same bits on any threads and as computed afresh).";
        info.parameters = {
            parameter("propertyIds", ParameterKind::IdList, "None", "keep these properties"),
            parameter("types", ParameterKind::TextList, "None", "keep these property types"),
            parameter("shearRule", ParameterKind::Text, "\"deck\"", "\"deck\", \"solver\" or \"beamLibrary\"")};
        info.defaultSort = "propId, station";
        ColumnListBuilder columns;
        columns.int32("propId").dictionary("type").dictionary("sectionType").int32("station");
        columns.reals("xxb area i1 i2 i12 j nsm k1 k2 c1 c2 d1 d2 e1 e2 f1 f2");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSectionsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SectionsTableQuery>());
}

} // namespace oor::query
