#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"
#include "oor/model/cards/thermal/forcedConvectionProp/ForcedConvectionPropTable.hpp"
#include "oor/model/cards/thermal/freeConvection/FreeConvectionTable.hpp"
#include "oor/model/cards/thermal/freeConvectionProp/FreeConvectionPropTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The free convection coefficient H of the MAT4 with this ID, NaN when there is none or it is blank.
double materialCoefficient(const model::Model& model, base::EntityId mid)
{
    const auto location = model.locate(base::IdSpace::ThermalMaterial, mid);
    if (!location || location->table != model::typeIndexOf<model::HeatIsoMatTable>())
        return NOT_A_NUMBER;
    const model::HeatIsoMatCard card =
        static_cast<const model::HeatIsoMatTable&>(*model.findTable(location->table)).card(location->row);
    return card.convectionCoefficient.value_or(NOT_A_NUMBER);
}

// convectionTable: the convection boundary conditions of the surface elements, free (CONV) and forced (CONVM), with
// the property that says how the heat transfer coefficient is found.
class ConvectionTableQuery final : public QueryBase
{
public:
    ConvectionTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<std::uint16_t> types = {model::typeIndexOf<model::FreeConvectionTable>(),
                                                  model::typeIndexOf<model::ForcedConvectionTable>()};
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());
        const std::size_t count = list.size();

        std::vector<std::string> card(count);
        std::vector<std::int32_t> pconid(count, 0);
        std::vector<std::int32_t> film(count, 0);
        std::vector<std::int32_t> control(count, 0);
        std::vector<std::vector<std::int32_t>> ambient(count);
        std::vector<std::int32_t> mid(count, 0);
        std::vector<double> coefficient(count, NOT_A_NUMBER);
        std::vector<std::int32_t> form(count, 0);
        std::vector<std::uint8_t> formValid(count, 0);
        std::vector<double> exponent(count, NOT_A_NUMBER);
        std::vector<std::int32_t> massFlow(count, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::CardLocation where = list.location(index);
            const model::CardTable& table = *model.findTable(where.table);
            card[index] = std::string(typeName(where.table));
            if (where.table == model::typeIndexOf<model::FreeConvectionTable>())
            {
                const model::FreeConvectionCard data =
                    static_cast<const model::FreeConvectionTable&>(table).card(where.row);
                pconid[index] = data.convectionPropId;
                film[index] = data.filmNode;
                control[index] = data.controlNode;
                ambient[index].assign(data.ambientNodes.begin(), data.ambientNodes.end());
                const auto property =
                    model.locateInType(model::typeIndexOf<model::FreeConvectionPropTable>(), data.convectionPropId);
                if (!property)
                    continue;
                const model::FreeConvectionPropCard values =
                    static_cast<const model::FreeConvectionPropTable&>(*model.findTable(property->table))
                        .card(property->row);
                mid[index] = values.matId;
                form[index] = values.matrixForm;
                formValid[index] = 1;
                exponent[index] = values.convectionExponent;
                // The alternate layout (FTYPE 3) gives the coefficients on the card; the others take H of the MAT4.
                coefficient[index] = values.formulaKind == 3 ? values.convectionCoefficient[0]
                                                             : materialCoefficient(model, values.matId);
            }
            else
            {
                const model::ForcedConvectionCard data =
                    static_cast<const model::ForcedConvectionTable&>(table).card(where.row);
                pconid[index] = data.convectionPropId;
                film[index] = data.filmNode;
                ambient[index] = {data.ambientNode1, data.ambientNode2};
                massFlow[index] = data.massFlowNode;
                const auto property =
                    model.locateInType(model::typeIndexOf<model::ForcedConvectionPropTable>(), data.convectionPropId);
                if (!property)
                    continue;
                const model::ForcedConvectionPropCard values =
                    static_cast<const model::ForcedConvectionPropTable&>(*model.findTable(property->table))
                        .card(property->row);
                mid[index] = values.matId;
                form[index] = values.matrixForm;
                formValid[index] = 1;
            }
        }
        const auto validIfNonzero = [](const std::vector<std::int32_t>& values)
        {
            std::vector<std::uint8_t> valid(values.size());
            for (std::size_t index = 0; index < values.size(); ++index)
                valid[index] = values[index] != 0 ? 1 : 0;
            return valid;
        };
        TableBuilder builder;
        builder.int32("elementId", std::vector<std::int32_t>(list.ids));
        builder.dictionary("card", card);
        builder.int32("convectionPropId", std::move(pconid));
        builder.int32("filmNode", film, validIfNonzero(film));
        builder.int32("controlNode", control, validIfNonzero(control));
        builder.intLists("ambientNodes", ambient);
        builder.int32("matId", mid, validIfNonzero(mid));
        builder.real("h", std::move(coefficient));
        builder.int32("form", std::move(form), std::move(formValid));
        builder.real("exponentFree", std::move(exponent));
        builder.int32("massFlowNode", massFlow, validIfNonzero(massFlow));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "convectionTable";
        info.family = "T11";
        info.description =
            "The convection boundary conditions: card is CONV (free convection) or CONVM (forced convection, on a "
            "CHBDYP), "
            "elementId the surface element, pconid the PCONV or PCONVM that says how the coefficient is found, "
            "filmNode the "
            "point of the film temperature, controlNode the control point of a CONV, ambientNodes the ambient points "
            "(the "
            "eight TA of a CONV, resolved: a blank one is TA1; TA1 and TA2 of a CONVM), mid the material of the "
            "property, "
            "h the free convection coefficient (the H of the MAT4 of the property, or the first coefficient of a PCONV "
            "of "
            "the alternate layout; null for a forced convection: its coefficient comes out of the film correlation), "
            "form "
            "the FORM of the property, exponentFree the exponent EXPF of a PCONV, and massFlowNode the point that "
            "holds "
            "the mass flow rate of a CONVM (CNTMDOT). A value the card has not (or a property that does not exist) is "
            "null.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep these surface elements")};
        info.defaultSort = "elementId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("card").int32("convectionPropId").int32("filmNode").int32("controlNode");
        columns.intList("ambientNodes")
            .int32("matId")
            .real("h")
            .int32("form")
            .real("exponentFree")
            .int32("massFlowNode");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addConvectionTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConvectionTableQuery>());
}

} // namespace oor::query
