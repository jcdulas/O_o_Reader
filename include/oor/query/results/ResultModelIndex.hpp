#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>

namespace oor::query
{

// What the model says of the elements a result set holds, by the element ID of the file (through the attachment, so it
// follows a renumbering): the property, the main material, the component, the location of the card. Each is made when
// first asked. Everything is 0 or nothing for a set without a model and for an element that is not linked.
class ResultModelIndex
{
public:
    explicit ResultModelIndex(const ResultQueryContext& context) : m_context(context) {}

    [[nodiscard]] std::int32_t property(std::size_t set, std::int32_t resultEid);
    // The main material: the one a CONROD gives, else the first one of the property (MID1 of a PSHELL).
    [[nodiscard]] std::int32_t material(std::size_t set, std::int32_t resultEid);
    [[nodiscard]] std::int32_t component(std::size_t set, std::int32_t resultEid);
    [[nodiscard]] std::optional<model::CardLocation> element(std::size_t set, std::int32_t resultEid) const;
    [[nodiscard]] std::optional<model::CardLocation> propertyLocation(std::size_t set, std::int32_t resultEid);

private:
    struct PerModel
    {
        std::unique_ptr<CardFacts> facts;
        std::shared_ptr<const analysis::ComponentModel> components;
    };
    PerModel& of(std::size_t set);

    const ResultQueryContext& m_context;
    std::map<const model::Model*, PerModel> m_models;
    std::map<std::uint64_t, std::int32_t> m_materials;
};

} // namespace oor::query
