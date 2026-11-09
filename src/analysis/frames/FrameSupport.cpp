#include "FrameSupport.hpp"

namespace oor::analysis::detail
{

namespace
{

template <typename TableT>
std::optional<base::EntityId> pidIf(const model::Model& model, model::CardLocation location)
{
    if (location.table != model::typeIndexOf<TableT>())
        return std::nullopt;
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return std::nullopt;
    return table->card(location.row).propId;
}

} // namespace

std::optional<base::EntityId> elementPropertyId(const model::Model& model, model::CardLocation location)
{
    if (const auto pid = pidIf<model::ShellQuad4Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShellTria3Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShellQuad8Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShellTria6Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShellQuad4DrillingTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShellTria3DrillingTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::SolidHexaTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::SolidPentaTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::SolidTetraTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::SolidPyramidTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::Bar2Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::Beam2Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::Rod2Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::Tube2Table>(model, location))
        return pid;
    if (const auto pid = pidIf<model::BushingTable>(model, location))
        return pid;
    if (const auto pid = pidIf<model::ShearPanel4Table>(model, location))
        return pid;
    return std::nullopt;
}

std::optional<base::EntityId> solidCordm(const model::Model& model, base::EntityId pid)
{
    const std::optional<model::CardLocation> location = model.locate(base::IdSpace::Property, pid);
    if (!location || location->table != model::typeIndexOf<model::SolidPropTable>())
        return std::nullopt;
    const model::SolidPropTable* table = model.findTable<model::SolidPropTable>();
    if (table == nullptr)
        return std::nullopt;
    return table->card(location->row).matFrameId;
}

std::vector<std::int32_t> elementIdsOf(const results::ResultTable& table, const results::DecodeContext& context)
{
    std::shared_ptr<const results::ResultData> data;
    if (table.isDecoded())
    {
        data = table.data(context);
    }
    else
    {
        results::ResultFilter filter;
        filter.stepIndices = std::vector<std::int64_t>{0};
        if (!table.meta().components.empty())
            filter.components = std::vector<std::string>{table.meta().components.front()};
        data = table.select(filter)->data(context);
    }
    return data->ids.toVector();
}

} // namespace oor::analysis::detail
