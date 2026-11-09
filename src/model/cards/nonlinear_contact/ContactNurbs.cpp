#include "oor/model/cards/nonlinear_contact/ContactNurbs.hpp"

#include <cmath>

namespace oor::model
{

ContactNurbsData readContactNurbsData(FieldReader& reader, const std::vector<ContactSection>& sections)
{
    ContactNurbsData data;
    bool hasGrid = false;
    bool hasCoord = false;
    bool hasHomo = false;
    bool hasKnot = false;
    for (const ContactSection& section : sections)
    {
        if (section.keyword == NURBS_GRID)
        {
            if (hasGrid)
                reader.error("card.bad-value", "GRID is given twice", "nodeId");
            hasGrid = true;
            data.grids = readContactIds(reader, section.first + 1, section.last, "nodeId");
        }
        else if (section.keyword == NURBS_COORD)
        {
            if (hasCoord)
                reader.error("card.bad-value", "COORD is given twice", "coordinates");
            hasCoord = true;
            data.coordinates = readContactReals(reader, section.first + 1, section.last, "coordinates");
        }
        else if (section.keyword == NURBS_HOMO)
        {
            if (hasHomo)
                reader.error("card.bad-value", "HOMO is given twice", "homogeneous");
            hasHomo = true;
            data.homogeneous = readContactReals(reader, section.first + 1, section.last, "homogeneous");
        }
        else if (section.keyword == NURBS_KNOT)
        {
            if (hasKnot)
                reader.error("card.bad-value", "KNOT is given twice", "knots");
            hasKnot = true;
            data.knots = readContactReals(reader, section.first + 1, section.last, "knots");
        }
    }
    if (hasGrid && hasCoord)
        reader.error("card.bad-value",
                     "the list of grid points (GRID) and the list of coordinates (COORD) cannot coexist",
                     "coordinates");
    data.byCoordinates = hasCoord;
    return data;
}

int writeContactNurbsData(FieldWriter& writer, int lineStart, const ContactNurbsData& data)
{
    int line = lineStart;
    if (data.byCoordinates)
        line = writeContactRealList(writer, line, NURBS_COORD, data.coordinates);
    else if (!data.grids.empty())
        line = writeContactIdList(writer, line, NURBS_GRID, data.grids);
    if (!data.homogeneous.empty())
        line = writeContactRealList(writer, line, NURBS_HOMO, data.homogeneous);
    if (!data.knots.empty())
        line = writeContactRealList(writer, line, NURBS_KNOT, data.knots);
    return line;
}

std::optional<CardProblem> checkContactNurbsData(const ContactNurbsData& data, std::int64_t points,
                                                 int coordinatesPerPoint, std::int64_t knots)
{
    if (data.byCoordinates)
    {
        if (!data.grids.empty())
            return CardProblem{"card.bad-value", "the grid points and the coordinates cannot coexist", "nodeId"};
        if (static_cast<std::int64_t>(data.coordinates.size()) != points * coordinatesPerPoint)
            return CardProblem{"card.bad-value",
                               "COORD needs " + std::to_string(coordinatesPerPoint) + " values for each of the " +
                                   std::to_string(points) + " control points",
                               "coordinates"};
    }
    else
    {
        if (!data.coordinates.empty())
            return CardProblem{"card.bad-value", "the grid points and the coordinates cannot coexist", "coordinates"};
        if (static_cast<std::int64_t>(data.grids.size()) != points)
            return CardProblem{
                "card.bad-value",
                "GRID needs one grid point for each of the " + std::to_string(points) + " control points", "nodeId"};
        for (const base::EntityId grid : data.grids)
        {
            if (grid <= 0)
                return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "nodeId"};
        }
    }
    for (const double value : data.coordinates)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "a coordinate must be a finite number", "coordinates"};
    }
    if (static_cast<std::int64_t>(data.homogeneous.size()) != points)
        return CardProblem{"card.bad-value",
                           "HOMO needs one value for each of the " + std::to_string(points) + " control points",
                           "homogeneous"};
    for (const double value : data.homogeneous)
    {
        if (!(value >= 0.0 && value <= 1.0))
            return CardProblem{"card.bad-value", "a homogeneous coordinate is between 0.0 and 1.0", "homogeneous"};
    }
    if (static_cast<std::int64_t>(data.knots.size()) != knots)
        return CardProblem{"card.bad-value", "KNOT needs " + std::to_string(knots) + " values", "knots"};
    for (const double value : data.knots)
    {
        if (!(value >= 0.0 && value <= 1.0))
            return CardProblem{"card.bad-value", "a knot is between 0.0 and 1.0", "knots"};
    }
    return std::nullopt;
}

} // namespace oor::model
