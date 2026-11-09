#pragma once

namespace oor::model
{

// One ECHOON card: marks the point of the Bulk Data from which the printed echo of the input file is activated. It has
// no field.
//
// Card layout: ECHOON   (nothing else on the line)
// The entry may also be used in the Executive Control and the Case Control sections, which keep their own handling of
// it (the Case Control command ECHO); this card is the Bulk Data entry. The companion entry is ECHOOFF. It is BEGIN
// BULK or BEGIN SUPER = n dependent: its last use carries over between the BEGIN BULK and BEGIN SUPER lines,
// so a ECHOON is meaningful inside a part, where it is kept in its place like any other card. The card is typed so that
// it round trips in place and is not an unknown card in strict mode; a field after the name is refused.
struct EchoOnMarkerCard
{
    [[nodiscard]] bool operator==(const EchoOnMarkerCard&) const noexcept = default;
};

} // namespace oor::model
