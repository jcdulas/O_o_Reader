#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// One interval of a BIAS card: the planes of the Euler mesh between its begin coordinate and the begin coordinate of
// the next interval (or the end of the mesh box, for the last one) get a graded spacing. The spacing is described by
// two of the four quantities GROWTH, N, DXS and DXE, or by N alone (equal spacing); a blank stays blank (None in
// Python).
struct EulerMeshBiasInterval
{
    std::optional<double> begin;       // Xi: the begin coordinate
    std::optional<double> growth;      // GROWTHi: the ratio of the first element size to the last, greater than zero
    std::optional<std::int32_t> count; // Ni: the number of elements in the interval, greater than zero
    std::optional<double> startSize;   // DXSi: the element size at the begin, greater than zero
    std::optional<double> endSize;     // DXEi: the element size at the end, greater than zero

    [[nodiscard]] bool operator==(const EulerMeshBiasInterval&) const noexcept = default;
};

// One BIAS card (SOL 700): the variation of the element size of an Euler mesh along one direction, which the IBIDX,
// IBIDY or IBIDZ of a MESH names. The mesh is a set of planes; the card says where they are, interval by interval.
// BIAS is also the name of a parameter of the contact entries (BCTABLE, BCONPRG, BCPARA), which is another thing: this
// is the Bulk Data entry of the MESH.
//
// Card layout: BIAS  BID                                   fields 1 (the others of the line are blank)
//             X0  GROWTH0  N0  DXS0  DXE0                 fields 9 to 13, one line per interval
//             X1  GROWTH1  N1  DXS1  DXE1                 fields 17 to 21
//             Xi  GROWTHi  Ni  DXSi  DXEi                 and so on, every 8 fields
// The intervals follow the BID on continuation lines and there is no limit on their number: the card keeps them in
// order and writes them back one per line. The format gives seven ways to describe the spacing of an interval: GROWTH
// and N; DXS and DXE; DXS and N; DXE and N; GROWTH and DXS; GROWTH and DXE; N alone. The check accepts exactly those,
// so the other two quantities of the interval are blank. The begin coordinate of the first interval is the X0 of the
// MESH and may be left blank, which is why Xi is optional: the format reference field list also gives 0.0 as a default,
// but that value would not be the mesh origin, so a blank is kept blank. The end of an interval is the begin of the
// next one, so given begin coordinates must increase.
struct EulerMeshBiasCard
{
    base::EntityId id = 0; // unique among the BIAS cards
    std::vector<EulerMeshBiasInterval> intervals;
};

} // namespace oor::model
