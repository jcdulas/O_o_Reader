#pragma once

#include <string>
#include <vector>

namespace oor::model
{

// One METADATA card: user information that is written with the results (the binary result file when PARAM,POST is +1,
// -1 or -7, and the exported .pch file of an EXTSEOUT). It has no effect on the solution.
//
// Card layout: METADATA  META (fields 2 to 4: up to 24 characters)
//                       METAINFO (fields 2 to 9: up to 64 characters)
//                       METAINFO
//                       etc.
// In this library's numbering META is fields 1 to 3 of the first line and each METAINFO is one continuation line,
// fields 1 to 8 of the line (field 9 on from the first continuation line). META is the handle that identifies the
// metadata and METAINFO is an open-ended list of lines of text, with blanks inside; both are kept as written. A text is
// read from the first field it uses to the last as the line has it (`FieldReader::spanText`) and written in pieces of
// eight characters in the small format and whole in the others (`FieldWriter::spanText`). It cannot hold a '$' (a
// comment), a comma (the lexer takes a line with a comma for a free format line and cuts the text there, whatever the
// card), a tab or any other control character, and it begins and ends with a character that is not a blank. The format
// says META and METAINFO may use any character of the bulk character set: the characters that no line
// can carry are refused. A line that is blank in the middle of the list is kept; one at its end cannot be (a blank
// continuation line is not a line of the card), so the card refuses it. A deck may keep one METADATA per superelement
// (it is read wherever it is).
struct ResultMetadataCard
{
    std::string handle;             // META: up to 24 characters
    std::vector<std::string> lines; // METAINFO: up to 64 characters each

    [[nodiscard]] bool operator==(const ResultMetadataCard&) const noexcept = default;
};

} // namespace oor::model
