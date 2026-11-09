#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/ThermalMaterials.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"

#include <cmath>
#include <map>
#include <memory>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

const std::vector<std::string> FIELDS = {"gradientX", "gradientY", "gradientZ", "fluxX", "fluxY", "fluxZ"};

// ---- the heat flux table
// ---------------------------------------------------------------------------------------------------

class StoredHeatFluxProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "heat flux table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        bool anyTable = false;
        for (const std::string& name : context.elementNames(results::ResultKind::HeatFlux))
        {
            if (!request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::HeatFlux, name, false))
            {
                anyTable = true;
                const results::ResultTableMeta& meta = ref.table->meta();
                const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
                if (!decoded)
                    continue;
                const TableValues values(*decoded);
                int component[6];
                bool complete = true;
                for (std::size_t field = 0; field < 6; ++field)
                {
                    component[field] = values.component(FIELDS[field]);
                    complete = complete && component[field] >= 0;
                }
                if (!complete || values.isComplex())
                    continue;
                ElementFlat flat;
                std::vector<std::vector<double>> real(6);
                const bool stepped =
                    meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                    meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
                for (std::size_t step = 0; step < values.steps(); ++step)
                    for (std::size_t row = 0; row < values.rows(); ++row)
                    {
                        const std::int32_t element = decoded->ids[row];
                        if (!request.wantsElement(element))
                            continue;
                        if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                            continue;
                        flat.eid.push_back(element);
                        flat.node.push_back(0);
                        flat.layer.push_back(0);
                        flat.step.push_back(stepped ? decoded->steps[step] : NOT_A_NUMBER);
                        flat.position.push_back(NOT_A_NUMBER);
                        flat.location.push_back("center");
                        for (std::size_t field = 0; field < 6; ++field)
                            real[field].push_back(values.real(step, row, static_cast<std::size_t>(component[field])));
                    }
                if (flat.eid.empty())
                    continue;
                flat.set = ref.set;
                flat.analysis = context.sets()[ref.set].analysis;
                flat.status = std::string(results::resultStatusName(meta.status));
                flat.subcase = meta.subcase;
                flat.designCycle = meta.designCycle;
                flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
                flat.elementName = meta.elementName;
                flat.card = cardOfContainer(meta.elementName);
                flat.frame = context.frameNameOf(ref);
                flat.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
                for (std::size_t field = 0; field < 6; ++field)
                    flat.values[FIELDS[field]] = std::move(real[field]);
                outcome.rows += sink.add(std::move(flat));
            }
        }
        if (outcome.rows == 0)
            outcome.reason = anyTable ? "the table has no row of the elements, subcases and steps asked for"
                                      : "the result file has no heat flux table (output of FLUX was not requested)";
        return outcome;
    }
};

// The heat flux from the temperature gradient of a table that has the gradient and no flux: q = -k grad(T), with the
// conductivity k of the MAT4 of the element (Fourier's law; the public heat flux tables are written with this sign).
// Only where the conductivity is one number: the element has a MAT4 whose K does not depend on the temperature (a MATT4
// table of K, or a MAT5, whose conductivity is a matrix in the material system, give no flux by this way). It needs the
// model the results are attached to. The way is approximate: it takes the K the card gives at the reference conditions.
class ConductivityTimesGradientProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override
    {
        return "conductivity of the material times the temperature gradient";
    }
    [[nodiscard]] bool approximate() const override { return true; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        std::string reason;
        std::map<const model::Model*, std::unique_ptr<CardFacts>> facts;
        for (const std::string& name : context.elementNames(results::ResultKind::HeatFlux))
        {
            if (!request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::HeatFlux, name, false))
            {
                const results::ResultTableMeta& meta = ref.table->meta();
                const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
                if (!decoded)
                    continue;
                const TableValues values(*decoded);
                int gradient[3];
                bool hasGradient = !values.isComplex();
                for (std::size_t field = 0; field < 3; ++field)
                {
                    gradient[field] = values.component(FIELDS[field]);
                    hasGradient = hasGradient && gradient[field] >= 0;
                }
                bool hasFlux = true;
                for (std::size_t field = 3; field < 6; ++field)
                    hasFlux = hasFlux && values.component(FIELDS[field]) >= 0;
                if (!hasGradient)
                {
                    reason = "the heat flux table has no temperature gradient";
                    continue;
                }
                if (hasFlux)
                    continue; // the stored way gave the flux
                const model::Model* model = context.modelOf(ref.set);
                if (model == nullptr)
                {
                    reason = "the results are not attached to a model: the conductivity is not available";
                    continue;
                }
                std::unique_ptr<CardFacts>& cardFacts = facts[model];
                if (!cardFacts)
                    cardFacts = std::make_unique<CardFacts>(*model);
                ElementFlat flat;
                std::vector<std::vector<double>> real(6);
                const bool stepped =
                    meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                    meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
                for (std::size_t step = 0; step < values.steps(); ++step)
                    for (std::size_t row = 0; row < values.rows(); ++row)
                    {
                        const std::int32_t element = decoded->ids[row];
                        if (!request.wantsElement(element))
                            continue;
                        const ResultQueryContext::ElementCard card = context.elementCard(ref.set, element);
                        if (!card.linked)
                            continue;
                        const auto location = model->locate(base::IdSpace::Element, card.modelId);
                        const auto material =
                            location ? thermalMaterialOfElement(*cardFacts, *model, *location) : std::nullopt;
                        if (!material)
                        {
                            reason = "element " + std::to_string(card.modelId) + " has no thermal material (MAT4)";
                            continue;
                        }
                        const std::optional<double> conductivity = isotropicConductivity(*model, *material);
                        const base::EntityId mid = model->findTable(material->table)->idAt(material->row);
                        if (!conductivity || conductivityDependsOnTemperature(*model, mid))
                        {
                            reason = "the conductivity of the material of element " + std::to_string(card.modelId) +
                                     " is not one number (a MAT5, a blank K or a MATT4 table)";
                            continue;
                        }
                        flat.eid.push_back(element);
                        flat.node.push_back(0);
                        flat.layer.push_back(0);
                        flat.step.push_back(stepped ? decoded->steps[step] : NOT_A_NUMBER);
                        flat.position.push_back(NOT_A_NUMBER);
                        flat.location.push_back("center");
                        for (std::size_t field = 0; field < 3; ++field)
                        {
                            const double slope = values.real(step, row, static_cast<std::size_t>(gradient[field]));
                            real[field].push_back(slope);
                            real[field + 3].push_back(-*conductivity * slope);
                        }
                    }
                if (flat.eid.empty())
                    continue;
                flat.set = ref.set;
                flat.analysis = context.sets()[ref.set].analysis;
                flat.status = std::string(results::resultStatusName(meta.status));
                flat.subcase = meta.subcase;
                flat.designCycle = meta.designCycle;
                flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
                flat.elementName = meta.elementName;
                flat.card = cardOfContainer(meta.elementName);
                flat.frame = context.frameNameOf(ref);
                flat.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
                for (std::size_t field = 0; field < 6; ++field)
                    flat.values[FIELDS[field]] = std::move(real[field]);
                outcome.rows += sink.add(std::move(flat));
            }
        }
        if (outcome.rows == 0)
            outcome.reason = reason.empty()
                                 ? "the result file has no heat flux table with a temperature gradient and no flux"
                                 : reason;
        return outcome;
    }
};

} // namespace

const std::vector<std::string>& heatFluxFields()
{
    return FIELDS;
}

const ElementChain& heatFluxChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("heat flux");
        chain.add(std::make_unique<StoredHeatFluxProvider>());
        chain.add(std::make_unique<ConductivityTimesGradientProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
