#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The CLASS words of a monitor group member: the kinds of monitor point (the aerodynamic and structural integrated
// loads and average displacements) and the two monitor point cards, by their neutral names (the format writes them in a
// short or a long spelling; the record writes the neutral name). A word that starts with A is an aerodynamic monitor
// point, the others are structural.
inline constexpr std::array<std::string_view, 6> MONITOR_GROUP_CLASS_WORDS{
    "AeroMonitorIntegratedLoad", "AeroMonitorDisplacement", "StructMonitorIntegratedLoad",
    "StructMonitorDisplacement", "MonitorElemLoad2",        "MonitorNodeForceSum3"};

// One monitor point of a group: its NAME and CLASS and the optional REAL, INT and STRING values the format provides for
// the display of the monitor data (labels of XY plots).
struct MonitorGroupMember
{
    std::string name;                    // NAMEi: a monitor point of the model, upper case
    std::string monitorClass;            // CLASSi, one of MONITOR_GROUP_CLASS_WORDS (as written if it is none)
    std::optional<double> real;          // REALi
    std::optional<std::int32_t> integer; // INTi
    std::string text;                    // STRINGi: up to 32 characters, may be empty

    [[nodiscard]] bool operator==(const MonitorGroupMember&) const noexcept = default;
};

// One MONGRP card: a collection of monitor points that is available to the postprocessing. It produces no output on its
// own.
//
// Card layout: MONGRP  GNAME  LABEL (fields 3 to 9: up to 56 characters)
//                     NAME1  CLASS1  REAL1  INT1  STRING1 (fields 6 to 9: up to 32 characters)
//                     NAME2  CLASS2  REAL2  INT2  STRING2
//                     etc.
// In this library's numbering (field 1 is GNAME): LABEL is fields 2 to 8 and each member is one line from field 9 on:
// NAME in its first field, CLASS in the second, REAL, INT and STRING in the third to the eighth (STRING takes the last
// four). GNAME is the name of the group and the NAMEi name monitor points (a MONPNT1, MONPNT2, MONPNT3, MONDSP1, or a
// monitor result a MONSUM adds): they are names, not IDs, kept in upper case and matched without regard to the case; a
// rule between cards can look them up (a deck may keep a monitor point in a part), the card cannot. The aerodynamic and
// the structural monitor points cannot be mixed in a group: the card refuses it. The format asks the LABEL to
// be unique across the MONGRP cards, a rule between cards.
struct MonitorGroupCard
{
    std::string groupName; // GNAME: up to 8 characters, upper case
    std::string label;     // up to 56 characters, may be empty
    std::vector<MonitorGroupMember> members;

    [[nodiscard]] bool operator==(const MonitorGroupCard&) const noexcept = default;
};

} // namespace oor::model
