#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"
#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorTable.hpp"

#include <unordered_set>

namespace oor::analysis
{

namespace
{

// A MESH of TYPE ADAPT that no COUPLE names. An adaptive mesh is built around a coupling surface and follows it, so the
// format reference requires the MID of the MESH to be the MESHID of a COUPLE; without one the mesh has nothing to adapt
// to.
class EulerMeshAdaptWithoutCouplingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.euler-mesh-adapt-without-coupling", base::Severity::Error, "explicit",
                                    "An adaptive Euler mesh is not the mesh of any Euler-Lagrange coupling."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::unordered_set<base::EntityId> named;
        forEachCard<model::EulerLagrangeCouplingTable>(model, [&](const model::EulerLagrangeCouplingCard& card,
                                                                  model::CardLocation) { named.insert(card.meshId); });
        forEachCard<model::EulerMeshGeneratorTable>(
            model,
            [&](const model::EulerMeshGeneratorCard& card, model::CardLocation where)
            {
                if (card.type != model::EulerMeshGeneratorType::Adapt || named.contains(card.id))
                    return;
                sink.reportCard(where, "meshType",
                                "no " + std::string("EulerLagrangeCoupling") + " names this " +
                                    std::string("EulerMeshGenerator") + " in its " + std::string("meshId"),
                                "give the " + std::string("meshId") + " of a " + std::string("EulerLagrangeCoupling") +
                                    ", or use " + std::string("meshType") + " BOX");
            });
    }
};

} // namespace

void addEulerMeshAdaptWithoutCouplingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EulerMeshAdaptWithoutCouplingCheck>());
}

} // namespace oor::analysis
