#include "oor/base/ids/IdSpace.hpp"

namespace oor::base
{

std::string_view idSpaceName(IdSpace space) noexcept
{
    switch (space)
    {
    case IdSpace::Node:
        return "node";
    case IdSpace::Element:
        return "element";
    case IdSpace::Property:
        return "property";
    case IdSpace::Material:
        return "material";
    case IdSpace::Coordinate:
        return "coordinate";
    case IdSpace::Method:
        return "method";
    case IdSpace::DynamicTable:
        return "dynamic table";
    case IdSpace::MaterialTable:
        return "material table";
    case IdSpace::StressTable:
        return "stress table";
    case IdSpace::HeatTable:
        return "heat table";
    case IdSpace::HybridDampingRotorOptionsTable:
        return "damping table";
    case IdSpace::RandomTable:
        return "random table";
    case IdSpace::ThermalMaterial:
        return "thermal material";
    case IdSpace::SurfaceElement:
        return "surface element";
    case IdSpace::AeroElement:
        return "aerodynamic element";
    case IdSpace::AeroProperty:
        return "aerodynamic property";
    case IdSpace::PerCardType:
        return "per card type";
    case IdSpace::Group:
        return "group";
    case IdSpace::None:
        return "none";
    }
    return "unknown";
}

} // namespace oor::base
