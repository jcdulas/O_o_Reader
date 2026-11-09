#include "oor/analysis/validation/engine/ValidationEngine.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/validation/engine/ValidationError.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <limits>
#include <memory>

namespace oor::analysis
{

namespace
{

// The label of a part in the findings: "SUPER 1".
std::string labelOf(const model::Model& part)
{
    return part.partInfo() ? part.partLabel() : std::string("part");
}

// The residual structure then every part, nested ones after their parent, with their labels.
void collectModels(const model::Model& model, std::vector<const model::Model*>& models,
                   std::vector<std::string>& labels)
{
    models.push_back(&model);
    labels.push_back(model.isPart() ? labelOf(model) : std::string());
    for (const std::unique_ptr<model::Model>& part : model.parts())
        collectModels(*part, models, labels);
}

// One unit of parallel work: the checks of one family on one model.
struct Task
{
    std::size_t modelIndex = 0;
    std::size_t familyIndex = 0;
    std::vector<const ValidationCheck*> checks;
    std::vector<ValidationFinding> findings;
    std::vector<std::uint32_t> checkOrder; // the position in the registry of the check that made each finding
};

int rank(base::Severity severity) noexcept
{
    switch (severity)
    {
    case base::Severity::Error:
        return 0;
    case base::Severity::Warning:
        return 1;
    case base::Severity::Info:
        return 2;
    }
    return 2;
}

} // namespace

std::vector<const ValidationCheck*> ValidationEngine::select(const ValidationOptions& options)
{
    const CheckRegistry& registry = CheckRegistry::instance();
    for (const std::vector<std::string>* list : {&options.select, &options.ignore})
    {
        for (const std::string& name : *list)
        {
            if (!registry.isKnownName(name))
            {
                throw base::UsageError("unknown check, family or code prefix \"" + name +
                                       "\" (see validation_codes() for the codes and the families)");
            }
        }
    }
    std::vector<const ValidationCheck*> chosen;
    for (const std::unique_ptr<ValidationCheck>& check : registry.checks())
    {
        const CheckInfo& info = check->info();
        bool wanted = false;
        if (options.select.empty())
        {
            wanted = !info.optIn;
        }
        else
        {
            // An opt-in check needs its own code (or "all"): naming its family or its prefix does not run it.
            wanted = std::ranges::any_of(options.select,
                                         [&info](const std::string& name)
                                         {
                                             if (!CheckRegistry::matches(name, info))
                                                 return false;
                                             return !info.optIn || name == info.code || name == "all";
                                         });
        }
        const bool ignored = std::ranges::any_of(
            options.ignore, [&info](const std::string& name) { return CheckRegistry::matches(name, info); });
        if (wanted && !ignored)
            chosen.push_back(check.get());
    }
    return chosen;
}

base::Severity ValidationEngine::defaultMinimum(const std::vector<std::string>& select)
{
    base::Severity floor = base::Severity::Warning;
    for (const std::string& name : select)
    {
        const ValidationCheck* check = CheckRegistry::instance().find(name);
        if (check != nullptr && rank(check->info().severity) > rank(floor))
            floor = check->info().severity;
    }
    return floor;
}

ValidationReport ValidationEngine::run(const model::Model& model, const ValidationOptions& options,
                                       base::ThreadPool* pool, base::ProgressCounter* progress)
{
    const std::vector<const ValidationCheck*> chosen = select(options);
    std::vector<const model::Model*> models;
    std::vector<std::string> labels;
    collectModels(model, models, labels);

    // A copy of the deck for the scan of the references (see ValidationContext::setScanModel): made now, while nothing
    // else runs.
    const model::Model scratch = model.copy();
    std::vector<const model::Model*> scratchModels;
    std::vector<std::string> scratchLabels;
    collectModels(scratch, scratchModels, scratchLabels);

    std::vector<std::unique_ptr<ValidationContext>> contexts;
    for (std::size_t index = 0; index < models.size(); ++index)
    {
        contexts.push_back(std::make_unique<ValidationContext>(model, *models[index], labels[index], index, options));
        contexts.back()->setAllModels(models);
        if (index < scratchModels.size())
            contexts.back()->setScanModel(scratchModels[index]);
    }
    // The positions of the grid points are made once, here, so that the checks that need them only read them. A
    // failure is not reported now: the checks that need positions meet it and say so.
    for (const model::Model* each : models)
    {
        try
        {
            (void)BasicPositionCache::of(*each);
        }
        catch (const base::OorError&)
        {
        }
    }

    // The tasks: for each model, each family that has a check to run there, in the order of the families.
    const std::size_t familyCount = CheckRegistry::families().size();
    std::vector<Task> tasks;
    for (std::size_t modelIndex = 0; modelIndex < models.size(); ++modelIndex)
    {
        for (std::size_t family = 0; family < familyCount; ++family)
        {
            Task task;
            task.modelIndex = modelIndex;
            task.familyIndex = family;
            for (const ValidationCheck* check : chosen)
            {
                const CheckInfo& info = check->info();
                if (CheckRegistry::familyIndex(info.family) != family)
                    continue;
                if (info.scope == CheckScope::Deck && modelIndex != 0)
                    continue; // a check of the deck runs once, on the residual structure
                task.checks.push_back(check);
            }
            if (!task.checks.empty())
                tasks.push_back(std::move(task));
        }
    }
    if (progress != nullptr)
        progress->beginStage("validation", tasks.size());

    const CheckRegistry& registry = CheckRegistry::instance();
    const auto position = [&registry](const ValidationCheck* check)
    {
        for (std::size_t index = 0; index < registry.checks().size(); ++index)
        {
            if (registry.checks()[index].get() == check)
                return static_cast<std::uint32_t>(index);
        }
        return std::numeric_limits<std::uint32_t>::max();
    };

    const auto runTask = [&](std::size_t taskIndex)
    {
        Task& task = tasks[taskIndex];
        const ValidationContext& context = *contexts[task.modelIndex];
        for (const ValidationCheck* check : task.checks)
        {
            if (context.cancelled())
                break;
            const std::size_t before = task.findings.size();
            FindingSink sink(check->info(), context.model(), context.part(), task.findings);
            try
            {
                check->run(context, sink);
            }
            catch (const base::OperationCancelled&)
            {
                throw;
            }
            catch (const base::OorError& error)
            {
                task.findings.resize(before);
                FindingDraft draft;
                draft.message = std::string("the check could not run: ") + error.what();
                FindingSink failure(check->info(), context.model(), context.part(), task.findings);
                failure.report(std::move(draft));
            }
            const std::uint32_t order = position(check);
            task.checkOrder.resize(task.findings.size(), order);
        }
        if (progress != nullptr)
            progress->add(1);
    };
    if (pool != nullptr)
        base::parallelFor(*pool, tasks.size(), runTask, options.token);
    else
    {
        for (std::size_t index = 0; index < tasks.size(); ++index)
        {
            if (options.token != nullptr)
                options.token->throwIfCancelled();
            runTask(index);
        }
    }
    if (options.token != nullptr)
        options.token->throwIfCancelled();

    // Together, in the order: errors first, then the model (residual structure before parts), the family, the card key
    // (a finding with no card last), the check, and the order the check reported in.
    struct Entry
    {
        std::size_t modelIndex;
        std::size_t familyIndex;
        std::uint32_t checkOrder;
        ValidationFinding finding;
    };
    std::vector<Entry> entries;
    for (Task& task : tasks)
    {
        for (std::size_t index = 0; index < task.findings.size(); ++index)
        {
            ValidationFinding& finding = task.findings[index];
            if (rank(finding.severity) > rank(options.minSeverity))
                continue;
            entries.push_back(Entry{task.modelIndex, task.familyIndex, task.checkOrder[index], std::move(finding)});
        }
    }
    std::ranges::stable_sort(entries,
                             [](const Entry& left, const Entry& right)
                             {
                                 if (rank(left.finding.severity) != rank(right.finding.severity))
                                     return rank(left.finding.severity) < rank(right.finding.severity);
                                 if (left.modelIndex != right.modelIndex)
                                     return left.modelIndex < right.modelIndex;
                                 if (left.familyIndex != right.familyIndex)
                                     return left.familyIndex < right.familyIndex;
                                 if (left.finding.cardKey != right.finding.cardKey)
                                     return left.finding.cardKey < right.finding.cardKey;
                                 return left.checkOrder < right.checkOrder;
                             });
    ValidationReport report;
    report.findings.reserve(entries.size());
    for (Entry& entry : entries)
        report.findings.push_back(std::move(entry.finding));
    return report;
}

void ValidationEngine::raiseIfErrors(const ValidationReport& report)
{
    if (report.errorCount() > 0)
        throw ValidationError(report);
}

} // namespace oor::analysis
