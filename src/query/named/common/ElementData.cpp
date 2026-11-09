#include "oor/query/named/common/ElementData.hpp"

#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"

#include <limits>
#include <map>

namespace oor::query
{

namespace
{
constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
}

MassSetting::MassSetting(const model::Model& model, base::ThreadPool* pool)
{
    if (const auto set = analysis::NonStructuralMass::selectedByCaseControl(model))
    {
        if (analysis::NonStructuralMass::exists(model, *set))
            m_nonStructural = analysis::NonStructuralMass::resolve(model, *set, pool);
    }
}

ElementData::ElementData(std::shared_ptr<model::Model> model, const CardList& list, ExecutionContext& context)
    : m_model(std::move(model)), m_list(list), m_context(context), m_columns(m_model, list, context)
{
}

const std::vector<std::int32_t>& ElementData::properties()
{
    if (!m_properties)
        m_properties = asInt32(m_columns.get("propId"));
    return *m_properties;
}

const std::vector<double>& ElementData::lengths()
{
    if (!m_lengths)
        m_lengths = m_columns.reals("length");
    return *m_lengths;
}

const std::vector<double>& ElementData::areas()
{
    if (!m_areas)
        m_areas = m_columns.reals("area");
    return *m_areas;
}

const std::vector<double>& ElementData::volumes()
{
    if (!m_volumes)
        m_volumes = m_columns.reals("volume");
    return *m_volumes;
}

const std::vector<std::pair<std::int32_t, double>>* ElementData::laminateShares(std::int32_t pid)
{
    const auto cached = m_shares.find(pid);
    if (cached != m_shares.end())
        return cached->second ? &*cached->second : nullptr;
    std::optional<std::vector<std::pair<std::int32_t, double>>> shares;
    if (const auto location = m_model->locate(base::IdSpace::Property, pid);
        location && analysis::Laminate::supports(location->table))
    {
        std::map<std::int32_t, double> weights;
        double total = 0.0;
        for (const analysis::LaminatePly& ply : analysis::Laminate::stack(*m_model, *location))
        {
            const double density = analysis::MaterialProperties::density(*m_model, ply.materialId).value_or(0.0);
            weights[ply.materialId] += density * ply.thickness;
            total += density * ply.thickness;
        }
        if (total > 0.0)
        {
            shares.emplace();
            for (const auto& [material, weight] : weights)
                shares->emplace_back(material, weight / total);
        }
    }
    const auto inserted = m_shares.emplace(pid, std::move(shares));
    return inserted.first->second ? &*inserted.first->second : nullptr;
}

const ElementMassData& ElementData::masses()
{
    if (m_masses)
        return *m_masses;
    const std::size_t count = m_list.size();
    ElementMassData data;
    data.mass.assign(count, NOT_A_NUMBER);
    data.structural.assign(count, NOT_A_NUMBER);
    data.nonStructural.assign(count, NOT_A_NUMBER);
    data.cx.assign(count, NOT_A_NUMBER);
    data.cy.assign(count, NOT_A_NUMBER);
    data.cz.assign(count, NOT_A_NUMBER);
    data.material.assign(count, 0);
    const MassSetting setting(*m_model, m_context.pool());
    const analysis::MassOptions options = setting.options();

    // The elements by card type: the rows of the table and the position in the list.
    std::map<std::uint16_t, std::pair<std::vector<std::uint32_t>, std::vector<std::size_t>>> groups;
    for (std::size_t index = 0; index < count; ++index)
    {
        auto& group = groups[m_list.types[index]];
        group.first.push_back(m_list.rows[index]);
        group.second.push_back(index);
    }
    for (const auto& [type, group] : groups)
    {
        m_context.checkCancelled();
        const auto fill = [&](const std::vector<std::uint32_t>& rows, const std::vector<std::size_t>& positions)
        {
            const analysis::ElementMasses masses =
                analysis::MassCalculator::compute(*m_model, type, rows, options, m_context.pool());
            for (std::size_t item = 0; item < positions.size(); ++item)
            {
                const std::size_t index = positions[item];
                data.mass[index] = masses.mass[item];
                data.structural[index] = masses.structural[item];
                data.nonStructural[index] = masses.nonStructural[item];
                data.cx[index] = masses.center[item].x;
                data.cy[index] = masses.center[item].y;
                data.cz[index] = masses.center[item].z;
                data.material[index] = masses.material[item];
            }
            for (const analysis::OwnInertia& own : masses.inertia)
                data.inertia[positions[own.index]] = own.tensor;
        };
        try
        {
            fill(group.first, group.second);
        }
        catch (const base::OperationCancelled&)
        {
            throw;
        }
        catch (const base::OorError&)
        {
            // One card the mass cannot be computed for (a property that does not exist): the others still count.
            for (std::size_t item = 0; item < group.second.size(); ++item)
            {
                try
                {
                    fill({group.first[item]}, {group.second[item]});
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
    m_masses = std::move(data);
    return *m_masses;
}

std::vector<std::int32_t> mainMaterials(model::Model& model, CardFacts& facts, const CardList& list,
                                        const std::vector<std::int32_t>& pids)
{
    std::vector<std::int32_t> mids(list.size(), 0);
    std::unordered_map<std::int32_t, std::int32_t> byProperty;
    for (std::size_t index = 0; index < list.size(); ++index)
    {
        if (facts.hasField(list.types[index], "matId"))
        {
            mids[index] = facts.firstReference(list.location(index), model::ReferenceTarget::Material);
            if (mids[index] != 0)
                continue;
        }
        const std::int32_t pid = pids[index];
        if (pid == 0)
            continue;
        const auto cached = byProperty.find(pid);
        if (cached != byProperty.end())
        {
            mids[index] = cached->second;
            continue;
        }
        std::int32_t material = 0;
        if (const auto location = model.locate(base::IdSpace::Property, pid))
            material = facts.firstReference(*location, model::ReferenceTarget::Material);
        byProperty.emplace(pid, material);
        mids[index] = material;
    }
    return mids;
}

std::vector<std::uint16_t> fastenerElementTypes()
{
    using namespace oor::model;
    return {typeIndexOf<BushingTable>(),    typeIndexOf<FastenerTable>(), typeIndexOf<WeldTable>(),
            typeIndexOf<SpringDofPropertyTable>(), typeIndexOf<SpringDofInlineTable>(),  typeIndexOf<SpringScalarPropertyTable>(),
            typeIndexOf<SpringScalarInlineTable>(), typeIndexOf<Bushing1DTable>()};
}

} // namespace oor::query
