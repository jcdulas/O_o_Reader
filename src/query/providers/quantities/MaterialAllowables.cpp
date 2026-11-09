#include "oor/query/providers/quantities/MaterialAllowables.hpp"

#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"
#include "oor/query/QueryErrors.hpp"

#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// "property:12:1" -> {"property", "12", "1"}.
std::vector<std::string> split(const std::string& text)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t colon = text.find(':', start);
        const std::size_t stop = colon == std::string::npos ? text.size() : colon;
        parts.push_back(text.substr(start, stop - start));
        if (colon == std::string::npos)
            break;
        start = colon + 1;
    }
    return parts;
}

} // namespace

CallerAllowables::CallerAllowables(const std::vector<std::pair<std::string, double>>& entries)
{
    for (const auto& [key, value] : entries)
    {
        const std::vector<std::string> parts = split(key);
        std::string kind = parts[0];
        std::int64_t id = 0;
        std::size_t position = 0;
        if (kind == "all")
        {
            if (parts.size() > 1)
                position = static_cast<std::size_t>(std::stoi(parts[1]));
        }
        else
        {
            if (parts.size() < 2)
                throw QueryError(
                    "an allowable is given by (\"material\", mid), (\"property\", pid), (\"element\", eid) or for all; "
                    "'" +
                    key + "' is not");
            if (kind != "material" && kind != "property" && kind != "element")
                throw QueryError("an allowable is given for a \"material\", a \"property\" or an \"element\", not '" +
                                 kind + "'");
            id = std::stoll(parts[1]);
            if (parts.size() > 2)
                position = static_cast<std::size_t>(std::stoi(parts[2]));
        }
        if (position > 1)
            throw QueryError("a pair of allowables has two numbers (tension, shear)");
        m_values[{kind, id}][position] = value;
    }
}

std::optional<double> CallerAllowables::find(std::int32_t eid, std::int32_t pid, std::int32_t mid, int position) const
{
    const std::pair<std::string, std::int64_t> order[4] = {
        {"element", eid}, {"property", pid}, {"material", mid}, {"all", 0}};
    for (const auto& key : order)
    {
        if (key.second <= 0 && key.first != "all")
            continue;
        const auto found = m_values.find(key);
        if (found != m_values.end() && found->second[static_cast<std::size_t>(position)])
            return found->second[static_cast<std::size_t>(position)];
    }
    return std::nullopt;
}

double isoMatAllowable(const model::Model& model, std::int32_t mid, bool compression, std::string& source)
{
    source = compression ? "MAT1.SC" : "MAT1.ST";
    const auto location = model.locate(base::IdSpace::Material, mid);
    if (!location || location->table != model::typeIndexOf<model::IsoMatTable>())
    {
        source = "no MAT1 material (the allowable is not read)";
        return NOT_A_NUMBER;
    }
    const model::IsoMatCard card =
        static_cast<const model::IsoMatTable&>(*model.findTable(location->table)).card(location->row);
    const double value = compression ? card.compressionAllowable : card.tensionAllowable;
    if (!(value > 0.0))
        source = "MAT1 " + std::to_string(mid) + " has no " + (compression ? "SC" : "ST");
    return value > 0.0 ? value : NOT_A_NUMBER;
}

std::optional<std::array<double, 3>> isoMatStressLimits(const model::Model& model, std::int32_t mid)
{
    const auto location = model.locate(base::IdSpace::Material, mid);
    if (!location || location->table != model::typeIndexOf<model::IsoMatTable>())
        return std::nullopt;
    const model::IsoMatCard card =
        static_cast<const model::IsoMatTable&>(*model.findTable(location->table)).card(location->row);
    return std::array<double, 3>{card.tensionAllowable, card.compressionAllowable, card.shearAllowable};
}

std::optional<PlyMaterial> orthoMat2DAllowables(const model::Model& model, std::int32_t mid)
{
    const auto place = model.locate(base::IdSpace::Material, mid);
    if (!place || place->table != model::typeIndexOf<model::OrthoMat2DTable>())
        return std::nullopt;
    const model::OrthoMat2DCard card =
        static_cast<const model::OrthoMat2DTable&>(*model.findTable(place->table)).card(place->row);
    PlyMaterial value;
    value.xt = card.tensionStrength1;
    value.xc = card.compressionStrength1;
    value.yt = card.tensionStrength2;
    value.yc = card.compressionStrength2;
    value.s = card.shearStrength;
    value.f12 = card.interactionTerm12;
    value.e1 = card.youngsModulus1;
    value.e2 = card.youngsModulus2;
    value.nu12 = card.poissonRatio12;
    value.g12 = card.shearModulus12;
    value.strainAllowables = card.strainAllowables != 0.0;
    value.hashin = card.hashinUnidirectional;
    return value;
}

} // namespace oor::query
