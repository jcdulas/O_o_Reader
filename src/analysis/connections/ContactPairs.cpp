#include "oor/analysis/connections/ContactPairs.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableTable.hpp"

#include <algorithm>
#include <span>

namespace oor::analysis
{

namespace
{

// The value of a named parameter as a number; nothing when it is absent, blank or a word.
std::optional<double> numberOf(std::span<const model::ContactParameter> params, std::string_view name)
{
    std::optional<double> value;
    for (const model::ContactParameter& param : params)
    {
        if (param.name != name)
            continue;
        if (const auto* integer = std::get_if<std::int64_t>(&param.value))
            value = static_cast<double>(*integer);
        else if (const auto* real = std::get_if<double>(&param.value))
            value = *real;
    }
    return value; // the last one, when a card gives a name twice
}

// The named parameters a pair gives and the card that gives them ("ContactTable 5", "ContactPairGeomParams 1").
struct ParameterSource
{
    std::vector<model::ContactParameter> params;
    std::string label;
};

// What the parameters of a pair say: its kind, its distance, its search order and the parameters as resolved.
struct PairParameters
{
    ContactKind kind = ContactKind::Touching;
    double tolerance = 0.0;
    bool searchesBothWays = true;
    ContactPairParameters resolved;
};

// The value of `name` from the first source that gives it, with that source.
ResolvedContactParameter resolve(std::initializer_list<const ParameterSource*> sources, std::string_view name)
{
    for (const ParameterSource* source : sources)
    {
        if (source == nullptr)
            continue;
        if (const auto value = numberOf(source->params, name))
            return ResolvedContactParameter{value, source->label};
    }
    return ResolvedContactParameter{};
}

// format reference BCTABLE and BCONPRG: IGLUE not 0 activates the glue (a negative value is the step glue of SOL 400);
// JGLUE, only relevant with the glue, is 0 for a node that does not separate, 1 for the separation on the residual
// force and 2 for the breaking glue. ERROR is the distance below which a node touches a body; DQNEAR (BCTABLE, BCONPRP)
// the distance of near thermal contact: the larger of the two given is the distance of the pair. ISEARCH 1 searches
// from the secondary body only.
//
// The parameters as the solver resolves them: the pair's own fields (`geometric`: a BCTABLE section or a BCONPRG;
// `physical`: a BCONPRP, which gives DQNEAR), else the BCPARA of the model for ERROR and, for a pair without glue, BIAS
// (format reference BCPARA: with a nonzero IGLUE, BIAS comes only from BCTABLE or BCONPRG), else the format
// reference default: BIAS 0.0 with the glue, 0.9 without it; ERROR blank is computed by the solver (the the solver rule
// of ContactSearch).
PairParameters readParameters(const ParameterSource& geometric, const ParameterSource* physical,
                              const ParameterSource* bcpara)
{
    PairParameters result;
    ContactPairParameters& resolved = result.resolved;
    resolved.glueOption = resolve({&geometric}, "GlueOption");
    resolved.glueSeparation = resolve({&geometric}, "GlueSeparationOption");
    resolved.coordinateCheck = resolve({&geometric}, "StressFreeInitialContact");
    resolved.searchOption = resolve({&geometric}, "ContactSearchOrder");
    resolved.errorTolerance = resolve({&geometric, bcpara}, "ContactDistanceTolerance");
    resolved.nearDistance = resolve({&geometric, physical}, "NearContactDistance");
    const double glueOption = resolved.glueOption.value.value_or(0.0);
    const double glueSeparation = resolved.glueSeparation.value.value_or(0.0);
    resolved.toleranceBias = resolve({&geometric}, "ContactToleranceBias");
    if (!resolved.toleranceBias.value)
    {
        resolved.toleranceBias =
            glueOption != 0.0 ? ResolvedContactParameter{} : resolve({bcpara}, "ContactToleranceBias");
        constexpr double GLUE_BIAS = 0.0;
        constexpr double CONTACT_BIAS = 0.9;
        if (!resolved.toleranceBias.value)
            resolved.toleranceBias.value = glueOption != 0.0 ? GLUE_BIAS : CONTACT_BIAS;
    }
    for (ResolvedContactParameter* parameter :
         {&resolved.glueOption, &resolved.glueSeparation, &resolved.coordinateCheck, &resolved.searchOption})
    {
        if (!parameter->value)
            parameter->value = 0.0; // format reference: every one of them defaults to 0
    }
    if (glueOption != 0.0)
        result.kind = glueSeparation == 1.0 || glueSeparation == 2.0 ? ContactKind::Breakable : ContactKind::Glued;
    for (const auto* parameter : {&resolved.errorTolerance, &resolved.nearDistance})
    {
        // the pair's own ERROR and DQNEAR: the BCPARA ERROR is a default of the solver, not the pair's distance
        if (parameter->source != "ContactParams" && parameter->value.value_or(0.0) > 0.0)
            result.tolerance = std::max(result.tolerance, *parameter->value);
    }
    if (physical != nullptr)
    {
        // DQNEAR may come from both the BCONPRG and the BCONPRP: the larger is the distance of the pair
        if (const auto value = numberOf(physical->params, "NearContactDistance"); value && *value > 0.0)
            result.tolerance = std::max(result.tolerance, *value);
    }
    result.searchesBothWays = *resolved.searchOption.value != 1.0;
    if (const auto copts = numberOf(geometric.params, "TouchingBodyOptions"))
        resolved.secondaryContactOption = static_cast<std::int32_t>(*copts);
    if (const auto coptm = numberOf(geometric.params, "TouchedBodyOptions"))
        resolved.primaryContactOption = static_cast<std::int32_t>(*coptm);
    return result;
}

// A body that can take part in the search: it exists, it is deformable and it has something to search.
bool isSearchable(const ContactBody* body) noexcept
{
    return body != nullptr && !body->isRigid && (!body->elements.empty() || !body->grids.empty());
}

class PairCollector
{
public:
    explicit PairCollector(std::vector<ContactPair>& pairs) : m_pairs(pairs) {}

    void add(std::optional<model::CardLocation> table, std::string_view tableName, base::EntityId tableId,
             const ContactBody* secondary, const ContactBody* primary, const PairParameters& parameters,
             std::span<const model::CardLocation> parameterCards)
    {
        if (!isSearchable(secondary) || !isSearchable(primary) || secondary == primary)
            return;
        for (auto existing = m_pairs.rbegin();
             existing != m_pairs.rend() && existing->table && existing->table == table; ++existing)
        {
            // A table that names the same pair twice (a BCONECT with a repeated body) makes it once. The pairs of one
            // table are added one after the other, so only the last ones are looked at.
            if (existing->secondary == secondary && existing->primary == primary)
                return;
        }
        ContactPair pair;
        pair.table = table;
        pair.tableName = std::string(tableName);
        pair.tableId = tableId;
        pair.secondary = secondary;
        pair.primary = primary;
        pair.kind = parameters.kind;
        pair.tolerance = parameters.tolerance;
        pair.searchesBothWays = parameters.searchesBothWays;
        pair.parameters = parameters.resolved;
        if (table)
            pair.cards.push_back(*table);
        pair.cards.insert(pair.cards.end(), parameterCards.begin(), parameterCards.end());
        for (const ContactBody* body : {secondary, primary})
        {
            pair.cards.push_back(body->card);
            pair.cards.insert(pair.cards.end(), body->definitionCards.begin(), body->definitionCards.end());
        }
        m_pairs.push_back(std::move(pair));
    }

private:
    std::vector<ContactPair>& m_pairs;
};

template <typename TableT>
std::optional<std::pair<model::CardLocation, std::vector<model::ContactParameter>>> parameterCard(
    const model::Model& model, base::EntityId id)
{
    if (id == 0)
        return std::nullopt;
    const auto location = model.locateInType(model::typeIndexOf<TableT>(), id);
    if (!location)
        return std::nullopt;
    return std::make_pair(*location, model.findTable<TableT>()->card(location->row).params);
}

} // namespace

std::string_view contactKindName(ContactKind kind) noexcept
{
    switch (kind)
    {
    case ContactKind::Glued:
        return "glued";
    case ContactKind::Breakable:
        return "breakable";
    case ContactKind::Touching:
        return "touching";
    }
    return "";
}

std::string ContactPair::tableLabel() const
{
    return table ? tableName + " " + std::to_string(tableId) : std::string();
}

ContactPairs ContactPairs::read(const model::Model& model, const ContactBodies& bodies)
{
    ContactPairs result;
    PairCollector collector(result.m_pairs);
    bool anyTable = false;

    // one card, of ID 0 (blank), for every subcase; the last one read wins.
    std::vector<model::ContactParameter> modelDefaults;
    forEachCard<model::ContactParamsTable>(model,
                                           [&](const model::ContactParamsCard& card, model::CardLocation)
                                           {
                                               if (card.id == 0)
                                                   modelDefaults = card.params;
                                           });
    const ParameterSource bcpara{modelDefaults, "ContactParams"};
    result.m_errorPerPair = numberOf(modelDefaults, "ContactToleranceBasis").value_or(0.0) == 1.0;
    result.m_thicknessOffInOrder = numberOf(modelDefaults, "IgnoreShellThickness").value_or(0.0) == 1.0;
    // The parameters of a pair that gives none (a legacy table, a Bulk Data without table).
    const auto defaults = [&] { return readParameters(ParameterSource{}, nullptr, &bcpara); };

    forEachCard<model::ContactTableTable>(
        model,
        [&](const model::ContactTableCard& card, model::CardLocation where)
        {
            anyTable = true;
            const std::string label = "ContactTable " + std::to_string(card.id);
            if (card.secondaryBodyId != 0 && card.primaryBodyId != 0)
            {
                // the short form takes the default parameters
                PairParameters parameters = readParameters(ParameterSource{{}, label}, nullptr, &bcpara);
                parameters.resolved.secondaryContactOption = card.secondaryContactOption;
                parameters.resolved.primaryContactOption = card.primaryContactOption;
                collector.add(where, "ContactTable", card.id, bodies.findBody(card.secondaryBodyId),
                              bodies.findBody(card.primaryBodyId), parameters, {});
            }
            for (const model::ContactTableGroup& group : card.groups)
            {
                PairParameters parameters = readParameters(ParameterSource{group.params, label}, nullptr, &bcpara);
                // COPTS1 and COPTM1 set one SECNDRY section; COPTS and COPTM of the first line every section
                parameters.resolved.secondaryContactOption = static_cast<std::int32_t>(
                    numberOf(group.params, "TouchingBodyOptions").value_or(card.secondaryContactOption));
                parameters.resolved.primaryContactOption = static_cast<std::int32_t>(
                    numberOf(group.params, "TouchedBodyOptions").value_or(card.primaryContactOption));
                for (const base::EntityId primary : group.primaryBodies)
                    collector.add(where, "ContactTable", card.id, bodies.findBody(group.secondaryBodyId),
                                  bodies.findBody(primary), parameters, {});
            }
        });

    forEachCard<model::ContactPairTable>(
        model,
        [&](const model::ContactPairCard& card, model::CardLocation where)
        {
            anyTable = true;
            std::vector<model::CardLocation> parameterCards;
            ParameterSource geometric;
            ParameterSource physical;
            if (auto found = parameterCard<model::ContactPairGeomParamsTable>(model, card.geomParamsId))
            {
                parameterCards.push_back(found->first);
                geometric = ParameterSource{std::move(found->second),
                                            "ContactPairGeomParams " + std::to_string(card.geomParamsId)};
            }
            if (auto found = parameterCard<model::ContactPairPhysicalParamsTable>(model, card.physicalParamsId))
            {
                parameterCards.push_back(found->first);
                physical = ParameterSource{std::move(found->second),
                                           "ContactPairPhysicalParams " + std::to_string(card.physicalParamsId)};
            }
            const PairParameters parameters = readParameters(geometric, &physical, &bcpara);
            // format reference BCONECT: the SECNDRY and PRIMARY lists are ignored when the short field
            // is given.
            const std::vector<base::EntityId> secondaries =
                card.secondaryBodyId != 0 ? std::vector<base::EntityId>{card.secondaryBodyId} : card.secondaryBodies;
            const std::vector<base::EntityId> primaries =
                card.primaryBodyId != 0 ? std::vector<base::EntityId>{card.primaryBodyId} : card.primaryBodies;
            for (const base::EntityId secondary : secondaries)
            {
                for (const base::EntityId primary : primaries)
                    collector.add(where, "ContactPair", card.id, bodies.findBody(secondary), bodies.findBody(primary),
                                  parameters, parameterCards);
            }
        });

    forEachCard<model::ContactSetTable>(
        model,
        [&](const model::ContactSetCard& card, model::CardLocation where)
        {
            anyTable = true;
            for (const model::ContactSetPair& pair : card.pairs)
            {
                PairParameters parameters = defaults();
                parameters.tolerance =
                    std::max(pair.maximumDistance, 0.0); // MAXD: the largest distance the search looks at
                collector.add(where, "ContactSet", card.id, bodies.findRegion(pair.sourceRegionId, true),
                              bodies.findRegion(pair.targetRegionId, true), parameters, {});
            }
        });

    forEachCard<model::ContactSlidelineTable>(model,
                                              [&](const model::ContactSlidelineCard& card, model::CardLocation where)
                                              {
                                                  anyTable = true;
                                                  collector.add(where, "ContactSlideline", card.id,
                                                                bodies.findLine(card.secondaryLineId),
                                                                bodies.findLine(card.primaryLineId), defaults(), {});
                                              });

    result.m_fromTables = anyTable;
    if (!anyTable)
    {
        // format reference BCTABLE: with no table every body may touch every other one, with the default
        // parameters.
        const std::vector<ContactBody>& all = bodies.bodies();
        for (std::size_t first = 0; first < all.size(); ++first)
        {
            for (std::size_t second = first + 1; second < all.size(); ++second)
                collector.add(std::nullopt, "", 0, &all[first], &all[second], defaults(), {});
        }
    }
    // The order of the queries: by table name and ID, then by bodies.
    std::stable_sort(result.m_pairs.begin(), result.m_pairs.end(),
                     [](const ContactPair& left, const ContactPair& right)
                     {
                         if (left.tableName != right.tableName)
                             return left.tableName < right.tableName;
                         if (left.tableId != right.tableId)
                             return left.tableId < right.tableId;
                         if (left.secondary->id != right.secondary->id)
                             return left.secondary->id < right.secondary->id;
                         return left.primary->id < right.primary->id;
                     });
    return result;
}

} // namespace oor::analysis
