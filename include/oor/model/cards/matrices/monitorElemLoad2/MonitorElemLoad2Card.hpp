#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One MONPNT2 card: an internal load monitor point. One output item of one element (a stress, force or strain
// component, named by its NDDL label) is monitored, in the solutions the format reference lists (101, 103, 108 to 112,
// 144, 146, 200). The component is assumed linear in the displacement vector.
//
// Card layout: MONPNT2  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                      .     TABLE  TYPE  NDDLITEM  EID
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, TABLE field 9, TYPE 10, NDDLITEM 11, EID 12.
// NAME and NDDLITEM are kept in upper case, TABLE and TYPE (neutral names) and the LABEL as written. NDDLITEM is a
// word, or an integer (kept as the text of the field).
struct MonitorElemLoad2Card
{
    std::string name;        // up to 8 characters
    std::string label;       // up to 56 characters, may be empty
    std::string resultTable; // Stress, Force or Strain
    std::string kind;        // the element type, a card name: Bar2, ShellQuad4, ...
    std::string resultItem;  // the NDDL label of the component for that table and element type
    base::EntityId elemId = 0;

    [[nodiscard]] bool operator==(const MonitorElemLoad2Card&) const noexcept = default;
};

} // namespace oor::model
