#include "SplineEntries.hpp"

#include "AeroQuerySupport.hpp"
#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"
#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Table.hpp"
#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Table.hpp"
#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"
#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline4/AeroSurfaceSpline4Table.hpp"
#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineTable.hpp"

#include <algorithm>
#include <set>

namespace oor::query::aero
{

namespace
{

std::string_view usageWord(model::SplineUsage usage)
{
    return model::SPLINE_USAGE_WORDS[static_cast<std::size_t>(usage)];
}

void addSurfaceSplines1(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroSurfaceSpline1Card& card : cardsOf<model::AeroSurfaceSpline1Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroSurfaceSpline1";
        entry.caero = card.aeroPanelId;
        entry.setg = card.nodeSetId;
        entry.boxRange = std::pair{card.firstBox, card.lastBox};
        entry.dz = card.linearFlexibility;
        entry.method = model::AERO_SURFACE_SPLINE1_METHOD_WORDS[static_cast<std::size_t>(card.method)];
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

void addBeamSplines2(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroBeamSpline2Card& card : cardsOf<model::AeroBeamSpline2Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroBeamSpline2";
        entry.caero = card.aeroPanelId;
        entry.setg = card.nodeSetId;
        entry.boxRange = std::pair{card.firstBox, card.lastBox};
        entry.dz = card.linearFlexibility;
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

// A SPLINE3 ties one component of one box to structural degrees of freedom: its structural points are the G of its
// terms, listed on the card.
void addSplineEquations3(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroSplineEquation3Card& card : cardsOf<model::AeroSplineEquation3Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroSplineEquation3";
        entry.caero = card.aeroPanelId;
        entry.boxRange = std::pair{card.boxId, card.boxId};
        entry.usage = usageWord(card.usage);
        entry.hasPointList = true;
        const std::set<base::EntityId> distinct = [&]
        {
            std::set<base::EntityId> points;
            for (const model::AeroSplineEquation3Point& point : card.points)
                points.insert(point.node);
            return points;
        }();
        entry.points.assign(distinct.begin(), distinct.end());
        entries.push_back(std::move(entry));
    }
}

void addSurfaceSplines4(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroSurfaceSpline4Card& card : cardsOf<model::AeroSurfaceSpline4Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroSurfaceSpline4";
        entry.caero = card.aeroPanelId;
        entry.aelist = card.aeroBoxListId;
        entry.setg = card.nodeSetId;
        entry.dz = card.linearFlexibility;
        entry.method = model::AERO_SURFACE_SPLINE4_METHOD_WORDS[static_cast<std::size_t>(card.method)];
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

void addBeamSplines5(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroBeamSpline5Card& card : cardsOf<model::AeroBeamSpline5Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroBeamSpline5";
        entry.caero = card.aeroPanelId;
        entry.aelist = card.aeroBoxListId;
        entry.setg = card.nodeSetId;
        entry.dz = card.linearFlexibility;
        entry.method = model::AERO_BEAM_SPLINE5_METHOD_WORDS[static_cast<std::size_t>(card.method)];
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

void addFiniteSurfaceSplines6(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroFiniteSurfaceSpline6Card& card : cardsOf<model::AeroFiniteSurfaceSpline6Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroFiniteSurfaceSpline6";
        entry.caero = card.aeroPanelId;
        entry.aelist = card.aeroBoxListId;
        entry.setg = card.nodeSetId;
        entry.dz = card.linearFlexibility;
        entry.method = model::AERO_FINITE_SURFACE_SPLINE6_METHOD_WORDS[static_cast<std::size_t>(card.method)];
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

void addFiniteBeamSplines7(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroFiniteBeamSpline7Card& card : cardsOf<model::AeroFiniteBeamSpline7Table>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroFiniteBeamSpline7";
        entry.caero = card.aeroPanelId;
        entry.aelist = card.aeroBoxListId;
        entry.setg = card.nodeSetId;
        entry.dz = card.linearFlexibility;
        entry.method = model::AERO_FINITE_BEAM_SPLINE7_METHOD_WORDS[static_cast<std::size_t>(card.method)];
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

// A SPLINEX is evaluated by a program of the user: it has no CAERO, no SET and no flexibility, only the AELIST the
// program reads (optional) and the components it joins, which are named by AECOMP cards.
void addUserSplines(const model::Model& model, std::vector<SplineEntry>& entries)
{
    for (const model::AeroUserSplineCard& card : cardsOf<model::AeroUserSplineTable>(model))
    {
        SplineEntry entry;
        entry.eid = card.id;
        entry.type = "AeroUserSpline";
        entry.aelist = card.aeroBoxListId;
        entry.usage = usageWord(card.usage);
        entries.push_back(std::move(entry));
    }
}

} // namespace

std::vector<SplineEntry> readSplines(const model::Model& model)
{
    std::vector<SplineEntry> entries;
    addSurfaceSplines1(model, entries);
    addBeamSplines2(model, entries);
    addSplineEquations3(model, entries);
    addSurfaceSplines4(model, entries);
    addBeamSplines5(model, entries);
    addFiniteSurfaceSplines6(model, entries);
    addFiniteBeamSplines7(model, entries);
    addUserSplines(model, entries);
    std::ranges::stable_sort(entries, {}, &SplineEntry::eid);
    return entries;
}

} // namespace oor::query::aero
