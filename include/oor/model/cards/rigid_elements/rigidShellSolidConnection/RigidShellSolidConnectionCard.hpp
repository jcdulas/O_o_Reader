#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// TYPE: what the IDs of an RSSCON are, elements or grid points.
enum class RigidShellSolidConnectionType : std::uint8_t
{
    Element, // ELEM (the default)
    Grid     // GRID
};

inline constexpr std::array<std::string_view, 2> RIGID_SHELL_SOLID_CONNECTION_TYPE_WORDS{"Elements", "Node"};

// One RSSCON card: a clamped connection between a shell and a solid element (multipoint constraints).
//
// Card layout: RSSCON  RBID  TYPE  ES1  EA1  EB1  ES2  EA2  EB2
// With TYPE = ELEM (the default when blank) the connection is described by two elements: ES1 is the shell element and
// EA1 the solid element, the other fields are blank. With TYPE = GRID it is described by grid points: ES1 is a shell
// grid point, EA1 and EB1 solid grid points (EB1 may be blank), and the second group ES2, EA2, EB2 is optional (blank
// or positive). The ids are kept in the order of the card: ES1, EA1, EB1, ES2, EA2, EB2 (0 for a blank field). The
// form TYPE = INTC of another input dialect is refused. The rigid elements share the ID space of the elements.
struct RigidShellSolidConnectionCard
{
    static constexpr std::size_t ID_COUNT = 6;

    base::EntityId id = 0;
    RigidShellSolidConnectionType kind = RigidShellSolidConnectionType::Element;
    std::array<base::EntityId, ID_COUNT> ids{0, 0, 0, 0, 0, 0};
};

} // namespace oor::model
