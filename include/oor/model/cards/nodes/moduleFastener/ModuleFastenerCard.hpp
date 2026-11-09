#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// What the patches IDA and IDB of an MDFAST are (the TYPE field). The enumerators are numbered in the order of
// MODULE_FASTENER_TYPE_WORDS.
enum class ModuleFastenerType : std::uint8_t
{
    Property, // PROP: IDA and IDB are PSHELL (or PCOMP) properties that define the patches
    Element   // ELEM: IDA and IDB are shell elements
};

inline constexpr std::array<std::string_view, 2> MODULE_FASTENER_TYPE_WORDS{"Property", "Elements"};

// One MDFAST card: a fastener with a material orientation that connects two surface patches that lie in two modules
// (the multi-module counterpart of the CFAST). The patch A is in the module MODA and the patch B in MODB; a module is a
// BEGIN MODULE section of the deck, a part of the deck, and module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDFAST  EID  PID  TYPE  IDA  IDB  GS  GA  GB      fields 1 to 8
//                     XS   YS   ZS                  MODA MODB   fields 9 to 11, and 15 and 16
// Fields 12 to 14 are blank. The card belongs in the main Bulk Data (a rule of the deck, not of the card);
// the rules of the CFAST apply.
//
// PID names a PFAST of the main Bulk Data and is EID when blank, so it is stored resolved. IDA and
// IDB are IDs of the modules MODA and MODB: properties or elements as TYPE says; they differ, which
// can only be asked of two patches of the same module, so the card refuses them equal when MODA and MODB are equal. GS,
// GA and GB are grid points that the format reference puts in the main Bulk Data (module 0), blank when not used; XS,
// YS and ZS locate the fastener in the basic system, are all given or all blank and are required when neither GS nor GA
// is given (as for the CFAST: `location` is empty when blank).
struct ModuleFastenerCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    ModuleFastenerType type = ModuleFastenerType::Property;
    base::EntityId patchIdA = 0;
    base::EntityId patchIdB = 0;
    base::EntityId spotNode = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::optional<std::array<double, 3>> location; // XS, YS, ZS
    base::EntityId moduleIdA = 0;                  // the module of IDA, 0 for the main Bulk Data
    base::EntityId moduleIdB = 0;                  // the module of IDB
};

} // namespace oor::model
