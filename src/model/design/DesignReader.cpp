#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/designBeadRegion/DesignBeadRegionTable.hpp"
#include "oor/model/cards/optimisation/designBoundaryShape/DesignBoundaryShapeTable.hpp"
#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"
#include "oor/model/cards/optimisation/designControlSurface/DesignControlSurfaceTable.hpp"
#include "oor/model/cards/optimisation/designElemRelation1/DesignElemRelation1Table.hpp"
#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Table.hpp"
#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"
#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"
#include "oor/model/cards/optimisation/designLoadRelation1/DesignLoadRelation1Table.hpp"
#include "oor/model/cards/optimisation/designMatRelation1/DesignMatRelation1Table.hpp"
#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"
#include "oor/model/cards/optimisation/designNodeMove/DesignNodeMoveTable.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsValues.hpp"
#include "oor/model/cards/optimisation/designPropRelation1/DesignPropRelation1Table.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"
#include "oor/model/cards/optimisation/designShapeBasis/DesignShapeBasisTable.hpp"
#include "oor/model/cards/optimisation/designTopologyRegion/DesignTopologyRegionTable.hpp"
#include "oor/model/cards/optimisation/designTopometryRegion/DesignTopometryRegionTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkTable.hpp"
#include "oor/model/design/DesignErrors.hpp"
#include "oor/model/design/DesignModel.hpp"

#include <algorithm>
#include <set>

namespace oor::model
{

namespace
{

constexpr double LOWEST_OF_POSITIVE_FIELD = 1.0e-15; // the format reference: PMIN of a field that can only be positive
constexpr double NO_LOWER_LIMIT = -1.0e35;
constexpr const char* NO_FIELDS_OF_THIS_CARD = "the library cannot design the fields of a ";
// The TYPE of a property relation that designs a ply by its global ply ID.
constexpr std::string_view GLOBAL_PLY_KIND = "GlobalPly";

// Calls `body(card, row, key)` for every live card of a table.
template <typename TableT, typename Body>
void forEachCard(const Model& model, Body&& body)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    const RowBook& rows = table->rows();
    for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
    {
        if (rows.isAlive(row))
            body(table->card(row), row, rows.keyAt(row));
    }
}

std::string joined(const std::vector<std::string>& words)
{
    std::string text;
    for (const std::string& word : words)
        text += (text.empty() ? "" : ", ") + word;
    return text;
}

base::IdSpace spaceOf(DesignTargetKind kind)
{
    switch (kind)
    {
    case DesignTargetKind::Property:
        return base::IdSpace::Property;
    case DesignTargetKind::Material:
        return base::IdSpace::Material;
    default:
        return base::IdSpace::Element;
    }
}

const char* nameOf(DesignTargetKind kind)
{
    switch (kind)
    {
    case DesignTargetKind::Property:
        return "property";
    case DesignTargetKind::Material:
        return "material";
    default:
        return "element";
    }
}

} // namespace

// Reads the cards of the design model into a DesignModel and works out what each relation drives.
class DesignReader
{
public:
    DesignReader(const Model& model, DesignModel& design) : m_model(model), m_design(design) {}

    void run()
    {
        m_design.m_contentVersion = m_model.contentVersion();
        readVariables();
        readLinks();
        readConstants();
        readEquations();
        readRelations();
        readCardsThatAreNotApplied();
        collectUsedVariables();
    }

private:
    void readVariables()
    {
        forEachCard<DesignVarTable>(m_model,
                                    [&](const DesignVarCard& card, std::uint32_t row, base::CardKey key)
                                    {
                                        DesignVariable variable;
                                        variable.id = card.id;
                                        variable.label = card.entryLabel;
                                        variable.initial = card.initialValue;
                                        variable.lower = card.lowerBound;
                                        variable.upper = card.upperBound;
                                        variable.key = key;
                                        variable.row = row;
                                        variable.discreteValues = card.discreteValuesId;
                                        if (card.designVarMoveLimit)
                                            variable.moveLimit = *card.designVarMoveLimit;
                                        else
                                            variable.moveLimit =
                                                designOptimParamsNumber(m_model, "DesignVarMoveLimit").value_or(0.5);
                                        m_design.m_variables.push_back(std::move(variable));
                                    });
        std::sort(m_design.m_variables.begin(), m_design.m_variables.end(),
                  [](const DesignVariable& left, const DesignVariable& right) { return left.id < right.id; });
    }

    // A variable is dependent when a DLINK gives it. a dependent variable is not
    // independent in another DLINK, so the values of the independent variables are always the ones of the point.
    void readLinks()
    {
        std::set<base::EntityId> independent;
        forEachCard<DesignVarLinkTable>(m_model,
                                        [&](const DesignVarLinkCard& card, std::uint32_t, base::CardKey)
                                        {
                                            DesignLink link;
                                            link.id = card.id;
                                            link.dependent = card.dependent;
                                            link.constant = card.constant0;
                                            link.multiplier = card.multiplier;
                                            link.terms = card.terms;
                                            for (const DesignTerm& term : card.terms)
                                                independent.insert(term.designVarId);
                                            m_design.m_links.push_back(std::move(link));
                                        });
        for (const DesignLink& link : m_design.m_links)
        {
            const std::optional<std::size_t> dependent = m_design.findVariable(link.dependent);
            if (!dependent)
            {
                problem("DLINK " + std::to_string(link.id) + " makes the design variable " +
                        std::to_string(link.dependent) + " depend on others, and no DESVAR defines it");
                continue;
            }
            DesignVariable& variable = m_design.m_variables[*dependent];
            if (variable.link != 0)
                problem("the design variable " + std::to_string(variable.id) + " is the dependent variable of DLINK " +
                        std::to_string(variable.link) + " and of DLINK " + std::to_string(link.id));
            variable.link = link.id;
            if (independent.contains(link.dependent))
                problem("the design variable " + std::to_string(link.dependent) + " is dependent in DLINK " +
                        std::to_string(link.id) +
                        " and independent in another DLINK (the format reference does not allow it)");
            for (const DesignTerm& term : link.terms)
            {
                if (!m_design.findVariable(term.designVarId))
                    problem("DLINK " + std::to_string(link.id) + " names the design variable " +
                            std::to_string(term.designVarId) + ", which no DESVAR defines");
            }
        }
    }

    void readConstants()
    {
        forEachCard<DesignConstantsTable>(
            m_model,
            [&](const DesignConstantsCard& card, std::uint32_t, base::CardKey)
            {
                for (const DesignConstantsEntry& entry : card.entries)
                {
                    const std::string label = std::string(entry.label);
                    if (entry.table != 0)
                        m_design.m_unavailable.emplace(label, "the constant " + entry.label + " is the dynamic table " +
                                                                  std::to_string(entry.table) +
                                                                  ", a function of frequency or time, not a number");
                    else
                        m_design.m_values.emplace(label, entry.value);
                }
            });
        forEachCard<DesignFieldConstantsTable>(
            m_model,
            [&](const DesignFieldConstantsCard& card, std::uint32_t, base::CardKey)
            {
                for (const DesignFieldConstantsEntry& entry : card.entries)
                    m_design.m_fieldConstants.emplace(
                        std::string(entry.label),
                        DesignModel::FieldConstant{entry.cardName, entry.targetId, entry.field});
            });
    }

    void readEquations()
    {
        forEachCard<DesignEquationTable>(m_model,
                                         [&](const DesignEquationCard& card, std::uint32_t, base::CardKey)
                                         {
                                             EquationParse parsed = parseEquations(card.text);
                                             if (parsed.ok())
                                                 m_design.m_equations.emplace(card.id, std::move(*parsed.equations));
                                         });
    }

    // The relations of each card type, by ID (cards that share an ID stay in file order), the types in the order
    // DVPREL1, DVPREL2, DVMREL1, DVMREL2, DVCREL1, DVCREL2.
    void readRelations()
    {
        DesignRelation relation;
        std::size_t first = 0;
        const auto sortSinceFirst = [&]
        {
            std::stable_sort(m_design.m_relations.begin() + static_cast<std::ptrdiff_t>(first),
                             m_design.m_relations.end(), [](const DesignRelation& left, const DesignRelation& right)
                             { return left.id < right.id; });
            first = m_design.m_relations.size();
        };
        forEachCard<DesignPropRelation1Table>(
            m_model,
            [&](const DesignPropRelation1Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignPropRelation1Table>(), row, key, card.id, DesignTargetKind::Property,
                                 DesignRelationShape::Linear, card.kind, card.propId, card.field, card.minValue,
                                 card.maxValue);
                relation.constant = card.constant0;
                relation.terms = card.terms;
                relation.fromProperty = card.fromProperty;
                finish(relation);
            });
        sortSinceFirst();
        forEachCard<DesignPropRelation2Table>(
            m_model,
            [&](const DesignPropRelation2Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignPropRelation2Table>(), row, key, card.id, DesignTargetKind::Property,
                                 DesignRelationShape::Equation, card.kind, card.propId, card.field, card.minValue,
                                 card.maxValue);
                relation.equationId = card.equationId;
                relation.variables = card.designVarIds;
                relation.labels = card.labels;
                finish(relation);
            });
        sortSinceFirst();
        forEachCard<DesignMatRelation1Table>(
            m_model,
            [&](const DesignMatRelation1Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignMatRelation1Table>(), row, key, card.id, DesignTargetKind::Material,
                                 DesignRelationShape::Linear, card.kind, card.matId, DesignField{card.name},
                                 card.minValue, card.maxValue);
                relation.constant = card.constant0;
                relation.terms = card.terms;
                relation.fromProperty = card.fromProperty;
                finish(relation);
            });
        sortSinceFirst();
        forEachCard<DesignMatRelation2Table>(
            m_model,
            [&](const DesignMatRelation2Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignMatRelation2Table>(), row, key, card.id, DesignTargetKind::Material,
                                 DesignRelationShape::Equation, card.kind, card.matId, DesignField{card.name},
                                 card.minValue, card.maxValue);
                relation.equationId = card.equationId;
                relation.variables = card.designVarIds;
                relation.labels = card.labels;
                finish(relation);
            });
        sortSinceFirst();
        forEachCard<DesignElemRelation1Table>(
            m_model,
            [&](const DesignElemRelation1Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignElemRelation1Table>(), row, key, card.id, DesignTargetKind::Element,
                                 DesignRelationShape::Linear, card.kind, card.elemId, DesignField{card.name},
                                 card.minValue, card.maxValue);
                relation.constant = card.constant0;
                relation.terms = card.terms;
                relation.fromProperty = card.fromProperty;
                finish(relation);
            });
        sortSinceFirst();
        forEachCard<DesignElemRelation2Table>(
            m_model,
            [&](const DesignElemRelation2Card& card, std::uint32_t row, base::CardKey key)
            {
                relation = start(typeIndexOf<DesignElemRelation2Table>(), row, key, card.id, DesignTargetKind::Element,
                                 DesignRelationShape::Equation, card.kind, card.elemId, DesignField{card.name},
                                 card.minValue, card.maxValue);
                relation.equationId = card.equationId;
                relation.variables = card.designVarIds;
                relation.labels = card.labels;
                finish(relation);
            });
        sortSinceFirst();
    }

    // The DVGRID cards (applied by analysis::DesignApplication, which has the coordinate systems), and the cards that
    // carry a design variable into the model without a field to write: the model cannot apply them.
    void readCardsThatAreNotApplied()
    {
        // the keyword of a card type in the messages
        const auto label = [&](std::string_view cardType) { return std::string(cardType); };
        forEachCard<DesignNodeMoveTable>(
            m_model,
            [&](const DesignNodeMoveCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_shapeTerms.push_back(
                    DesignShapeTerm{card.designVarId, card.nodeId, card.frameId, card.coefficient, card.direction});
            });
        forEachCard<DesignShapeBasisTable>(
            m_model,
            [&](const DesignShapeBasisCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back(
                    {"DesignShapeBasis", card.designVarId,
                     "a " + label("DesignShapeBasis") +
                         " combines the columns of a displacement matrix that is not in the deck: shape changes are "
                         "not applied"});
            });
        forEachCard<DesignBoundaryShapeTable>(
            m_model,
            [&](const DesignBoundaryShapeCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back(
                    {"DesignBoundaryShape", card.designVarId,
                     "a " + label("DesignBoundaryShape") +
                         " names the shape basis vectors of a database: shape changes are not applied"});
            });
        forEachCard<DesignTopologyRegionTable>(
            m_model,
            [&](const DesignTopologyRegionCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back(
                    {"DesignTopologyRegion", card.id,
                     "a " + label("DesignTopologyRegion") + " is a topology region: it is not applied"});
            });
        forEachCard<DesignBeadRegionTable>(
            m_model,
            [&](const DesignBeadRegionCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back({"DesignBeadRegion", card.id,
                                                 "a " + label("DesignBeadRegion") +
                                                     " is a topography region: its bead design variables are "
                                                     "generated by the solver and are not applied"});
            });
        forEachCard<DesignTopometryRegionTable>(
            m_model,
            [&](const DesignTopometryRegionCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back(
                    {"DesignTopometryRegion", card.id,
                     "a " + label("DesignTopometryRegion") +
                         " is a topometry region (an element by element design): it is not applied"});
            });
        forEachCard<DesignLoadRelation1Table>(
            m_model,
            [&](const DesignLoadRelation1Card& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back({"DesignLoadRelation1", card.id,
                                                 "a " + label("DesignLoadRelation1") + " designs a value of the load " +
                                                     label(card.kind) + " of the set " + std::to_string(card.setId) +
                                                     ": loads are not applied"});
            });
        forEachCard<DesignControlSurfaceTable>(
            m_model,
            [&](const DesignControlSurfaceCard& card, std::uint32_t, base::CardKey)
            {
                m_design.m_notApplied.push_back({"DesignControlSurface", card.id,
                                                 "a " + label("DesignControlSurface") + " sets the control surface " +
                                                     card.controlSurfaceLabel + " of the trim case " +
                                                     std::to_string(card.trimId) +
                                                     ": the settings of a trim are not applied"});
            });
    }

    // The design variables something uses: a relation, a card that moves grid points or shapes, a DRESP2 or DRESP3, and
    // the independent variables of a DLINK whose dependent variable is used.
    void collectUsedVariables()
    {
        std::set<base::EntityId>& used = m_design.m_used;
        for (const DesignRelation& relation : m_design.m_relations)
        {
            for (const DesignTerm& term : relation.terms)
                used.insert(term.designVarId);
            used.insert(relation.variables.begin(), relation.variables.end());
        }
        forEachCard<DesignNodeMoveTable>(m_model, [&](const DesignNodeMoveCard& card, std::uint32_t, base::CardKey)
                                         { used.insert(card.designVarId); });
        forEachCard<DesignShapeBasisTable>(m_model, [&](const DesignShapeBasisCard& card, std::uint32_t, base::CardKey)
                                           { used.insert(card.designVarId); });
        forEachCard<DesignBoundaryShapeTable>(m_model, [&](const DesignBoundaryShapeCard& card, std::uint32_t,
                                                           base::CardKey) { used.insert(card.designVarId); });
        // A designed load (DVLREL1) and a designed control surface setting (DVPSURF) use their variables too, though
        // the design model does not apply them. The TOMVAR and BEADVAR cards name no DESVAR: the solver makes the
        // variables of their regions.
        forEachCard<DesignLoadRelation1Table>(m_model,
                                              [&](const DesignLoadRelation1Card& card, std::uint32_t, base::CardKey)
                                              {
                                                  for (const DesignTerm& term : card.terms)
                                                      used.insert(term.designVarId);
                                              });
        forEachCard<DesignControlSurfaceTable>(m_model, [&](const DesignControlSurfaceCard& card, std::uint32_t,
                                                            base::CardKey) { used.insert(card.designVarId); });
        const auto addGroups = [&](const std::vector<DesignGroup>& groups)
        {
            if (const DesignGroup* group = findDesignGroup(groups, "DesignVar"))
                used.insert(group->ids.begin(), group->ids.end());
        };
        forEachCard<DesignResp2Table>(
            m_model, [&](const DesignResp2Card& card, std::uint32_t, base::CardKey) { addGroups(card.groups); });
        forEachCard<DesignResp3Table>(
            m_model, [&](const DesignResp3Card& card, std::uint32_t, base::CardKey) { addGroups(card.groups); });
        for (const DesignLink& link : m_design.m_links)
        {
            if (!used.contains(link.dependent))
                continue;
            for (const DesignTerm& term : link.terms)
                used.insert(term.designVarId);
        }
    }

    DesignRelation start(std::uint16_t type, std::uint32_t row, base::CardKey key, base::EntityId id,
                         DesignTargetKind kind, DesignRelationShape shape, const std::string& targetCard,
                         base::EntityId targetId, const DesignField& field, const std::optional<double>& lowerLimit,
                         double upperLimit) const
    {
        DesignRelation relation;
        // the name the messages give the relation: the keyword of its card type ("DVPREL1")
        relation.card = std::string(CardRegistry::instance().entry(type).info.name);
        relation.cardType = type;
        relation.row = row;
        relation.key = key;
        relation.id = id;
        relation.targetKind = kind;
        relation.shape = shape;
        relation.targetCard = targetCard;
        relation.targetLabel = std::string(targetCard);
        relation.targetId = targetId;
        relation.field = field;
        relation.lowerLimit = lowerLimit;
        relation.upperLimit = upperLimit;
        return relation;
    }

    // Works out what the relation drives, and keeps it.
    void finish(DesignRelation& relation)
    {
        resolve(relation);
        m_design.m_relations.push_back(relation);
    }

    // The card types that have design functions, as the deck writes them.
    std::vector<std::string> designableKeywords() const
    {
        std::vector<std::string> keywords;
        for (const std::string& name : designableCardNames())
            keywords.push_back(std::string(name));
        return keywords;
    }

    void resolve(DesignRelation& relation) const
    {
        const std::string name = relation.card + " " + std::to_string(relation.id);
        const auto refuse = [&](const std::string& reason) { relation.problem = name + ": " + reason; };

        if (relation.targetKind == DesignTargetKind::Property && relation.targetCard == GLOBAL_PLY_KIND)
            return refuse("a ply designed by its global ply ID (" + std::string("kind") + " " +
                          std::string(GLOBAL_PLY_KIND) + ") is not applied: name the property of the laminate");
        relation.fields = findDesignableCard(relation.targetCard);
        if (relation.fields == nullptr)
            return refuse(NO_FIELDS_OF_THIS_CARD + relation.targetLabel + " (it can design " +
                          joined(designableKeywords()) + ")");
        const std::optional<CardLocation> location = m_model.locate(spaceOf(relation.targetKind), relation.targetId);
        if (!location)
            return refuse(std::string("the ") + nameOf(relation.targetKind) + " " + std::to_string(relation.targetId) +
                          " does not exist");
        // The TYPE of a relation is a neutral card name: the card found is named the same way.
        const std::string actual(CardRegistry::instance().entry(location->table).info.name);
        if (actual != relation.targetCard)
            return refuse(std::string("the ") + nameOf(relation.targetKind) + " " + std::to_string(relation.targetId) +
                          " is a " + std::string(actual) + ", not a " + relation.targetLabel);
        relation.target = location;
        DesignFieldLookup lookup = relation.fields->find(m_model, location->row, relation.field);
        if (!lookup.facts)
        {
            relation.badField = true;
            return refuse(lookup.problem);
        }
        relation.facts = std::move(*lookup.facts);

        if (relation.shape == DesignRelationShape::Linear)
        {
            for (const DesignTerm& term : relation.terms)
            {
                if (!m_design.findVariable(term.designVarId))
                    return refuse("the design variable " + std::to_string(term.designVarId) + " does not exist");
            }
        }
        else if (!resolveEquation(relation, refuse))
        {
            return;
        }
        computeLimits(relation);
        relation.applicable = true;
    }

    // An equation needs its DEQATN, the design variables and the constants of its arguments.
    template <typename Refuse>
    bool resolveEquation(DesignRelation& relation, const Refuse& refuse) const
    {
        const auto equation = m_design.m_equations.find(relation.equationId);
        if (equation == m_design.m_equations.end())
        {
            refuse(std::string("DesignEquation") + " " + std::to_string(relation.equationId) +
                   " does not exist or does not parse");
            return false;
        }
        const std::size_t given = relation.variables.size() + relation.labels.size();
        const std::size_t needed = equation->second.parameters().size();
        if (given < needed)
        {
            refuse(std::string("DesignEquation") + " " + std::to_string(relation.equationId) + " has " +
                   std::to_string(needed) + " arguments and the relation gives it " + std::to_string(given) +
                   " values");
            return false;
        }
        for (const base::EntityId variable : relation.variables)
        {
            if (!m_design.findVariable(variable))
            {
                refuse("the design variable " + std::to_string(variable) + " does not exist");
                return false;
            }
        }
        for (const std::string& label : relation.labels)
        {
            const std::string key = std::string(label);
            if (m_design.m_values.contains(key) || m_design.m_fieldConstants.contains(key))
                continue;
            const auto unavailable = m_design.m_unavailable.find(key);
            refuse(unavailable != m_design.m_unavailable.end()
                       ? unavailable->second
                       : "the constant " + label + " is not defined by a " + std::string("DesignConstants") + " or a " +
                             std::string("DesignFieldConstants"));
            return false;
        }
        return true;
    }

    // PMIN blank is 1.0E-15 for a field that can only be positive and -1.0E35 for the others; for a DVPREL2 it
    // is 1.0E-15 for every field except a stress recovery location (see DesignLowerLimit). The defaults are not applied
    // when a linear relation is a function of one variable with a zero constant (a DVPREL1, DVMREL1 or
    // DVCREL1): the bounds of the DESVAR keep the field in range. A PMIN or a PMAX the
    // card gives is always applied.
    static void computeLimits(DesignRelation& relation)
    {
        const bool oneVariable =
            relation.shape == DesignRelationShape::Linear && relation.terms.size() == 1 && relation.constant == 0.0;
        relation.defaultsApplied = !oneVariable;
        const DesignLowerLimit kind = relation.facts.lowerLimit;
        const bool positiveByDefault = relation.cardType == typeIndexOf<DesignPropRelation2Table>()
                                           ? kind != DesignLowerLimit::StressRecovery
                                           : kind == DesignLowerLimit::Positive;
        const double fallback =
            relation.defaultsApplied && positiveByDefault ? LOWEST_OF_POSITIVE_FIELD : NO_LOWER_LIMIT;
        relation.lowest = relation.lowerLimit.value_or(fallback);
        relation.highest = relation.upperLimit;
    }

    void problem(std::string text) { m_design.m_problems.push_back(std::move(text)); }

    const Model& m_model;
    DesignModel& m_design;
};

DesignModel DesignModel::read(const Model& model)
{
    DesignModel design;
    DesignReader(model, design).run();
    return design;
}

bool DesignRelation::usesVariable(base::EntityId variable) const
{
    for (const DesignTerm& term : terms)
    {
        if (term.designVarId == variable)
            return true;
    }
    return std::find(variables.begin(), variables.end(), variable) != variables.end();
}

std::optional<std::size_t> DesignModel::findVariable(base::EntityId id) const
{
    const auto found =
        std::lower_bound(m_variables.begin(), m_variables.end(), id,
                         [](const DesignVariable& variable, base::EntityId wanted) { return variable.id < wanted; });
    if (found == m_variables.end() || found->id != id)
        return std::nullopt;
    return static_cast<std::size_t>(found - m_variables.begin());
}

std::optional<std::size_t> DesignModel::findVariable(const std::string& label) const
{
    const std::string wanted = std::string(label);
    std::optional<std::size_t> result;
    for (std::size_t index = 0; index < m_variables.size(); ++index)
    {
        if (std::string(m_variables[index].label) != wanted)
            continue;
        if (result)
            throw DesignError("the label " + label + " names the design variables " +
                              std::to_string(m_variables[*result].id) + " and " +
                              std::to_string(m_variables[index].id) + ": give the ID");
        result = index;
    }
    return result;
}

std::optional<base::EntityId> DesignModel::variableDriving(DesignTargetKind kind, base::EntityId id) const
{
    for (const DesignRelation& relation : m_relations)
    {
        if (!relation.applicable || relation.targetKind != kind || relation.targetId != id ||
            relation.facts.massEffect == DesignMassEffect::None)
            continue;
        if (relation.shape == DesignRelationShape::Linear)
            return relation.terms.front().designVarId;
        if (!relation.variables.empty())
            return relation.variables.front();
    }
    return std::nullopt;
}

DesignPoint DesignModel::initialPoint() const
{
    DesignPoint point;
    point.values.reserve(m_variables.size());
    for (const DesignVariable& variable : m_variables)
        point.values.push_back(variable.initial);
    return point;
}

const EquationSet* DesignModel::equation(base::EntityId id) const
{
    const auto found = m_equations.find(id);
    return found == m_equations.end() ? nullptr : &found->second;
}

std::optional<double> DesignModel::constant(const Model& model, const std::string& label, std::string* reason) const
{
    const std::string key = std::string(label);
    const auto fail = [&](std::string text) -> std::optional<double>
    {
        if (reason != nullptr)
            *reason = std::move(text);
        return std::nullopt;
    };
    if (const auto value = m_values.find(key); value != m_values.end())
        return value->second;
    if (const auto table = m_unavailable.find(key); table != m_unavailable.end())
        return fail(table->second);
    const std::string fieldConstantsCard = std::string("DesignFieldConstants");
    const auto source = m_fieldConstants.find(key);
    if (source == m_fieldConstants.end())
        return fail("the constant " + label + " is not defined by a " + std::string("DesignConstants") + " or a " +
                    fieldConstantsCard);

    // A DTABLE2 entry: the field of a card of the model, as the model holds it before the design changes it (format
    // reference).
    const FieldConstant& entry = source->second;
    const DesignableCard* fields = findDesignableCard(entry.cardName);
    if (fields == nullptr)
        return fail(NO_FIELDS_OF_THIS_CARD + std::string(entry.cardName) + ", which the " + fieldConstantsCard +
                    " constant " + label + " reads");
    const std::optional<ReferenceTarget> target = targetOfDesignedCard(entry.cardName);
    DesignTargetKind kind = DesignTargetKind::Element;
    if (target && *target == ReferenceTarget::Property)
        kind = DesignTargetKind::Property;
    else if (target && *target == ReferenceTarget::Material)
        kind = DesignTargetKind::Material;
    const std::optional<CardLocation> location = model.locate(spaceOf(kind), entry.id);
    // The card of a DTABLE2 entry is a neutral card name.
    if (!location || CardRegistry::instance().entry(location->table).info.name != entry.cardName)
        return fail("the " + fieldConstantsCard + " constant " + label + " reads " + std::string(entry.cardName) + " " +
                    std::to_string(entry.id) + ", which the model does not have");
    const DesignFieldLookup lookup = fields->find(model, location->row, DesignField{entry.field});
    if (!lookup.facts)
        return fail("the " + fieldConstantsCard + " constant " + label + ": " + lookup.problem);
    return fields->read(model, location->row, *lookup.facts);
}

} // namespace oor::model
