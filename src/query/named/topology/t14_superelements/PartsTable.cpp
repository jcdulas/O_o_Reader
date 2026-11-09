#include "oor/analysis/superelements/ExternalSuperelementFiles.hpp"
#include "oor/analysis/superelements/ListSuperelementPartition.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t14_superelements/SuperelementQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <tuple>

namespace oor::query
{

namespace
{

// The part kinds in the order of the table; another kind sorts last.
constexpr std::array<std::string_view, 10> PART_KINDS{
    "SuperelementPart", "AuxiliaryModel", "AcousticFieldPointMesh", "ArbitraryModel", "MassModel",
    "FlexibleBody",     "TrimComponent",  "ModuleSection",          "ActivePart",     "PassivePart"};

std::size_t kindRank(std::string_view kind) noexcept
{
    const auto found = std::find(PART_KINDS.begin(), PART_KINDS.end(), kind);
    return static_cast<std::size_t>(found - PART_KINDS.begin());
}

// The live cards of a model in one ID space (the grid, scalar and extra points: IdSpace::Node; the elements:
// IdSpace::Element).
std::int32_t countCards(const model::Model& model, base::IdSpace space)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::size_t count = 0;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table != nullptr && registry.entry(type).info.idSpace == space)
            count += table->rows().aliveCount();
    }
    return static_cast<std::int32_t>(count);
}

struct PartRow
{
    std::string kind;
    std::int32_t id = 0;
    std::int32_t massId = 0;
    const analysis::SuperelementEntry* entry = nullptr; // the superelement the row describes (no MASSID)
    const model::Model* section = nullptr;              // its BEGIN section, null when it has none
};

// partsTable: the parts of the deck and the superelements that have no part of their own.
class PartsTableQuery final : public QueryBase
{
public:
    PartsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& residual = run.model->root();
        const analysis::SuperelementCatalogue catalogue(residual);
        const analysis::ListSuperelementPartition partition(catalogue);
        std::vector<PartRow> rows;
        for (const auto& part : residual.parts())
        {
            const model::PartInfo& info = *part->partInfo();
            PartRow row{info.kind, static_cast<std::int32_t>(info.id), static_cast<std::int32_t>(info.massId), nullptr,
                        part.get()};
            if (info.kind == "SuperelementPart" && info.massId == 0)
                row.entry = catalogue.find(static_cast<base::EntityId>(info.id));
            rows.push_back(row);
        }
        for (const analysis::SuperelementEntry& entry : catalogue.entries())
        {
            if (entry.part == nullptr)
                rows.push_back(PartRow{"SuperelementPart", entry.seid, 0, &entry, nullptr});
        }
        std::stable_sort(rows.begin(), rows.end(),
                         [](const PartRow& left, const PartRow& right)
                         {
                             return std::tuple(kindRank(left.kind), left.id, left.massId) <
                                    std::tuple(kindRank(right.kind), right.id, right.massId);
                         });

        const std::size_t count = rows.size();
        std::vector<std::string> kind(count);
        std::vector<std::int32_t> id(count);
        std::vector<std::int32_t> massId(count);
        std::vector<std::string> type(count);
        std::vector<std::uint8_t> typeValid(count, 0);
        std::vector<std::int32_t> downstream(count, 0);
        std::vector<std::uint8_t> downstreamValid(count, 0);
        std::vector<std::string> method(count);
        std::vector<std::uint8_t> methodValid(count, 0);
        std::vector<double> tolerance(count, std::numeric_limits<double>::quiet_NaN());
        std::vector<std::int32_t> nodes(count, 0);
        std::vector<std::int32_t> elements(count, 0);
        std::vector<std::int32_t> cards(count, 0);
        std::vector<std::uint8_t> nodesValid(count, 0);
        std::vector<std::uint8_t> cardsValid(count, 0);
        std::vector<std::uint8_t> external(count, 0);
        std::vector<std::string> file(count);
        std::vector<std::uint8_t> fileValid(count, 0);
        std::vector<std::int32_t> imageOf(count, 0);
        std::vector<std::uint8_t> imageValid(count, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const PartRow& row = rows[index];
            kind[index] = row.kind;
            id[index] = row.id;
            massId[index] = row.massId;
            // The points and elements: of the section; of the primary for an image without one; of the partition of
            // the main Bulk Data for a list superelement.
            const model::Model* counted = row.section;
            if (row.entry != nullptr && row.entry->hasPartPoints() && (counted == nullptr || row.entry->isImage()))
                counted = row.entry->idModel;
            if (counted != nullptr)
            {
                nodes[index] = countCards(*counted, base::IdSpace::Node);
                elements[index] = countCards(*counted, base::IdSpace::Element);
                nodesValid[index] = 1;
            }
            else if (row.entry != nullptr && row.entry->type == analysis::SuperelementType::List)
            {
                nodes[index] = static_cast<std::int32_t>(partition.interiorCount(row.entry->seid));
                elements[index] = static_cast<std::int32_t>(partition.elementCount(row.entry->seid));
                nodesValid[index] = 1;
            }
            if (row.section != nullptr)
            {
                cards[index] = static_cast<std::int32_t>(row.section->cardCount());
                cardsValid[index] = 1;
            }
            const analysis::SuperelementEntry* entry = row.entry;
            if (entry == nullptr)
                continue;
            type[index] = analysis::superelementTypeName(entry->type);
            typeValid[index] = 1;
            downstream[index] = entry->downstream;
            downstreamValid[index] = 1;
            method[index] = entry->method == model::SuperelementBulkOptionsMethod::Manual ? "Manual" : "Automatic";
            methodValid[index] = 1;
            tolerance[index] = entry->tolerance;
            external[index] = entry->isExternal() ? 1 : 0;
            if (const std::optional<std::string> assigned =
                    analysis::ExternalSuperelementFiles::assignedFile(residual, *entry))
            {
                file[index] = *assigned;
                fileValid[index] = 1;
            }
            if (entry->imageOf != 0)
            {
                imageOf[index] = entry->imageOf;
                imageValid[index] = 1;
            }
        }
        std::vector<std::uint8_t> elementsValid = nodesValid;
        TableBuilder builder;
        builder.dictionary("partKind", kind);
        builder.int32("partId", std::move(id));
        builder.int32("massId", std::move(massId));
        builder.dictionary("superelementType", type, std::move(typeValid));
        builder.int32("downstreamId", std::move(downstream), std::move(downstreamValid));
        builder.dictionary("method", method, std::move(methodValid));
        builder.real("tolerance", std::move(tolerance));
        builder.int32("nodeCount", std::move(nodes), std::move(nodesValid));
        builder.int32("elementCount", std::move(elements), std::move(elementsValid));
        builder.int32("cardCount", std::move(cards), std::move(cardsValid));
        builder.flag("isExternal", external);
        builder.text("assignedFile", file, std::move(fileValid));
        builder.int32("imageOf", std::move(imageOf), std::move(imageValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "partsTable";
        info.family = "T14";
        info.description =
            "The parts of the deck (BEGIN SUPER, AUXMODEL, AFPM, ARBMODEL, MASSID, FLXBDY, TRMC, MODULE, ACTIVE, "
            "PASSIVE; partKind the neutral name of the kind: SuperelementPart, AuxiliaryModel, ...; a part BEGIN "
            "SUPER=n "
            "MASSID=m is the mass increment m of superelement n, keyed by massId) and the superelements that have no "
            "BEGIN section (list superelements of SESET, the SEID of GRID, SEELT and CSUPEXT; REPEAT and MIRROR "
            "images; "
            "CSUPER secondaries; external superelements without a part), one row each. For a superelement: its type "
            "(the neutral SEBULK TYPE word: Primary, List, Repeat, Mirror, External, ExternalBinaryResults, "
            "ExternalMatrixFile, ExternalPunchFile, ExternalReducedModel, CollectorSuperelement, Secondary), the "
            "superelement downstream of it (its SETREE, else 0, the residual structure), the boundary search of its "
            "SEBULK (method Automatic or Manual) and its tolerance (the SEBULK TOL, else PARAM CONFAC, else 1.0E-5), "
            "whether "
            "it is external and the file the ASSIGN statement of its SEBULK UNITNO names (external binary results, "
            "external matrix file, EXTROM), the "
            "part an image copies (REPEAT, MIRROR: RSEID; CSUPER: PSID). nodeCount and elementCount count the "
            "points and elements of the BEGIN section (of the primary for an image, of the partition of the main Bulk "
            "Data for a list superelement: its interior points and its elements); cardCount the cards of the BEGIN "
            "section, null without one. The superelement columns are null for the parts that are not a superelement "
            "and for the mass increments.";
        info.defaultSort =
            "partKind (SuperelementPart, AuxiliaryModel, AcousticFieldPointMesh, ArbitraryModel, "
            "MassModel, FlexibleBody, TrimComponent, ModuleSection, ActivePart, PassivePart), partId, "
            "massId";
        ColumnListBuilder columns;
        columns.dictionary("partKind").int32("partId").int32("massId").dictionary("superelementType");
        columns.int32("downstreamId").dictionary("method").real("tolerance").int32("nodeCount");
        columns.int32("elementCount").int32("cardCount").flag("isExternal").text("assignedFile");
        columns.int32("imageOf");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPartsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PartsTableQuery>());
}

} // namespace oor::query
