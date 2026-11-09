#pragma once

#include <string_view>

namespace oor::base
{

// The ID spaces of a model. All element types share one EID space, likewise properties,
// materials, coordinate systems and methods; dynamic tables, material tables, stress tables, heat tables, damping
// tables and random tables have separate spaces. Categories
// of the long tail have no cross-type space: an ID is unique per card type there (IdSpace::PerCardType).
enum class IdSpace
{
    Node,
    Element,
    Property,
    Material,
    Coordinate,
    Method,
    DynamicTable,
    MaterialTable,
    StressTable, // TABLES1 and TABLEST: the tables of stress dependent properties (MATS1, CREEP, MATHP, ...)
    HeatTable,   // TABLEH1 and TABLEHT: the tables of convection coefficients
    HybridDampingRotorOptionsTable, // TABDMP1: the modal damping tables (Case Control SDAMPING)
    RandomTable,                    // TABRND1 and TABRNDG: the power spectral density tables (RANDPS)
    ThermalMaterial, // MAT4 and MAT5: the heat transfer materials. Apart from the structural MID space: the format
                     // reference lets a MAT4 or MAT5 have the ID of a MAT1, MAT2 or MAT3 (a coupled model has both),
                     // and they must be unique with respect to each other
    SurfaceElement, // CHBDYE, CHBDYG and CHBDYP: the heat transfer surface elements. Apart from the conduction
                    // elements: a CHEXA 1 may have the CHBDYE 1 to 6 on its sides
    AeroElement,    // CAERO1 to CAERO5: the aerodynamic macro elements. Apart from the structural elements (the format
                 // reference lets the boxes of a CAERO1 overlap the IDs of the structure) and unique with respect to
                 // each other; the boxes of a CAERO take the IDs from its EID on (the AeroBox reference target)
    AeroProperty, // PAERO1 to PAERO5: the properties a CAERO names, one number space for the five cards
    PerCardType,
    Group, // the ID is a set ID shared by several cards (FORCE, SPC1, ...): not unique, cards are grouped by it
    None   // the card has no ID
};

[[nodiscard]] std::string_view idSpaceName(IdSpace space) noexcept;

} // namespace oor::base
