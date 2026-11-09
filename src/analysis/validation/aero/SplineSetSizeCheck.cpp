#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"
#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline4/AeroSurfaceSpline4Table.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/cards/sets/typedListSet3/TypedListSet3Table.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <map>
#include <string>

namespace oor::analysis
{

namespace
{

// A spline fits a surface or a beam through the structural grid points of its SETG, and a fit needs points: a beam
// spline (SPLINE2, SPLINE5) at least two, a surface spline (SPLINE1, SPLINE4) at least three. Only the grid points of
// the model count (a number of the set that is no grid point is not a point of the spline). A warning: with the
// attachment flexibilities the solver may still run, but the spline cannot say what the surface does between so few
// points. A set that does not exist is `ref.unresolved`.
class SplineSetSizeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.spline-set-size", base::Severity::Warning, "aero",
        "The set of structural grid points of a spline has fewer points than the spline needs."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        Counts counts;
        forEachCard<model::AeroSurfaceSpline1Table>(
            model, [&](const model::AeroSurfaceSpline1Card& card, model::CardLocation where)
            { look(model, sink, where, counts, "AeroSurfaceSpline1", card.nodeSetId, SURFACE_POINTS); });
        forEachCard<model::AeroBeamSpline2Table>(
            model, [&](const model::AeroBeamSpline2Card& card, model::CardLocation where)
            { look(model, sink, where, counts, "AeroBeamSpline2", card.nodeSetId, BEAM_POINTS); });
        forEachCard<model::AeroSurfaceSpline4Table>(
            model, [&](const model::AeroSurfaceSpline4Card& card, model::CardLocation where)
            { look(model, sink, where, counts, "AeroSurfaceSpline4", card.nodeSetId, SURFACE_POINTS); });
        forEachCard<model::AeroBeamSpline5Table>(
            model, [&](const model::AeroBeamSpline5Card& card, model::CardLocation where)
            { look(model, sink, where, counts, "AeroBeamSpline5", card.nodeSetId, BEAM_POINTS); });
    }

private:
    static constexpr std::size_t BEAM_POINTS = 2;
    static constexpr std::size_t SURFACE_POINTS = 3;
    // The IDs of the set that are examined at most: a THRU over millions of numbers is not walked to the end.
    static constexpr std::size_t MAX_EXAMINED = 2'000'000;

    // The grid points each set has, counted once (up to SURFACE_POINTS, which is all a check needs); nothing when the
    // set has no SET1 or SET3 of grid points, or when it was too long to examine.
    using Counts = std::map<base::EntityId, std::optional<std::size_t>>;

    static std::optional<std::size_t> gridPointsOf(const model::Model& model, base::EntityId sid)
    {
        std::size_t points = 0;
        std::size_t examined = 0;
        bool found = false;
        for (const model::CardLocation location : model::cardsOfSet(model, model::CardCategory::Set, sid))
        {
            std::vector<model::IdRun> runs;
            if (isType<model::IdListSet1Table>(location))
            {
                runs = cardAt<model::IdListSet1Table>(model, location).ids;
            }
            else if (isType<model::TypedListSet3Table>(location))
            {
                const model::TypedListSet3Card card = cardAt<model::TypedListSet3Table>(model, location);
                if (card.kind != model::TypedListSet3Kind::Grid)
                    continue;
                runs = card.ids;
            }
            else
            {
                continue;
            }
            found = true;
            for (const model::IdRun& run : runs)
            {
                for (std::int64_t id = run.first; id <= run.last; ++id)
                {
                    if (points >= SURFACE_POINTS)
                        return points;
                    if (++examined > MAX_EXAMINED)
                        return std::nullopt;
                    if (isGridPoint(model, static_cast<base::EntityId>(id)))
                        ++points;
                }
            }
        }
        return found ? std::optional<std::size_t>(points) : std::nullopt;
    }

    // `spline`: the card type of the spline.
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, Counts& counts,
                     std::string_view spline, base::EntityId setg, std::size_t needed)
    {
        if (setg == 0)
            return;
        auto found = counts.find(setg);
        if (found == counts.end())
            found = counts.emplace(setg, gridPointsOf(model, setg)).first;
        if (found->second && *found->second < needed)
        {
            sink.reportCard(where, "nodeSetId",
                            "the set " + std::to_string(setg) + " has " + std::to_string(*found->second) +
                                " grid point" + (*found->second == 1 ? "" : "s") + ", and a " + std::string(spline) +
                                " needs at least " + std::to_string(needed),
                            "list more structural grid points");
        }
    }
};

} // namespace

void addSplineSetSizeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SplineSetSizeCheck>());
}

} // namespace oor::analysis
