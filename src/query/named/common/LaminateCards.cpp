#include "oor/query/named/common/LaminateCards.hpp"

#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"

namespace oor::query
{

namespace
{

std::string optionName(model::LaminateOption option)
{
    switch (option)
    {
    case model::LaminateOption::None:
        return {};
    case model::LaminateOption::Sym:
        return "Symmetry";
    case model::LaminateOption::Mem:
        return "MembraneOnly";
    case model::LaminateOption::Bend:
        return "BendingOnly";
    case model::LaminateOption::Smear:
        return "SmearedLaminate";
    case model::LaminateOption::SmCore:
        return "SandwichCore";
    }
    return {};
}

void place(std::vector<PlyInfo>& plies, double z0)
{
    double z = z0;
    for (PlyInfo& ply : plies)
    {
        ply.zBottom = z;
        z += ply.thickness;
        ply.zTop = z;
    }
}

} // namespace

std::optional<LaminateInfo> readLaminate(const model::Model& model, model::CardLocation location)
{
    const model::CardTable* table = model.findTable(location.table);
    if (table == nullptr)
        return std::nullopt;
    LaminateInfo info;
    if (location.table == model::typeIndexOf<model::CompPropTable>())
    {
        const model::CompPropCard card = static_cast<const model::CompPropTable&>(*table).card(location.row);
        info.pid = card.id;
        info.type = model::CompPropTable::NAME;
        info.z0 = card.bottomOffset;
        info.option = optionName(card.option);
        info.bondAllowable = card.bondShearAllowable;
        for (const model::PlyCard& ply : card.plies)
            info.plies.push_back(
                PlyInfo{ply.matId, ply.thickness, ply.thetaInDegrees, ply.stressOutput, 0, false, 0.0, 0.0});
        if (card.option == model::LaminateOption::Sym)
        {
            for (std::size_t index = card.plies.size(); index-- > 0;)
            {
                const model::PlyCard& ply = card.plies[index];
                info.plies.push_back(
                    PlyInfo{ply.matId, ply.thickness, ply.thetaInDegrees, ply.stressOutput, 0, true, 0.0, 0.0});
            }
        }
    }
    else if (location.table == model::typeIndexOf<model::CompPropGlobalTable>())
    {
        const model::CompPropGlobalCard card =
            static_cast<const model::CompPropGlobalTable&>(*table).card(location.row);
        info.pid = card.id;
        info.type = model::CompPropGlobalTable::NAME;
        info.z0 = card.bottomOffset;
        info.option = optionName(card.option);
        info.bondAllowable = card.bondShearAllowable;
        info.hasGlobalIds = true;
        for (const model::PlygCard& ply : card.plies)
            info.plies.push_back(
                PlyInfo{ply.matId, ply.thickness, ply.thetaInDegrees, ply.stressOutput, ply.globalId, false, 0.0, 0.0});
    }
    else
    {
        return std::nullopt;
    }
    place(info.plies, info.z0);
    return info;
}

} // namespace oor::query
