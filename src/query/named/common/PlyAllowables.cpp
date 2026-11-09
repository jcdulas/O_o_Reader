#include "oor/query/named/common/PlyAllowables.hpp"

#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

std::int8_t codeOf(const std::string& theory)
{
    const std::vector<std::string>& names = plyTheoryNames();
    const auto found = std::find(names.begin(), names.end(), theory);
    return found == names.end() ? std::int8_t{-1} : static_cast<std::int8_t>(found - names.begin());
}

// The FT of a PCOMP or PCOMPG as the margins name it ("" for none).
std::string propertyTheory(const model::Model& model, const model::CardLocation& location)
{
    const model::CardTable* table = model.findTable(location.table);
    if (location.table == model::typeIndexOf<model::CompPropTable>())
        return theoryName(
            static_cast<int>(static_cast<const model::CompPropTable&>(*table).card(location.row).failureTheory));
    if (location.table == model::typeIndexOf<model::CompPropGlobalTable>())
        return theoryName(
            static_cast<int>(static_cast<const model::CompPropGlobalTable&>(*table).card(location.row).failureTheory));
    return {};
}

// Adds the plies of the laminate at `location` (property `propId`) as rows, and returns their rows.
std::vector<std::int32_t> addPlies(const model::Model& model, std::int32_t pid, const model::CardLocation& location,
                                   const std::string& asked, PlyAllowables& result)
{
    std::vector<std::int32_t> rows;
    const std::optional<LaminateInfo> laminate = readLaminate(model, location);
    if (!laminate)
        return rows;
    const std::int8_t theory = codeOf(asked.empty() ? propertyTheory(model, location) : asked);
    const double sb = laminate->bondAllowable > 0.0 ? laminate->bondAllowable : NOT_A_NUMBER;
    std::int32_t number = 0;
    for (const PlyInfo& ply : laminate->plies)
    {
        rows.push_back(static_cast<std::int32_t>(result.rows()));
        result.pid.push_back(pid);
        result.ply.push_back(++number);
        result.plyId.push_back(ply.globalId);
        result.mid.push_back(ply.material);
        result.theta.push_back(ply.angle);
        result.thickness.push_back(ply.thickness);
        result.theory.push_back(theory);
        result.sb.push_back(sb);
        const std::optional<PlyMaterial> material = orthoMat2DAllowables(model, ply.material);
        if (!material)
        {
            for (std::vector<double>* column : {&result.xt, &result.xc, &result.yt, &result.yc, &result.s, &result.f12,
                                                &result.st, &result.e1, &result.e2, &result.nu12, &result.g12})
                column->push_back(NOT_A_NUMBER);
            result.strainAllowables.push_back(0);
            continue;
        }
        // Hashin reads the strengths of the HFAIL group where the MAT8 has one (as PlyFailureModes.cpp does)
        const bool hfail = theory == codeOf("hashin") && material->hashin.has_value();
        const std::array<double, 6> hashin = hfail ? *material->hashin : std::array<double, 6>{};
        result.xt.push_back(hfail ? hashin[0] : material->xt);
        result.xc.push_back(hfail ? hashin[1] : material->xc);
        result.yt.push_back(hfail ? hashin[2] : material->yt);
        result.yc.push_back(hfail ? hashin[3] : material->yc);
        result.s.push_back(hfail ? hashin[4] : material->s);
        result.st.push_back(hfail ? hashin[5] : NOT_A_NUMBER);
        result.f12.push_back(material->f12);
        result.e1.push_back(material->e1);
        result.e2.push_back(material->e2);
        result.nu12.push_back(material->nu12);
        result.g12.push_back(material->g12);
        result.strainAllowables.push_back(material->strainAllowables ? 1 : 0);
    }
    return rows;
}

} // namespace

const std::vector<std::string>& plyTheoryNames()
{
    static const std::vector<std::string> NAMES = {"maxStress", "maxStrain", "hill", "hoffman",
                                                   "tsaiWu",    "hashin",    "puck", "larc03"};
    return NAMES;
}

PlyAllowables plyAllowables(const model::Model& model, const std::vector<std::int32_t>& elementIds,
                            const std::vector<std::int32_t>& layers, std::size_t locations, const std::string& theory)
{
    if (!theory.empty() && codeOf(theory) < 0)
        throw QueryError(
            "plyAllowables: theory is None, \"maxStress\", \"maxStrain\", \"hill\", \"hoffman\", \"tsaiWu\", "
            "\"hashin\", \"puck\" or \"larc03\", not '" +
            theory + "'");
    if (layers.size() != elementIds.size() * locations)
        throw QueryError("plyAllowables: the ply numbers are one per slot of each element (elements x slots)");
    PlyAllowables result;
    result.slots.assign(layers.size(), -1);
    // the plies of each property, read once for its elements
    std::unordered_map<std::int32_t, std::vector<std::int32_t>> plies;
    for (std::size_t element = 0; element < elementIds.size(); ++element)
    {
        const auto location = model.locate(base::IdSpace::Element, elementIds[element]);
        if (!location)
            continue;
        const std::optional<base::EntityId> property = analysis::StoredFrames::propertyOf(model, *location);
        if (!property)
            continue;
        const auto pid = static_cast<std::int32_t>(*property);
        auto found = plies.find(pid);
        if (found == plies.end())
        {
            const auto propertyLocation = model.locate(base::IdSpace::Property, pid);
            found = plies
                        .emplace(pid, propertyLocation ? addPlies(model, pid, *propertyLocation, theory, result)
                                                       : std::vector<std::int32_t>{})
                        .first;
        }
        const std::vector<std::int32_t>& rows = found->second;
        for (std::size_t slot = 0; slot < locations; ++slot)
        {
            const std::int32_t layer = layers[element * locations + slot];
            if (layer >= 1 && static_cast<std::size_t>(layer) <= rows.size())
                result.slots[element * locations + slot] = rows[static_cast<std::size_t>(layer - 1)];
        }
    }
    return result;
}

} // namespace oor::query
