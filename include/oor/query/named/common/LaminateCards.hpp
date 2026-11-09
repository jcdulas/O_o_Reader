#pragma once

#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// One ply of a laminate as it is stacked (the plies a symmetry option adds included).
struct PlyInfo
{
    std::int32_t material = 0;
    double thickness = 0.0;
    double angle = 0.0; // degrees
    bool stressOutput = false;
    std::int32_t globalId = 0; // the global ply ID of a PCOMPG, 0 for a PCOMP
    bool mirrored = false;     // made by the SYM option
    double zBottom = 0.0;
    double zTop = 0.0;
};

// What a laminate property says: the stack of the plies from the bottom, bottom to top, with the z of each face.
struct LaminateInfo
{
    std::int32_t pid = 0;
    std::string type; // the card type: "CompProp" or "CompPropGlobal"
    double z0 = 0.0;
    std::string option;         // the LAM option as written: "", "SYM", "MEM", "BEND", "SMEAR", "SMCORE"
    double bondAllowable = 0.0; // SB, the allowable interlaminar shear stress (0: blank)
    std::vector<PlyInfo> plies;
    bool hasGlobalIds = false;

    [[nodiscard]] double thickness() const noexcept
    {
        double total = 0.0;
        for (const PlyInfo& ply : plies)
            total += ply.thickness;
        return total;
    }
};

// The laminate in `location`, nothing when the card is not a PCOMP or PCOMPG. A SYM laminate gets the plies of the
// other half after the ones given, in the opposite order (the same rule as the mass and stiffness code).
[[nodiscard]] std::optional<LaminateInfo> readLaminate(const model::Model& model, model::CardLocation location);

} // namespace oor::query
