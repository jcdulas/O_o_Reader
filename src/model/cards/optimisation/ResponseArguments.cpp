#include "oor/model/cards/optimisation/ResponseArguments.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr base::EntityId MAX_COMPONENT = 3; // DNODE: the components of a grid point are 1 to 3 (format reference: Cm)

} // namespace

std::optional<CardProblem> checkResponseArguments(const std::vector<DesignGroup>& groups,
                                                  std::span<const GroupKeyword> keywords)
{
    if (const auto problem = checkDesignGroups(groups, keywords))
        return problem;
    bool hasArgument = false;
    for (const DesignGroup& group : groups)
    {
        hasArgument = hasArgument || !group.ids.empty() || !group.words.empty();
        if (group.keyword != "DesignNodes")
            continue;
        for (std::size_t index = 1; index < group.ids.size(); index += 2)
        {
            if (group.ids[index] < 1 || group.ids[index] > MAX_COMPONENT)
                return CardProblem{"card.bad-value", "the component of a DNODE grid point must be 1, 2 or 3",
                                   "nodeComponents"};
        }
    }
    if (!hasArgument)
        return CardProblem{"card.missing-field",
                           "at least one argument is required: a " + CardProblem::cardNamed("DesignVar") + ", " +
                               CardProblem::cardNamed("DesignConstants") + ", " +
                               CardProblem::cardNamed("DesignResp1") + ", DNODE, relation or " +
                               CardProblem::cardNamed("DesignResp2") + " group",
                           "groupKeyword"};
    return std::nullopt;
}

void visitResponseArguments(ReferenceVisitor& visitor, std::vector<DesignGroup>& groups)
{
    for (DesignGroup& group : groups)
    {
        if (group.keyword == "DesignVar")
            visitGroupIds(visitor, group, "designVarIds", ReferenceTarget::DesignVariable);
        else if (group.keyword == "DesignResp1")
            visitGroupIds(visitor, group, "responseIds", ReferenceTarget::DesignResponse);
        else if (group.keyword == "DesignResp2")
            visitGroupIds(visitor, group, "combinedResponseIds", ReferenceTarget::DesignResponse);
        else if (group.keyword == "DesignPropRelation1" || group.keyword == "DesignPropRelation2")
            visitGroupIds(visitor, group, "propRelationIds", ReferenceTarget::PropertyRelation);
        else if (group.keyword == "DesignElemRelation1" || group.keyword == "DesignElemRelation2")
            visitGroupIds(visitor, group, "elemRelationIds", ReferenceTarget::ConnectivityRelation);
        else if (group.keyword == "DesignMatRelation1" || group.keyword == "DesignMatRelation2")
            visitGroupIds(visitor, group, "matRelationIds", ReferenceTarget::MaterialRelation);
        else if (group.keyword == "DesignNodes")
        {
            // The grid points are every other number: G1 C1 G2 C2 ...
            for (std::size_t index = 0; index < group.ids.size(); index += 2)
                visitId(visitor, "nodeIds", ReferenceTarget::Node, group.ids[index]);
        }
    }
}

} // namespace oor::model
