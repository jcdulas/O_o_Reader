#include "oor/analysis/mass/Laminate.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"

namespace oor::query
{

namespace
{

// The laminate cards of a model (PCOMP and PCOMPG) in order of property ID.
CardList laminateCards(const model::Model& model, const std::optional<std::vector<std::int64_t>>& ids)
{
    std::vector<std::uint16_t> types;
    for (const std::uint16_t type : typesInSpace(model::CardCategory::Property, base::IdSpace::Property))
    {
        if (analysis::Laminate::supports(type))
            types.push_back(type);
    }
    return listCards(model, types, ids);
}

// laminatePliesTable: the stack of each laminate, ply by ply.
class LaminatePliesTableQuery final : public QueryBase
{
public:
    LaminatePliesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const CardList list = laminateCards(model, run.arguments.ids("propertyIds"));
        std::vector<std::int32_t> pid;
        std::vector<std::string> type;
        std::vector<std::int32_t> ply;
        std::vector<std::int32_t> plyId;
        std::vector<std::uint8_t> plyIdValid;
        std::vector<std::int32_t> mid;
        std::vector<double> thickness;
        std::vector<double> theta;
        std::vector<std::uint8_t> sout;
        std::vector<double> zBottom;
        std::vector<double> zTop;
        std::vector<std::uint8_t> mirrored;
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            const auto info = readLaminate(model, list.location(index));
            if (!info)
                continue;
            std::int32_t number = 0;
            for (const PlyInfo& item : info->plies)
            {
                ++number;
                pid.push_back(info->pid);
                type.push_back(info->type);
                ply.push_back(number);
                plyId.push_back(item.globalId);
                plyIdValid.push_back(info->hasGlobalIds ? 1 : 0);
                mid.push_back(item.material);
                thickness.push_back(item.thickness);
                theta.push_back(item.angle);
                sout.push_back(item.stressOutput ? 1 : 0);
                zBottom.push_back(item.zBottom);
                zTop.push_back(item.zTop);
                mirrored.push_back(item.mirrored ? 1 : 0);
            }
        }
        TableBuilder builder;
        builder.int32("propId", std::move(pid));
        builder.dictionary("type", type);
        builder.int32("ply", std::move(ply));
        builder.int32("plyId", std::move(plyId), std::move(plyIdValid));
        builder.int32("matId", std::move(mid));
        builder.real("thickness", std::move(thickness));
        builder.real("theta", std::move(theta));
        builder.flag("sout", sout);
        builder.real("zBottom", std::move(zBottom));
        builder.real("zTop", std::move(zTop));
        builder.flag("isMirrored", mirrored);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "laminatePliesTable";
        info.family = "T12";
        info.description =
            "The stack of each laminate (PCOMP and PCOMPG), ply by ply from the bottom: the material, thickness and "
            "fibre angle "
            "(degrees), whether stresses are output (sout), the global ply ID of a PCOMPG (null for a PCOMP), the z of "
            "the "
            "bottom and top face from the reference plane (from Z0, as the stiffness of the laminate uses them) and "
            "isMirrored for the plies the SYM option adds. The plies of a PCOMP are not editable columns: the table "
            "has no "
            "setter for a ply, edit the property.";
        info.parameters = {parameter("propertyIds", ParameterKind::IdList, "None", "keep these laminates")};
        info.defaultSort = "propId, ply";
        ColumnListBuilder columns;
        columns.int32("propId")
            .dictionary("type")
            .int32("ply")
            .int32("plyId")
            .int32("matId")
            .real("thickness")
            .real("theta");
        columns.flag("sout").real("zBottom").real("zTop").flag("isMirrored");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addLaminatePliesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LaminatePliesTableQuery>());
}

} // namespace oor::query
