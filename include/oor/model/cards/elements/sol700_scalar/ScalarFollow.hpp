#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// The value that CELAS1D, CELAS2D, CDAMP1D and CDAMP2D (the scalar springs and dampers of SOL 700) share: the FOLLOW
// word. It moves one kind of value; every card still lays out, reads, writes and checks its own fields by hand.
namespace oor::model
{

// How the direction in which a SOL 700 spring or damper acts is updated. The only word the format reference lists is
// CORD (the direction follows the motion of the coordinate system named by the CORD field); a blank FOLLOW leaves the
// direction as it was defined.
enum class ScalarFollow : std::uint8_t
{
    Fixed, // blank
    Cord   // CoordinateFrame
};

inline constexpr std::string_view SCALAR_FOLLOW_CORD = "CoordinateFrame";

// The word of a FOLLOW value ("" for a blank field).
[[nodiscard]] constexpr std::string_view scalarFollowText(ScalarFollow follow) noexcept
{
    return follow == ScalarFollow::Cord ? SCALAR_FOLLOW_CORD : std::string_view{};
}

// The value a word names (the case does not matter), nothing for any other word. A blank word is Fixed.
[[nodiscard]] inline std::optional<ScalarFollow> parseScalarFollow(std::string_view text) noexcept
{
    if (text.empty())
        return ScalarFollow::Fixed;
    if ((text == SCALAR_FOLLOW_CORD))
        return ScalarFollow::Cord;
    return std::nullopt;
}

// Reads the FOLLOW field of a card (field 10 of the four SOL 700 scalar elements, the second field of the continuation
// line): blank or CoordinateFrame, anything else is an error.
[[nodiscard]] inline ScalarFollow readScalarFollow(FieldReader& reader, int field)
{
    const std::string_view text = reader.text(field);
    if (const std::optional<ScalarFollow> follow = parseScalarFollow(text))
        return *follow;
    reader.error("card.bad-value", "FOLLOW must be CoordinateFrame or blank, not \"" + std::string(text) + "\"",
                 "follow");
    return ScalarFollow::Fixed;
}

// A blank FOLLOW is not written, so that a card without a CORD has no continuation line.
inline void writeScalarFollow(FieldWriter& writer, int field, ScalarFollow follow)
{
    if (follow == ScalarFollow::Cord)
        writer.text(field, SCALAR_FOLLOW_CORD);
}

} // namespace oor::model
