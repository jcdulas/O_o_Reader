#include "AeroModelLinks.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// A label as the cards compare it: upper case, with no blank around it.
std::string labelKey(const std::string& label)
{
    const std::size_t first = label.find_first_not_of(' ');
    if (first == std::string::npos)
        return {};
    const std::size_t last = label.find_last_not_of(' ');
    return std::string(std::string_view(label).substr(first, last - first + 1));
}

} // namespace

std::string AeroModelLinks::editedNote(std::size_t set) const
{
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr || set >= m_context.sets().size())
        return {};
    const std::shared_ptr<results::ResultAttachment>& attachment = m_context.sets()[set].attachment;
    if (!attachment || attachment->state(*model) == results::AttachmentState::Current)
        return {};
    return " (the model was edited after the results were attached)";
}

AeroModelLinks::SetData& AeroModelLinks::dataOf(std::size_t set)
{
    return m_data[m_context.modelOf(set)];
}

IdLookup AeroModelLinks::panelsOfBoxes(std::size_t set, std::span<const std::int32_t> boxes)
{
    IdLookup lookup;
    lookup.id.assign(boxes.size(), 0);
    lookup.valid.assign(boxes.size(), 0);
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr)
        return lookup;
    SetData& data = dataOf(set);
    if (!data.boxes)
        data.boxes = std::make_unique<model::AeroBoxRanges>(*model);
    for (std::size_t row = 0; row < boxes.size(); ++row)
    {
        const model::AeroBoxRange* owner = data.boxes->owner(boxes[row]);
        if (owner == nullptr)
        {
            ++lookup.withoutCard;
            continue;
        }
        lookup.id[row] = owner->panelId;
        lookup.valid[row] = 1;
        ++lookup.found;
    }
    return lookup;
}

IdLookup AeroModelLinks::surfacesOfLabels(std::size_t set, const std::vector<std::string>& labels)
{
    IdLookup lookup;
    lookup.id.assign(labels.size(), 0);
    lookup.valid.assign(labels.size(), 0);
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr)
        return lookup;
    SetData& data = dataOf(set);
    if (!data.surfacesRead)
    {
        data.surfacesRead = true;
        analysis::forEachCard<model::AeroControlSurfaceTable>(
            *model,
            [&](const model::AeroControlSurfaceCard& card, model::CardLocation)
            {
                // two cards with one label name no surface for certain
                // (`aero.label-repeated` reports it)
                const auto [position, inserted] = data.surfaces.try_emplace(labelKey(card.label), card.id);
                if (!inserted)
                    position->second = 0;
            });
    }
    for (std::size_t row = 0; row < labels.size(); ++row)
    {
        const auto found = data.surfaces.find(labelKey(labels[row]));
        if (found == data.surfaces.end())
        {
            ++lookup.withoutCard;
            continue;
        }
        if (found->second == 0)
        {
            ++lookup.ambiguous;
            continue;
        }
        lookup.id[row] = found->second;
        lookup.valid[row] = 1;
        ++lookup.found;
    }
    return lookup;
}

void noteLookup(RowAccumulator& accumulator, const AeroModelLinks& links, std::size_t set, const IdLookup& lookup,
                std::size_t rows, const LookupWords& words)
{
    const std::vector<std::string> column{words.column};
    if (!links.hasModel(set))
    {
        accumulator.note(accumulator.addProvider(words.noModel, false), column, static_cast<std::int64_t>(rows));
        return;
    }
    const std::string edited = links.editedNote(set);
    const auto add = [&](const std::string& name, std::size_t count)
    {
        if (count > 0)
            accumulator.note(accumulator.addProvider(name + edited, false), column, static_cast<std::int64_t>(count));
    };
    add(words.found, lookup.found);
    add(words.withoutCard, lookup.withoutCard);
    add(words.ambiguous, lookup.ambiguous);
}

std::vector<std::int32_t> boxesOfForces(std::span<const std::int32_t> pressureBoxes,
                                        const std::vector<std::string>& pressureKinds,
                                        const std::vector<std::string>& forceKinds)
{
    if (pressureBoxes.empty() || pressureBoxes.size() != forceKinds.size() || pressureKinds != forceKinds)
        return {};
    return std::vector<std::int32_t>(pressureBoxes.begin(), pressureBoxes.end());
}

} // namespace oor::query
