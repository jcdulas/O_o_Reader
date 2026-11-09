#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One GENUDS card (SOL 700): integer, real and character data that is handed to the notify method of a user service at
// the start and the end of a load case and of an increment. No other entry of the format reference has this name.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   GENUDS  SRV_ID                                                        field 1
// then up to three blocks of data, each starting on a line of its own with its keyword in the first field of the line
// and seven values to a line (the next lines of a block leave that first field blank):
//   "INT"   IDATA1 IDATA2 ... IDATAn                                      integers
//   "REAL"  RDATA1 RDATA2 ... RDATAn                                      reals
//   "CHAR"  CDATA1 CDATA2 ... CDATAn                                      words
// SRV_ID is the identifier that the CONNECT SERVICE statement of the FMS section gives the service (the service must
// implement the RuntimeInfo interface); it is a name of at most eight characters and the card has no numeric ID, so its
// table has IdSpace::None and Python finds a card by its SRV_ID. The blocks may come in any order and each at most
// once, with at least one value; the card is written with INT, REAL and CHAR in that order. A CDATAi is a word of at
// most eight characters and is never one of the three keywords. The words are kept in capitals. The layout is the one
// of MATUDS (SOL 400 material services) with the service name alone in front of the blocks, and is written here on its
// own.
struct UserServiceDataCard
{
    std::string serviceId;
    std::vector<std::int32_t> integers; // the INT block
    std::vector<double> reals;          // the REAL block
    std::vector<std::string> words;     // the CHAR block
};

} // namespace oor::model
