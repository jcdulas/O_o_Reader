#include "oor/query/providers/quantities/ShearPanelRods.hpp"

#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/query/results/TableValues.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr std::size_t NO_STEP = SIZE_MAX;
// up to this effectiveness the rods share the panel's area, above it each is 0.5 F
// T^2
constexpr double EFFECTIVENESS_LIMIT = 1.01;
// a panel whose diagonals are within this ratio of parallel has no plane
constexpr double DEGENERATE_RATIO = 1e-12;

double rodAreaOf(double effectiveness, double thickness, double area, double one, double other)
{
    if (!(effectiveness > 0.0))
        return 0.0;
    if (effectiveness <= EFFECTIVENESS_LIMIT)
        return effectiveness * thickness * area / (one + other);
    return 0.5 * effectiveness * thickness * thickness;
}

// The Young's modulus of a MAT1 (from G and NU when E is blank); NaN when it has none.
double youngModulus(const model::IsoMatCard& card)
{
    if (card.youngsModulus > 0.0)
        return card.youngsModulus;
    if (card.shearModulus > 0.0 && std::isfinite(card.poissonRatio))
        return 2.0 * (1.0 + card.poissonRatio) * card.shearModulus;
    return NOT_A_NUMBER;
}

bool sameStep(double one, double other)
{
    if (std::isnan(one) || std::isnan(other))
        return std::isnan(one) && std::isnan(other);
    return std::fabs(one - other) <= 1e-9 * std::max(1.0, std::fabs(one));
}

} // namespace

ShearPanelRodFacts::ShearPanelRodFacts(const ResultQueryContext& context, std::size_t set)
    : m_context(context), m_set(set), m_model(context.modelOf(set))
{
}

const PanelRods& ShearPanelRodFacts::panel(std::int32_t resultEid)
{
    if (const auto found = m_panels.find(resultEid); found != m_panels.end())
        return found->second;
    PanelRods rods;
    do
    {
        if (m_model == nullptr)
        {
            rods.why = "no model is attached to the results";
            break;
        }
        const ResultQueryContext::ElementCard card = m_context.elementCard(m_set, resultEid);
        const auto element = card.linked ? m_model->locate(base::IdSpace::Element, card.modelId) : std::nullopt;
        if (!element || element->table != model::typeIndexOf<model::ShearPanel4Table>())
        {
            rods.why = "the element is not linked to a CSHEAR of the model";
            break;
        }
        const model::ShearPanel4Card panel = m_model->findTable<model::ShearPanel4Table>()->card(element->row);
        const auto property = m_model->locate(base::IdSpace::Property, panel.propId);
        if (!property || property->table != model::typeIndexOf<model::ShearPanelPropTable>())
        {
            rods.why = "the property " + std::to_string(panel.propId) + " is not a PSHEAR";
            break;
        }
        const model::ShearPanelPropCard pshear = m_model->findTable<model::ShearPanelPropTable>()->card(property->row);
        rods.any = pshear.extensionalFactor1 > 0.0 || pshear.extensionalFactor2 > 0.0;
        if (!rods.any)
        {
            rods.ok = true;
            break;
        }
        const auto material = m_model->locate(base::IdSpace::Material, pshear.matId);
        if (!material || material->table != model::typeIndexOf<model::IsoMatTable>())
        {
            rods.why = "the material " + std::to_string(pshear.matId) + " of the PSHEAR is not a MAT1";
            break;
        }
        const double young = youngModulus(m_model->findTable<model::IsoMatTable>()->card(material->row));
        if (!(young > 0.0))
        {
            rods.why = "the MAT1 " + std::to_string(pshear.matId) + " has no Young's modulus";
            break;
        }
        if (!m_positionsTried)
        {
            m_positionsTried = true;
            try
            {
                m_positions = std::make_unique<NodePositions>(*m_model, m_context.exec().pool());
            }
            catch (const std::exception& error)
            {
                m_positionsWhy = std::string("the positions of the grid points are unknown: ") + error.what();
            }
        }
        if (!m_positions)
        {
            rods.why = m_positionsWhy;
            break;
        }
        std::array<base::Vec3, 4> corners{};
        bool placed = true;
        for (std::size_t i = 0; i < 4; ++i)
        {
            rods.nodes[i] = static_cast<std::int32_t>(panel.nodes[i]);
            placed = placed && m_positions->has(panel.nodes[i]);
            corners[i] = m_positions->position(panel.nodes[i]);
        }
        if (!placed)
        {
            rods.why = "a grid point of the panel is not a GRID of the model";
            break;
        }
        // the mean plane: normal to the cross product of the diagonals, through the centroid
        const base::Vec3 diagonals = base::cross(corners[2] - corners[0], corners[3] - corners[1]);
        const double twice = base::norm(diagonals);
        if (!(twice > DEGENERATE_RATIO * base::norm(corners[2] - corners[0]) * base::norm(corners[3] - corners[1])))
        {
            rods.why = "the panel is degenerate (its diagonals are parallel)";
            break;
        }
        const base::Vec3 normal = diagonals / twice;
        rods.area = 0.5 * twice;
        for (std::size_t e = 0; e < 4; ++e)
        {
            const base::Vec3 side = corners[(e + 1) % 4] - corners[e];
            const base::Vec3 inPlane = side - base::dot(side, normal) * normal;
            rods.length[e] = base::norm(inPlane);
            if (!(rods.length[e] > 0.0))
                break;
            rods.direction[e] = inPlane / rods.length[e];
        }
        if (!std::all_of(rods.length.begin(), rods.length.end(), [](double length) { return length > 0.0; }))
        {
            rods.why = "an edge of the panel has no length";
            break;
        }
        const base::Vec3 x = rods.direction[0];
        const base::Vec3 y = base::cross(normal, x);
        for (std::size_t e = 0; e < 4; ++e)
        {
            const double factor = e % 2 == 0 ? pshear.extensionalFactor1 : pshear.extensionalFactor2;
            rods.rodArea[e] = e % 2 == 0
                                  ? rodAreaOf(factor, pshear.thickness, rods.area, rods.length[0], rods.length[2])
                                  : rodAreaOf(factor, pshear.thickness, rods.area, rods.length[1], rods.length[3]);
            rods.stiffness[e] = young * rods.rodArea[e] / rods.length[e];
            const double dx = base::dot(rods.direction[e], x);
            const double dy = base::dot(rods.direction[e], y);
            const double scale = rods.rodArea[e] > 0.0 ? rods.length[e] / rods.area : 0.0;
            rods.weights[e] = {scale * dx * dx, scale * dy * dy, scale * dx * dy};
        }
        rods.ok = true;
    } while (false);
    return m_panels.emplace(resultEid, std::move(rods)).first->second;
}

PanelTranslations::PanelTranslations(const ResultQueryContext& context, const std::vector<TableRef>& tables,
                                     const TableRef& forces, const results::ResultData& forceRows,
                                     const std::vector<std::int32_t>& nodes)
{
    const results::ResultTableMeta& wanted = forces.table->meta();
    const bool modes = wanted.stepKind == results::StepKind::Mode;
    for (const TableRef& ref : tables)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        if (ref.set != forces.set || meta.subcase != wanted.subcase || meta.designCycle != wanted.designCycle ||
            (meta.kind == results::ResultKind::Eigenvectors) != modes)
            continue;
        if (meta.kind != results::ResultKind::Displacements && meta.kind != results::ResultKind::Eigenvectors)
            continue;
        // the rows of the grid points wanted, found by the ID of the file (the model's unless renumbered), else the
        // whole table
        std::vector<std::int64_t> ids(nodes.begin(), nodes.end());
        for (int attempt = 0; attempt < 2; ++attempt)
        {
            std::shared_ptr<const results::ResultData> data = context.decodeRows(
                ref, std::nullopt, attempt == 0 ? std::optional<std::vector<std::int64_t>>(ids) : std::nullopt);
            if (!data)
                continue;
            const TableValues values(*data);
            m_t1 = values.component("t1");
            if (m_t1 < 0)
                break;
            m_rowOf.clear();
            for (std::size_t row = 0; row < data->rowCount; ++row)
            {
                const std::int32_t node = context.modelNodeId(ref.set, data->ids[row]);
                if (node > 0)
                    m_rowOf.emplace(node, row);
            }
            const bool every =
                std::all_of(nodes.begin(), nodes.end(), [&](std::int32_t node) { return m_rowOf.contains(node); });
            if (!every && attempt == 0)
                continue;
            m_data = std::move(data);
            m_complex = values.isComplex();
            break;
        }
        if (!m_data)
            continue;
        m_stepOf.assign(forceRows.stepCount, NO_STEP);
        for (std::size_t step = 0; step < forceRows.stepCount; ++step)
        {
            const double value = step < forceRows.steps.size() ? forceRows.steps[step] : NOT_A_NUMBER;
            for (std::size_t other = 0; other < m_data->stepCount; ++other)
            {
                const double candidate = other < m_data->steps.size() ? m_data->steps[other] : NOT_A_NUMBER;
                if (sameStep(value, candidate) || (forceRows.stepCount == 1 && m_data->stepCount == 1))
                {
                    m_stepOf[step] = other;
                    break;
                }
            }
        }
        return;
    }
}

std::optional<base::Vec3> PanelTranslations::at(std::size_t forceStep, std::int32_t node, bool imaginary) const
{
    if (!m_data || forceStep >= m_stepOf.size() || m_stepOf[forceStep] == NO_STEP)
        return std::nullopt;
    const auto found = m_rowOf.find(node);
    if (found == m_rowOf.end())
        return std::nullopt;
    const TableValues values(*m_data);
    const std::size_t step = m_stepOf[forceStep];
    const auto component = [&](std::size_t offset)
    {
        const auto index = static_cast<std::size_t>(m_t1) + offset;
        return imaginary ? values.imag(step, found->second, index) : values.real(step, found->second, index);
    };
    return base::Vec3{component(0), component(1), component(2)};
}

std::optional<base::Vec3> PanelTranslations::real(std::size_t forceStep, std::int32_t node) const
{
    return at(forceStep, node, false);
}

std::optional<base::Vec3> PanelTranslations::imag(std::size_t forceStep, std::int32_t node) const
{
    return at(forceStep, node, true);
}

std::array<double, 4> rodForces(const PanelRods& rods, const PanelTranslations& translations, std::size_t forceStep,
                                bool imaginary)
{
    std::array<double, 4> forces{};
    for (std::size_t e = 0; e < 4; ++e)
    {
        if (!(rods.stiffness[e] > 0.0))
            continue;
        const auto first =
            imaginary ? translations.imag(forceStep, rods.nodes[e]) : translations.real(forceStep, rods.nodes[e]);
        const auto second = imaginary ? translations.imag(forceStep, rods.nodes[(e + 1) % 4])
                                      : translations.real(forceStep, rods.nodes[(e + 1) % 4]);
        forces[e] = first && second ? rods.stiffness[e] * base::dot(*second - *first, rods.direction[e]) : NOT_A_NUMBER;
    }
    return forces;
}

} // namespace oor::query
