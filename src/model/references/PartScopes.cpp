#include "oor/model/references/PartScopes.hpp"

#include "oor/model/cards/nodes/moduleBulkOptions/ModuleBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// How many copies of copies are followed before the search gives up (a loop of copies has no end).
constexpr int MAX_COPY_STEPS = 8;

// The part a superelement that is the image of another one (SEBULK REPEAT or MIRROR) copies: a part has no card of the
// IDs it uses that the image holds, so its references are looked up there too. Null for a model that is not such a
// part.
const Model* imageOf(const Model& model)
{
    const Model* owner = model.owner();
    if (owner == nullptr || !model.partInfo() || model.partInfo()->kind != "SuperelementPart" ||
        model.partInfo()->massId != 0)
        return nullptr;
    const SuperelementBulkOptionsTable* bulks = owner->findTable<SuperelementBulkOptionsTable>();
    if (bulks == nullptr)
        return nullptr;
    for (std::uint32_t row = 0; row < bulks->rows().rowCount(); ++row)
    {
        if (!bulks->rows().isAlive(row) || bulks->idAtRow(row) != model.partInfo()->id)
            continue;
        const SuperelementBulkOptionsCard card = bulks->card(row);
        if ((card.partType != SuperelementBulkOptionsType::Repeat &&
             card.partType != SuperelementBulkOptionsType::Mirror) ||
            card.referenceSuperelementId <= 0)
            continue;
        const Model* image = &owner->superelementModel(card.referenceSuperelementId);
        return image == &model || image == owner ? nullptr : image;
    }
    return nullptr;
}

// The primary module that module `module` copies (the RMODID of its MDBULK), 0 when it copies none.
std::int64_t primaryModuleOf(const Model& root, std::int64_t module)
{
    const ModuleBulkOptionsTable* bulks = root.findTable<ModuleBulkOptionsTable>();
    if (bulks == nullptr)
        return 0;
    for (std::uint32_t row = 0; row < bulks->rows().rowCount(); ++row)
    {
        if (!bulks->rows().isAlive(row))
            continue;
        const ModuleBulkOptionsCard card = bulks->card(row);
        if (!card.module.all && card.module.number == module && card.sourceModuleId > 0 &&
            card.sourceModuleId != module)
            return card.sourceModuleId;
    }
    return 0;
}

void addModel(std::vector<const Model*>& models, const Model* candidate, const Model& self)
{
    if (candidate != nullptr && candidate != &self &&
        std::find(models.begin(), models.end(), candidate) == models.end())
        models.push_back(candidate);
}

// The sections of module `module` and of the primary modules it copies, the first section of each first.
std::optional<std::vector<const Model*>> moduleModels(const Model& root, std::int64_t module)
{
    if (module == 0)
        return std::vector<const Model*>{&root};
    std::vector<const Model*> models;
    bool found = false;
    for (int step = 0; module > 0 && step < MAX_COPY_STEPS; ++step)
    {
        for (const std::unique_ptr<Model>& part : root.parts())
        {
            const PartInfo& info = *part->partInfo();
            if (info.kind == "ModuleSection" && info.id == module)
            {
                models.push_back(part.get());
                found = true;
            }
        }
        module = primaryModuleOf(root, module);
    }
    if (!found)
        return std::nullopt;
    return models;
}

} // namespace

std::vector<const Model*> PartScopes::sharedModels(const Model& model)
{
    std::vector<const Model*> models;
    const Model* owner = model.owner();
    const std::optional<PartInfo>& info = model.partInfo();
    if (!info || owner == nullptr)
    {
        // The main Bulk Data of a single-step TPA deck names the points of its active and passive sub-models.
        if (!model.isPart())
        {
            addModel(models, model.findPart("ActivePart", 0), model);
            addModel(models, model.findPart("PassivePart", 0), model);
        }
        return models;
    }
    if (info->kind == "SuperelementPart" && info->massId == 0)
        addModel(models, imageOf(model), model);
    else if (info->kind == "SuperelementPart")
        addModel(models, &owner->superelementModel(info->id), model);
    else if (info->kind == "MassModel")
        addModel(models, owner, model);
    else if (info->kind == "ModuleSection")
    {
        if (const auto sections = moduleModels(*owner, info->id))
        {
            for (const Model* section : *sections)
                addModel(models, section, model);
        }
    }
    else if (info->kind == "ActivePart" || info->kind == "PassivePart")
    {
        addModel(models, owner, model);
        addModel(models, owner->findPart(info->kind == "ActivePart" ? "PassivePart" : "ActivePart", 0), model);
    }
    return models;
}

std::optional<std::vector<const Model*>> PartScopes::modelsOfPart(const Model& anyModelOfDeck, std::string_view kind,
                                                                  std::int64_t id)
{
    const Model& root = anyModelOfDeck.root();
    if (kind == "ModuleSection")
        return moduleModels(root, id);
    std::vector<const Model*> models;
    if (const Model* part = root.findPart(kind, id))
    {
        models.push_back(part);
        for (const Model* shared : sharedModels(*part))
            addModel(models, shared, *part);
    }
    return models;
}

} // namespace oor::model
