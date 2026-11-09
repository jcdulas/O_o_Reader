#include "oor/analysis/quality/GeometryCheckThresholds.hpp"
#include "oor/analysis/quality/MeshQuality.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

#include <map>

namespace oor::query
{

namespace
{

// qualityTable: the metrics of each shell and solid with a verdict against the GEOMCHECK thresholds.
class QualityTableQuery final : public QueryBase
{
public:
    QualityTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        std::vector<std::uint16_t> types;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        const std::vector<std::uint16_t> wanted = typeNames.empty()
                                                      ? typesOfCategory(model::CardCategory::Element)
                                                      : typesNamed(typeNames, model::CardCategory::Element, "types");
        for (const std::uint16_t type : wanted)
        {
            if (analysis::MeshQuality::supports(type))
                types.push_back(type);
        }
        analysis::GeometryCheckThresholds thresholds = analysis::GeometryCheckThresholds::fromModel(model);
        for (const auto& [keyword, value] : run.arguments.thresholds("thresholds"))
        {
            if (!thresholds.set(keyword, value))
                throw QueryError(
                    "qualityTable: '" + keyword +
                    "' is not a GEOMCHECK tolerance (Q4_SKEW, Q4_TAPER, Q4_WARP, Q4_IAMIN, Q4_IAMAX, T3_SKEW, "
                    "T3_IAMAX, TET_AR, HEX_WARP, ...)");
        }
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        const std::size_t count = list.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        // The metrics, a card type at a time (one broken element does not hide the others).
        std::vector<analysis::QualityValues> values(count);
        std::map<std::uint16_t, std::pair<std::vector<std::uint32_t>, std::vector<std::size_t>>> groups;
        for (std::size_t index = 0; index < count; ++index)
        {
            auto& group = groups[list.types[index]];
            group.first.push_back(list.rows[index]);
            group.second.push_back(index);
        }
        for (const auto& [type, group] : groups)
        {
            run.context.checkCancelled();
            try
            {
                const analysis::QualityArrays arrays =
                    analysis::MeshQuality::compute(model, type, group.first, run.context.pool());
                for (std::size_t item = 0; item < group.second.size(); ++item)
                    values[group.second[item]] = arrays.at(item);
            }
            catch (const base::OperationCancelled&)
            {
                throw;
            }
            catch (const base::OorError&)
            {
                for (std::size_t item = 0; item < group.second.size(); ++item)
                {
                    try
                    {
                        values[group.second[item]] =
                            analysis::MeshQuality::of(model, model::CardLocation{type, group.first[item]});
                    }
                    catch (const base::OperationCancelled&)
                    {
                        throw;
                    }
                    catch (const base::OorError&)
                    {
                    }
                }
            }
        }
        // The verdict.
        std::vector<std::uint8_t> failed(count);
        std::vector<std::int64_t> nameOffsets{0};
        std::vector<std::string> names;
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint16_t bits =
                thresholds.failedChecks(analysis::ElementTopology::shapeOf(list.types[index]), values[index]);
            failed[index] = bits != 0 ? 1 : 0;
            for (const analysis::QualityCheck check : analysis::allQualityChecks())
            {
                if ((bits & analysis::bit(check)) != 0)
                    names.emplace_back(analysis::qualityCheckName(check));
            }
            nameOffsets.push_back(static_cast<std::int64_t>(names.size()));
        }
        std::vector<std::size_t> kept;
        for (std::size_t index = 0; index < count; ++index)
        {
            if (!run.arguments.boolOr("onlyFailed", false) || failed[index] != 0)
                kept.push_back(index);
        }
        ListColumns columns(run.model, list, run.context);
        const std::vector<std::int32_t> pids = asInt32(columns.get("propId"));
        const std::size_t shown = kept.size();
        std::vector<std::int32_t> eid(shown);
        std::vector<std::string> type(shown);
        std::vector<std::int32_t> pid(shown);
        std::vector<std::uint8_t> pidValid(shown);
        std::vector<std::uint8_t> failedShown(shown);
        std::array<std::vector<double>, 7> metrics;
        for (auto& column : metrics)
            column.resize(shown);
        std::vector<std::int64_t> shownOffsets{0};
        std::vector<std::string> shownNames;
        for (std::size_t at = 0; at < shown; ++at)
        {
            const std::size_t index = kept[at];
            const analysis::QualityValues& value = values[index];
            eid[at] = list.ids[index];
            type[at] = std::string(typeName(list.types[index]));
            pid[at] = pids[index];
            pidValid[at] = pids[index] != 0 ? 1 : 0;
            failedShown[at] = failed[index];
            metrics[0][at] = value.aspectRatio;
            metrics[1][at] = value.skew;
            metrics[2][at] = value.warp;
            metrics[3][at] = value.taper;
            metrics[4][at] = value.minAngle;
            metrics[5][at] = value.maxAngle;
            metrics[6][at] = value.jacobianRatio;
            for (std::int64_t name = nameOffsets[index]; name < nameOffsets[index + 1]; ++name)
                shownNames.push_back(names[static_cast<std::size_t>(name)]);
            shownOffsets.push_back(static_cast<std::int64_t>(shownNames.size()));
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.int32("propId", std::move(pid), std::move(pidValid));
        static const char* const NAMES[7] = {"aspectRatio", "skew",     "warp",         "taper",
                                             "minAngle",    "maxAngle", "jacobianRatio"};
        for (std::size_t item = 0; item < 7; ++item)
            builder.real(NAMES[item], std::move(metrics[item]));
        builder.flag("failed", failedShown);
        builder.textList("failedChecks", std::move(shownOffsets), shownNames);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "qualityTable";
        info.family = "T5";
        info.description =
            "The quality metrics of each shell and solid (aspect ratio, skew, warp, taper, the smallest and largest "
            "interior "
            "angle, the Jacobian ratio) with a verdict: failed, and failedChecks, the names of the GEOMCHECK tests "
            "the element "
            "fails (aspectRatio, skew, warp, taper, minAngle, maxAngle, jacobian, edgePointLengthRatio, "
            "edgePointAngle, faceWarp). A metric that does not apply to the element is null. `thresholds` is a dict "
            "of "
            "GEOMCHECK keywords (Q4_SKEW, Q4_TAPER, Q4_WARP, Q4_IAMIN, Q4_IAMAX, T3_SKEW, T3_IAMAX, TET_AR, HEX_WARP "
            "...) to "
            "limits; the others take the format reference defaults, changed by the deck's own GEOMCHECK statements. "
            "The 14 metrics "
            "themselves are the computed columns of `model.query(\"elements\")`.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
                           parameter("types", ParameterKind::TextList, "None", "keep these card types"),
                           parameter("thresholds", ParameterKind::Thresholds, "None", "GEOMCHECK keyword -> limit"),
                           parameter("onlyFailed", ParameterKind::Bool, "False", "keep the elements that fail a test")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("type").int32("propId");
        columns.reals("aspectRatio skew warp taper minAngle maxAngle jacobianRatio");
        columns.flag("failed").textList("failedChecks");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addQualityTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<QualityTableQuery>());
}

} // namespace oor::query
