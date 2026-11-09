#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/superelements/SuperelementIds.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsTable.hpp"
#include "oor/model/cards/superelements/superelementTree/SuperelementTreeTable.hpp"
#include "oor/model/parameters/ParameterValues.hpp"

#include <algorithm>
#include <cstdlib>

namespace oor::analysis
{

namespace
{

// format reference PARAM CONFAC: the default location tolerance of the superelement connections.
constexpr double DEFAULT_CONNECTION_TOLERANCE = 1.0e-5;

SuperelementType typeOfBulkOptions(model::SuperelementBulkOptionsType type, bool hasPart) noexcept
{
    switch (type)
    {
    case model::SuperelementBulkOptionsType::Primary:
        return hasPart ? SuperelementType::Primary : SuperelementType::List;
    case model::SuperelementBulkOptionsType::Collector:
        return SuperelementType::Collector;
    case model::SuperelementBulkOptionsType::External:
        return SuperelementType::External;
    case model::SuperelementBulkOptionsType::ExternalBinaryResults:
        return SuperelementType::ExternalBinaryResults;
    case model::SuperelementBulkOptionsType::ExternalOutput4:
        return SuperelementType::ExternalMatrixFile;
    case model::SuperelementBulkOptionsType::ExternalPunch:
        return SuperelementType::ExternalPunch;
    case model::SuperelementBulkOptionsType::Repeat:
        return SuperelementType::Repeat;
    case model::SuperelementBulkOptionsType::Mirror:
        return SuperelementType::Mirror;
    case model::SuperelementBulkOptionsType::ExternalReducedModel:
        return SuperelementType::ExternalReducedModel;
    }
    return SuperelementType::Primary;
}

} // namespace

std::string superelementTypeName(SuperelementType type)
{
    switch (type)
    {
    case SuperelementType::Primary:
        return "Primary";
    case SuperelementType::List:
        return "List";
    case SuperelementType::Repeat:
        return "Repeat";
    case SuperelementType::Mirror:
        return "Mirror";
    case SuperelementType::External:
        return "External";
    case SuperelementType::ExternalBinaryResults:
        return "ExternalBinaryResults";
    case SuperelementType::ExternalMatrixFile:
        return "ExternalMatrixFile";
    case SuperelementType::ExternalPunch:
        return "ExternalPunchFile";
    case SuperelementType::ExternalReducedModel:
        return "ExternalReducedModel";
    case SuperelementType::Collector:
        return "CollectorSuperelement";
    case SuperelementType::Secondary:
        return "Secondary";
    }
    return "Primary";
}

bool SuperelementEntry::isExternal() const noexcept
{
    return type == SuperelementType::External || type == SuperelementType::ExternalBinaryResults ||
           type == SuperelementType::ExternalMatrixFile || type == SuperelementType::ExternalPunch ||
           type == SuperelementType::ExternalReducedModel;
}

bool SuperelementEntry::isImage() const noexcept
{
    return type == SuperelementType::Repeat || type == SuperelementType::Mirror || type == SuperelementType::Secondary;
}

bool SuperelementEntry::hasPartPoints() const noexcept
{
    return idModel != nullptr && idModel->isPart();
}

SuperelementCatalogue::SuperelementCatalogue(const model::Model& model) : m_residual(&model.root())
{
    const model::Model& residual = *m_residual;
    m_parameterTolerance = model::realParameter(residual, "ConnectionTolerance").value_or(DEFAULT_CONNECTION_TOLERANCE);
    if (!(m_parameterTolerance > 0.0))
        m_parameterTolerance = DEFAULT_CONNECTION_TOLERANCE;

    std::map<base::EntityId, model::SuperelementBulkOptionsCard> sebulks;
    forEachCard<model::SuperelementBulkOptionsTable>(
        residual, [&](const model::SuperelementBulkOptionsCard& card, model::CardLocation)
        { sebulks[card.superelementId] = card; });
    if (const auto found = sebulks.find(0); found != sebulks.end() && found->second.tolerance > 0.0)
        m_residualTolerance = found->second.tolerance;
    std::map<base::EntityId, model::SuperelementCopyPointsCard> csupers;
    forEachCard<model::SuperelementCopyPointsTable>(
        residual,
        [&](const model::SuperelementCopyPointsCard& card, model::CardLocation) { csupers[card.number()] = card; });
    // format reference SETREE: each SEUPi is upstream of SEID; a superelement no SETREE lists is upstream of the
    // residual structure.
    std::map<base::EntityId, base::EntityId> downstreamOf;
    forEachCard<model::SuperelementTreeTable>(residual,
                                              [&](const model::SuperelementTreeCard& card, model::CardLocation)
                                              {
                                                  for (const base::EntityId upstream : card.upstream)
                                                      downstreamOf[upstream] = card.superelementId;
                                              });
    forEachCard<model::SuperelementConnectionTable>(
        residual,
        [&](const model::SuperelementConnectionCard& card, model::CardLocation)
        {
            if (card.tolerance > 0.0)
                m_connectionTolerances[std::minmax(card.superelementIdA, card.superelementIdB)] = card.tolerance;
        });

    const model::SuperelementIds numbers(residual);
    for (const base::EntityId seid : numbers.sorted())
    {
        SuperelementEntry entry;
        entry.seid = seid;
        entry.part = residual.findPart("SuperelementPart", seid, 0);
        const auto sebulk = sebulks.find(seid);
        const auto csuper = csupers.find(seid);
        if (sebulk != sebulks.end())
        {
            entry.type = typeOfBulkOptions(sebulk->second.partType, entry.part != nullptr);
            entry.method = sebulk->second.method;
            entry.hasOwnTolerance = sebulk->second.tolerance > 0.0;
            entry.tolerance = entry.hasOwnTolerance ? sebulk->second.tolerance : m_parameterTolerance;
            entry.checksLocation = sebulk->second.location == model::SuperelementBulkOptionsLocation::Yes;
            entry.unitNumber = sebulk->second.unitNumber;
            if (entry.type == SuperelementType::Repeat || entry.type == SuperelementType::Mirror)
                entry.imageOf = sebulk->second.referenceSuperelementId;
        }
        else
        {
            entry.tolerance = m_parameterTolerance;
            if (entry.part != nullptr)
                entry.type = SuperelementType::Primary;
            else if (csuper != csupers.end())
                entry.type = SuperelementType::Secondary;
            else
                entry.type = SuperelementType::List;
        }
        if (csuper != csupers.end() && entry.part == nullptr && !entry.isImage())
            entry.type = SuperelementType::Secondary;
        if (entry.type == SuperelementType::Secondary && csuper != csupers.end())
            entry.imageOf = std::abs(csuper->second.primarySuperelementId);
        // An image has the points of its primary, even when it has a BEGIN SUPER section of its own for its loads
        // (public see101nd: SE 1 is the MIRROR of SE 7 and its section holds a PLOAD2 on the points 71 and 72 of 7).
        if (entry.imageOf != 0)
            entry.idModel = &residual.superelementModel(entry.imageOf);
        else if (entry.part != nullptr)
            entry.idModel = entry.part;
        else
            entry.idModel = &residual;
        if (const auto downstream = downstreamOf.find(seid); downstream != downstreamOf.end())
            entry.downstream = downstream->second;
        m_entries.push_back(entry);
    }
}

const SuperelementEntry* SuperelementCatalogue::find(base::EntityId seid) const noexcept
{
    const auto found =
        std::lower_bound(m_entries.begin(), m_entries.end(), seid,
                         [](const SuperelementEntry& entry, base::EntityId id) { return entry.seid < id; });
    return found != m_entries.end() && found->seid == seid ? &*found : nullptr;
}

double SuperelementCatalogue::ownTolerance(base::EntityId seid) const noexcept
{
    if (seid == 0)
        return m_residualTolerance;
    const SuperelementEntry* entry = find(seid);
    return entry != nullptr && entry->hasOwnTolerance ? entry->tolerance : 0.0;
}

double SuperelementCatalogue::searchTolerance(base::EntityId first, base::EntityId second) const noexcept
{
    if (const auto found = m_connectionTolerances.find(std::minmax(first, second));
        found != m_connectionTolerances.end())
        return found->second;
    const double own = std::max(ownTolerance(first), ownTolerance(second));
    return own > 0.0 ? own : m_parameterTolerance;
}

double SuperelementCatalogue::largestTolerance() const noexcept
{
    double largest = std::max(m_parameterTolerance, m_residualTolerance);
    for (const SuperelementEntry& entry : m_entries)
        largest = std::max(largest, entry.tolerance);
    for (const auto& [pair, tolerance] : m_connectionTolerances)
        largest = std::max(largest, tolerance);
    return largest;
}

double SuperelementCatalogue::connectionTolerance(base::EntityId seida, double pairTolerance) const noexcept
{
    if (pairTolerance > 0.0)
        return pairTolerance;
    const double own = ownTolerance(seida);
    return own > 0.0 ? own : m_parameterTolerance;
}

} // namespace oor::analysis
