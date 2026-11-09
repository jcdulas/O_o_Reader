#include "ForceBlocks.hpp"

#include "oor/query/named/TableBuilder.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

RowBlock forceBlock(const ResultQueryContext& context, const ElementFlat& flat, const std::vector<std::string>& values,
                    const ForceColumns& options, std::int16_t provider)
{
    const std::size_t rows = flat.rows();
    RowBlock block(rows);
    block.ints("subcase", std::vector<std::int32_t>(rows, flat.subcase));
    block.ints("elementId", flat.eid);
    block.texts("type", std::vector<std::string>(rows, flat.card),
                std::vector<std::uint8_t>(rows, flat.card.empty() ? 0 : 1));
    std::vector<std::int32_t> pid(rows, 0);
    std::vector<std::uint8_t> pidValid(rows, 0);
    for (std::size_t row = 0; row < rows; ++row)
    {
        const std::int32_t found = context.elementCard(flat.set, flat.eid[row]).pid;
        if (found > 0)
        {
            pid[row] = found;
            pidValid[row] = 1;
        }
    }
    block.ints("propId", std::move(pid), std::move(pidValid));
    if (options.location)
    {
        block.texts("location", flat.location);
    }
    if (options.station)
        block.reals("station", flat.position);
    if (options.node)
    {
        std::vector<std::uint8_t> valid(rows);
        for (std::size_t row = 0; row < rows; ++row)
            valid[row] = flat.node[row] > 0 ? 1 : 0;
        block.ints("nodeId", flat.node, std::move(valid));
    }
    for (const std::string& field : values)
    {
        const auto real = flat.values.find(field);
        if (real == flat.values.end())
            continue;
        const auto imaginary = flat.imaginary.find(field);
        if (imaginary != flat.imaginary.end())
            block.complex(field, real->second, imaginary->second);
        else
            block.reals(field, real->second);
    }
    if (options.tension)
    {
        const std::vector<double>& axial = flat.values.at("axial");
        std::vector<double> tension(rows);
        for (std::size_t row = 0; row < rows; ++row)
            tension[row] = std::isnan(axial[row]) ? axial[row] : std::max(axial[row], 0.0);
        block.reals("tension", std::move(tension));
    }
    if (options.frame)
        block.texts("frame", std::vector<std::string>(rows, flat.frame));
    if (flat.stepKind != results::StepKind::None)
        block.steps(flat.stepKind, flat.step, flat.eigenvalue);
    block.origin(flat.set, flat.analysis, flat.designCycle);
    block.precision(flat.precision);
    block.storedAsMagnitudePhase(flat.magnitudePhase);
    block.validation(flat.status);
    if (provider >= 0)
        block.provider(provider);
    return block;
}

std::shared_ptr<const QueryTable> forceTable(const ResultQueryContext& context, const QueryInfo& info,
                                             const ElementResolution& resolution,
                                             const std::vector<std::string>& values, const ForceColumns& options,
                                             const std::vector<std::string>& sortKeys)
{
    if (resolution.set.flats().empty())
        return emptyTable(info.defaultColumns());
    RowAccumulator accumulator;
    std::vector<std::int16_t> providers;
    for (const ProviderInfo& provider : resolution.set.providers())
        providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
    for (const ElementFlat& flat : resolution.set.flats())
        accumulator.add(
            forceBlock(context, flat, values, options,
                       flat.provider >= 0 ? providers[static_cast<std::size_t>(flat.provider)] : std::int16_t(-1)));
    std::vector<std::string> keys = leadingKeys();
    keys.insert(keys.end(), sortKeys.begin(), sortKeys.end());
    return sortedResult(accumulator.finish(context, info), context.exec(), keys);
}

} // namespace oor::query
