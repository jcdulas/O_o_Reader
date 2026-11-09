#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/validation/coordinates/CoordinateChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <vector>

namespace oor::analysis
{

namespace
{

// A coordinate system whose three points do not define axes: two of them coincide, or the three are on a line (the axis
// of the system is zero, or the x-z plane is). A system that cannot be resolved for another reason is left to the check
// that says why (coord.cycle, ref.unresolved).
class CoordinateDegenerateCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"coord.degenerate", base::Severity::Error, "coordinates",
                                    "The three points of a coordinate system coincide or are on a line."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        CoordinateTransform transform(model);
        // In ID order, so that the findings do not depend on the hash order of the definitions.
        std::vector<base::EntityId> ids;
        for (const auto& entry : transform.definitions())
            ids.push_back(entry.first);
        std::ranges::sort(ids);
        for (const base::EntityId cid : ids)
        {
            const SystemDefinition& definition = transform.definitions().at(cid);
            try
            {
                std::array<base::Vec3, 3> points;
                if (definition.form == SystemDefinition::Form::ThreePoints)
                {
                    const std::array<base::Vec3, 3> given{
                        base::Vec3{definition.points[0][0], definition.points[0][1], definition.points[0][2]},
                        base::Vec3{definition.points[1][0], definition.points[1][1], definition.points[1][2]},
                        base::Vec3{definition.points[2][0], definition.points[2][1], definition.points[2][2]}};
                    for (std::size_t index = 0; index < 3; ++index)
                        points[index] = transform.toBasic(definition.referenceFrameId, given[index]);
                }
                else
                {
                    for (std::size_t index = 0; index < 3; ++index)
                        points[index] = transform.basicPositionOfNode(definition.nodes[index]);
                }
                CoordinateFrame frame;
                if (frameFromThreePoints(definition.kind, points[0], points[1], points[2], frame))
                    continue;
            }
            catch (const base::OorError&)
            {
                continue;
            }
            if (const std::optional<model::CardLocation> where = model.locate(base::IdSpace::Coordinate, cid))
                sink.reportCard(*where, "",
                                "the three points do not define axes (two coincide, or all three are on a line)",
                                "move one of the points: B gives the z axis, C the x-z plane");
        }
    }
};

} // namespace

void addCoordinateDegenerateCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CoordinateDegenerateCheck>());
}

} // namespace oor::analysis
