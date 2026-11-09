#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"
#include "oor/query/providers/ModelSource.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The rows, column by column: a value and a validity flag for each nullable column.
struct AllowableRows
{
    std::vector<std::int32_t> eid;
    std::vector<std::string> type;
    std::vector<std::int32_t> pid;
    std::vector<std::uint8_t> pidValid;
    std::vector<std::int32_t> layer;
    std::vector<std::uint8_t> layerValid;
    std::vector<std::int32_t> plyId;
    std::vector<std::uint8_t> plyIdValid;
    std::vector<std::int32_t> mid;
    std::vector<std::uint8_t> midValid;
    std::vector<double> allowable;
    std::vector<std::string> source;
    // st, sc, ss, xt, xc, yt, yc, s, f12, ta, sa, sb
    std::array<std::vector<double>, 12> values;
    std::vector<std::uint8_t> strainAllowables;
    std::vector<std::uint8_t> strainValid;

    void add(std::int32_t elementId, const std::string& cardType, std::int32_t propertyId, std::int32_t layerNumber,
             std::int32_t globalPly, std::int32_t materialId, double resolved, const std::string& resolvedFrom,
             const std::array<double, 12>& raw, std::optional<bool> strain)
    {
        eid.push_back(elementId);
        type.push_back(cardType);
        pid.push_back(propertyId);
        pidValid.push_back(propertyId != 0 ? 1 : 0);
        layer.push_back(layerNumber);
        layerValid.push_back(layerNumber != 0 ? 1 : 0);
        plyId.push_back(globalPly);
        plyIdValid.push_back(globalPly != 0 ? 1 : 0);
        mid.push_back(materialId);
        midValid.push_back(materialId != 0 ? 1 : 0);
        allowable.push_back(resolved);
        source.push_back(resolvedFrom);
        for (std::size_t column = 0; column < raw.size(); ++column)
            values[column].push_back(raw[column]);
        strainAllowables.push_back(strain.value_or(false) ? 1 : 0);
        strainValid.push_back(strain ? 1 : 0);
    }
};

std::array<double, 12> blank()
{
    std::array<double, 12> raw;
    raw.fill(NOT_A_NUMBER);
    return raw;
}

// A field of a material card as an allowable: NaN for a blank (0) field.
double given(double value)
{
    return value > 0.0 ? value : NOT_A_NUMBER;
}

// elementAllowables: the allowables the margins use for each element (and ply), resolved without results (the
// building blocks for the plugin packages; a companion package). The rules are those of `margins` and come
// from the same functions (MaterialAllowables.hpp): the caller's allowables by element, property, material or for all,
// else the MAT1 field (ST in tension, SC in compression), the MAT8 of each ply (the failure index is limited to 1), the
// caller's pair (Ta, Sa) for a fastener.
class ElementAllowablesQuery final : public QueryBase
{
public:
    ElementAllowablesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        const std::string criterion = run.arguments.textOr("criterion", "");
        if (criterion != "vonMises" && criterion != "maxPrincipal" && criterion != "minPrincipal" &&
            criterion != "composite" && criterion != "bond" && criterion != "fastener")
            throw QueryError(
                "elementAllowables: criterion is \"vonMises\", \"maxPrincipal\", \"minPrincipal\", "
                "\"composite\", \"bond\" or \"fastener\", not '" +
                criterion + "'");
        const CallerAllowables caller(run.arguments.values("allowables"));
        const std::vector<std::uint16_t> types =
            criterion == "fastener" ? fastenerElementTypes() : typesOfCategory(model::CardCategory::Element);
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        ElementData data(run.model, list, run.context);
        const std::vector<std::int32_t>& pids = data.properties();
        CardFacts facts(model);
        const std::vector<std::int32_t> mids = mainMaterials(model, facts, list, pids);

        AllowableRows rows;
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            const std::int32_t eid = list.ids[index];
            const std::string card(typeName(list.types[index]));
            const std::int32_t pid = pids[index];
            if (criterion == "fastener")
            {
                std::array<double, 12> raw = blank();
                const auto tension = caller.find(eid, pid, 0, 0);
                const auto shear = caller.find(eid, pid, 0, 1);
                raw[9] = tension.value_or(NOT_A_NUMBER);
                raw[10] = shear.value_or(NOT_A_NUMBER);
                const std::string source = tension && shear ? "caller"
                                                            : "no allowable: a fastener takes the pair (Ta, Sa) the "
                                                              "caller gives";
                rows.add(eid, card, pid, 0, 0, 0, NOT_A_NUMBER, source, raw, std::nullopt);
                continue;
            }
            if (criterion == "composite" || criterion == "bond")
            {
                addPlies(model, rows, caller, eid, card, pid, criterion == "bond");
                continue;
            }
            // Metallic: an element with no material (a spring, a rigid element) is not judged by these criteria.
            const std::int32_t mid = mids[index];
            if (mid == 0)
                continue;
            const bool compression = criterion == "minPrincipal";
            std::array<double, 12> raw = blank();
            std::string source;
            double resolved = NOT_A_NUMBER;
            if (const auto byCaller = caller.find(eid, pid, mid, 0))
            {
                resolved = *byCaller;
                source = "caller";
            }
            else
                resolved = isoMatAllowable(model, mid, compression, source);
            readIsoMat(model, mid, raw);
            rows.add(eid, card, pid, 0, 0, mid, resolved, source, raw, std::nullopt);
        }
        if (rows.eid.empty())
            return emptyTable(info().defaultColumns());
        // A value that is not there (a blank field, no allowable) is null, not NaN.
        const auto validOf = [](const std::vector<double>& values)
        {
            std::vector<std::uint8_t> valid(values.size());
            for (std::size_t row = 0; row < values.size(); ++row)
                valid[row] = std::isnan(values[row]) ? 0 : 1;
            return valid;
        };
        const auto realColumn = [&](TableBuilder& table, const char* name, std::vector<double> values)
        {
            std::vector<std::uint8_t> valid = validOf(values);
            table.real(name, std::move(values), std::move(valid));
        };
        TableBuilder builder;
        builder.int32("elementId", std::move(rows.eid));
        builder.dictionary("type", rows.type);
        builder.int32("propId", std::move(rows.pid), std::move(rows.pidValid));
        builder.int32("layer", std::move(rows.layer), std::move(rows.layerValid));
        builder.int32("plyId", std::move(rows.plyId), std::move(rows.plyIdValid));
        builder.int32("matId", std::move(rows.mid), std::move(rows.midValid));
        realColumn(builder, "allowable", std::move(rows.allowable));
        builder.text("allowableSource", rows.source);
        const char* const names[11] = {"st", "sc", "ss", "xt", "xc", "yt", "yc", "s", "f12", "ta", "sa"};
        for (std::size_t column = 0; column < 9; ++column)
            realColumn(builder, names[column], std::move(rows.values[column]));
        builder.flag("strainAllowables", rows.strainAllowables, rows.strainValid);
        realColumn(builder, "ta", std::move(rows.values[9]));
        realColumn(builder, "sa", std::move(rows.values[10]));
        realColumn(builder, "sb", std::move(rows.values[11]));
        return builder.finish();
    }

private:
    // ST, SC, SS of an IsoMat (MAT1) into the first three columns (NaN for a blank field or another material card).
    static void readIsoMat(const model::Model& model, std::int32_t mid, std::array<double, 12>& raw)
    {
        if (const auto fields = isoMatStressLimits(model, mid))
        {
            raw[0] = given((*fields)[0]);
            raw[1] = given((*fields)[1]);
            raw[2] = given((*fields)[2]);
        }
    }

    // The plies of the laminate of an element, one row each, with the allowables of their MAT8 and the SB of the
    // laminate; `bond`: the bonding index is judged (against 1, when the laminate has an SB).
    static void addPlies(const model::Model& model, AllowableRows& rows, const CallerAllowables& caller,
                         std::int32_t eid, const std::string& card, std::int32_t pid, bool bond)
    {
        if (pid == 0)
            return;
        const auto location = model.locate(base::IdSpace::Property, pid);
        if (!location)
            return;
        const std::optional<LaminateInfo> laminate = readLaminate(model, *location);
        if (!laminate)
            return;
        std::int32_t number = 0;
        for (const PlyInfo& ply : laminate->plies)
        {
            ++number;
            std::array<double, 12> raw = blank();
            const std::optional<PlyMaterial> material = orthoMat2DAllowables(model, ply.material);
            if (material)
            {
                raw[3] = given(material->xt);
                raw[4] = given(material->xc);
                raw[5] = given(material->yt);
                raw[6] = given(material->yc);
                raw[7] = given(material->s);
                raw[8] = material->f12;
            }
            raw[11] = given(laminate->bondAllowable);
            // The margins limit the failure index to 1 unless the caller gives another limit.
            const auto byCaller = caller.find(eid, pid, ply.material, 0);
            if (bond)
            {
                const bool hasBond = laminate->bondAllowable > 0.0;
                const std::string source = byCaller  ? "caller"
                                           : hasBond ? "bonding index (PCOMP SB)"
                                                     : "no SB (the laminate gives no allowable interlaminar shear)";
                rows.add(eid, card, pid, number, ply.globalId, ply.material,
                         byCaller.value_or(hasBond ? 1.0 : NOT_A_NUMBER), source, raw,
                         material ? std::optional<bool>(material->strainAllowables) : std::nullopt);
                continue;
            }
            const std::string source = byCaller
                                           ? "caller"
                                           : (material ? "failure index (MAT8 " + std::to_string(ply.material) + ")"
                                                       : "no MAT8 material (the ply allowables are not read)");
            rows.add(eid, card, pid, number, ply.globalId, ply.material, byCaller.value_or(1.0), source, raw,
                     material ? std::optional<bool>(material->strainAllowables) : std::nullopt);
        }
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementAllowables";
        info.family = "T1";
        info.description =
            "The allowables the margins use for each element (and ply), without results (what margins "
            "divides by, for a caller who forms its own): criterion \"vonMises\" or \"maxPrincipal\" (the MAT1 ST), "
            "\"minPrincipal\" (the MAT1 SC), \"composite\" (one row per ply: the allowables of its MAT8, the failure "
            "index limited to 1), \"bond\" (one row per ply: the bonding index limited to 1, when the laminate has an "
            "SB, given in `sb`), \"fastener\" (the pair Ta, Sa the caller gives). allowables: as in margins, by "
            "(\"element\", elementId), (\"property\", propId), (\"material\", mid) or one value for all, and they come "
            "first. "
            "allowable is the value the margins compare with; allowableSource says where it comes from, or why there "
            "is none (the allowable is then null). The metallic criteria list the elements that have a material.";
        info.parameters = {
            parameter("criterion", ParameterKind::Text, "(required)",
                      "\"vonMises\", \"maxPrincipal\", \"minPrincipal\", \"composite\", \"bond\" or \"fastener\""),
            parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
            parameter(
                "allowables", ParameterKind::Values, "None",
                "a dict keyed by (\"material\", mid), (\"property\", propId) or (\"element\", elementId), or one value "
                "for all; pairs (tension, shear) for fasteners")};
        info.defaultSort = "elementId, layer";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("type").int32("propId").int32("layer").int32("plyId").int32("matId");
        columns.real("allowable").text("allowableSource");
        columns.reals("st sc ss xt xc yt yc s f12").flag("strainAllowables").reals("ta sa sb");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addElementAllowablesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementAllowablesQuery>());
}

} // namespace oor::query
