#include "oor/model/cards/optimisation/RelationVariables.hpp"

#include "oor/model/cards/optimisation/DesignGroups.hpp"

#include <array>

namespace oor::model
{

namespace
{

constexpr std::array<GroupKeyword, 2> RELATION_GROUPS{{
    {"DesignVar", GroupContent::Integers, "designVarIds"},
    {"DesignConstants", GroupContent::Words, "constantLabels"},
}};

} // namespace

void readRelationVariables(FieldReader& reader, int firstField, std::vector<base::EntityId>& designVarIds,
                           std::vector<std::string>& labels)
{
    designVarIds.clear();
    labels.clear();
    for (DesignGroup& group : readDesignGroups(reader, firstField, RELATION_GROUPS))
    {
        if (group.keyword == "DesignVar")
            designVarIds = std::move(group.ids);
        else
            labels = std::move(group.words);
    }
}

void writeRelationVariables(FieldWriter& writer, int firstField, const std::vector<base::EntityId>& designVarIds,
                            const std::vector<std::string>& labels)
{
    std::vector<DesignGroup> groups;
    if (!designVarIds.empty())
        groups.push_back(DesignGroup{"DesignVar", designVarIds, {}});
    if (!labels.empty())
        groups.push_back(DesignGroup{"DesignConstants", {}, labels});
    writeDesignGroups(writer, firstField, groups, RELATION_GROUPS);
}

std::optional<CardProblem> checkRelationVariables(const std::vector<base::EntityId>& designVarIds,
                                                  const std::vector<std::string>& labels)
{
    if (designVarIds.empty() && labels.empty())
        return CardProblem{"card.missing-field",
                           "a relation needs at least one design variable (" + CardProblem::cardNamed("DesignVar") +
                               ") or constant (" + CardProblem::cardNamed("DesignConstants") + ")",
                           "designVarIds"};
    std::vector<DesignGroup> groups;
    groups.push_back(DesignGroup{"DesignVar", designVarIds, {}});
    groups.push_back(DesignGroup{"DesignConstants", {}, labels});
    if (const auto problem = checkDesignGroups(groups, RELATION_GROUPS))
        return problem;
    return std::nullopt;
}

} // namespace oor::model
