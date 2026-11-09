#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <string>
#include <string_view>

// The names the cards that couple SOL 700 to an external program give to the things of that program: a segment of the
// external occupant model (ATBSEG, ATBACC), a joint of it (ATBJNT) and the external ellipsoid of RELEX. One kind of
// value: the name is a word of the Bulk Data that the other program knows, so it is not a reference to any card of the
// model.
namespace oor::model
{

// A name that the card requires, in upper case (the solver reads its input in upper case). A blank field is an error.
[[nodiscard]] inline std::string readExternalName(FieldReader& reader, int field, const char* name)
{
    if (reader.isBlank(field))
    {
        reader.error("card.missing-field", std::string(name) + " is required", name);
        return {};
    }
    return std::string(reader.text(field));
}

} // namespace oor::model
