#include "oor/model/cards/nonlinear_contact/crackClosureCrack/CrackClosureCrackTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 25;

// The crack front is a list in the order of the crack, so a range may go down as well as up, and BY gives the step.
constexpr ContactRunRules GRID_RULES{true, true};

// The fields the layout leaves blank in lines 2 and 3 (the first line has two, which are not looked at: see the card).
constexpr std::initializer_list<int> BLANK_FIELDS = {11, 12, 13, 14, 19, 20, 21, 22};

// The text of a field starts with a minus sign: a negative integer.
bool looksNegative(std::string_view text) noexcept
{
    return !text.empty() && text.front() == '-';
}

// The problem with an integer option (ITYPE, IGROW, INCM, METHOD), if there is one: it is not negative. The format
// gives a meaning to a few numbers only, but it is not consistent about them (an INCM of 2 also appears), so they are
// not taken as the complete set of values.
std::optional<CardProblem> checkOptionNumber(std::int32_t value, const char* field)
{
    if (value < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", field};
    return std::nullopt;
}

} // namespace

std::optional<CrackClosureCrackCard> CrackClosureCrackFormat::read(FieldReader& reader)
{
    CrackClosureCrackCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.crackId = reader.integer(2, "crackId", 1);
    card.propagationKind = reader.integer(3, "propagationKind", CRACK_CLOSURE_DEFAULT_PROPAGATION);
    card.growthMethod = reader.integer(4, "growthMethod", CRACK_CLOSURE_DEFAULT_GROWTH);
    card.incrementMethod = reader.integer(5, "incrementMethod", CRACK_CLOSURE_DEFAULT_INCREMENT);
    card.growthCriterion = reader.integer(6, "growthCriterion", CRACK_CLOSURE_DEFAULT_METHOD);
    card.growthIncrement = reader.real(9, "growthIncrement", 0.0);
    card.growthResistance = reader.real(10, "growthResistance", 0.0);
    card.growthResistanceII = reader.real(15, "growthResistanceII", 0.0);
    card.growthResistanceIII = reader.real(16, "growthResistanceIII", 0.0);
    card.growthIncrementTableId = reader.integer(17, "growthIncrementTableId", 0);
    card.growthResistanceTableId = reader.integer(18, "growthResistanceTableId", 0);
    card.growthResistanceIITableId = reader.integer(23, "growthResistanceIITableId", 0);
    card.growthResistanceIIITableId = reader.integer(24, "growthResistanceIIITableId", 0);
    requireUnusedBlank(reader, BLANK_FIELDS, std::numeric_limits<int>::max());

    const int last = reader.lastUsedField();
    if (last < FIRST_GRID_FIELD)
    {
        reader.error("card.missing-field", "the crack front needs at least one grid point, " + std::string("node1"),
                     "node1");
    }
    else if (looksNegative(reader.text(FIRST_GRID_FIELD)))
    {
        // A negative G1 is the ID of a SET3, and then it is the only thing in the list.
        card.nodeSetId = -reader.requiredInteger(FIRST_GRID_FIELD, "node1");
        for (int field = FIRST_GRID_FIELD + 1; field <= last; ++field)
        {
            if (!reader.isBlank(field))
            {
                reader.error("card.bad-value",
                             std::string("node2") + ", ... must be blank when " + std::string("node1") +
                                 " is the negative ID of a " + std::string("TypedListSet3"),
                             "node2");
                break;
            }
        }
    }
    else
    {
        card.nodes = readContactRuns(reader, FIRST_GRID_FIELD, last, "nodes", GRID_RULES);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrackClosureCrackFormat::check(const CrackClosureCrackCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkRequiredId(card.crackId, "crackId"))
        return problem;
    if (const auto problem = checkOptionNumber(card.propagationKind, "propagationKind"))
        return problem;
    if (const auto problem = checkOptionNumber(card.growthMethod, "growthMethod"))
        return problem;
    if (const auto problem = checkOptionNumber(card.incrementMethod, "incrementMethod"))
        return problem;
    if (const auto problem = checkOptionNumber(card.growthCriterion, "growthCriterion"))
        return problem;
    if (const auto problem = checkReal(card.growthIncrement, "growthIncrement", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.growthResistance, "growthResistance", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.growthResistanceII, "growthResistanceII", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.growthResistanceIII, "growthResistanceIII", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.growthIncrementTableId, "growthIncrementTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.growthResistanceTableId, "growthResistanceTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.growthResistanceIITableId, "growthResistanceIITableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.growthResistanceIIITableId, "growthResistanceIIITableId"))
        return problem;
    if (card.nodeSetId < 0)
        return CardProblem{"card.bad-id",
                           "the " + std::string("TypedListSet3") + " of the crack front must be a positive ID (" +
                               std::string(CardProblem::FIELD) + " is its negative)",
                           "node1"};
    if (card.nodeSetId > 0 && !card.nodes.empty())
        return CardProblem{
            "card.bad-value",
            "the crack front is a " + std::string("TypedListSet3") + " or a list of grid points, not both", "node2"};
    if (card.nodeSetId == 0 && card.nodes.empty())
        return CardProblem{"card.missing-field",
                           "the crack front needs at least one grid point, " + std::string(CardProblem::FIELD),
                           "node1"};
    return checkContactRuns(card.nodes, "node1", GRID_RULES);
}

void CrackClosureCrackFormat::write(const CrackClosureCrackCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.integerUnlessDefault(2, card.crackId, 1);
    writer.integerUnlessDefault(3, card.propagationKind, CRACK_CLOSURE_DEFAULT_PROPAGATION);
    writer.integerUnlessDefault(4, card.growthMethod, CRACK_CLOSURE_DEFAULT_GROWTH);
    writer.integerUnlessDefault(5, card.incrementMethod, CRACK_CLOSURE_DEFAULT_INCREMENT);
    writer.integerUnlessDefault(6, card.growthCriterion, CRACK_CLOSURE_DEFAULT_METHOD);
    // Lines 2 and 3 are never blank: the first field of each is written even when it is the default.
    writer.real(9, card.growthIncrement);
    writer.realUnlessDefault(10, card.growthResistance, 0.0);
    writer.realUnlessDefault(15, card.growthResistanceII, 0.0);
    writer.realUnlessDefault(16, card.growthResistanceIII, 0.0);
    writer.integer(17, card.growthIncrementTableId);
    writer.integerUnlessDefault(18, card.growthResistanceTableId, 0);
    writer.integerUnlessDefault(23, card.growthResistanceIITableId, 0);
    writer.integerUnlessDefault(24, card.growthResistanceIIITableId, 0);
    if (card.nodeSetId > 0)
        writer.integer(FIRST_GRID_FIELD, -card.nodeSetId);
    else
        writeContactRuns(writer, FIRST_GRID_FIELD, card.nodes);
}

std::uint32_t CrackClosureCrackTable::append(const CrackClosureCrackCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void CrackClosureCrackTable::store(std::uint32_t row, const CrackClosureCrackCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void CrackClosureCrackTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void CrackClosureCrackTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void CrackClosureCrackTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CrackClosureCrackCard& card = m_cards.edit(row);
    visitId(visitor, "growthIncrementTableId", ReferenceTarget::MaterialTable, card.growthIncrementTableId);
    visitId(visitor, "growthResistanceTableId", ReferenceTarget::MaterialTable, card.growthResistanceTableId);
    visitId(visitor, "growthResistanceIITableId", ReferenceTarget::MaterialTable, card.growthResistanceIITableId);
    visitId(visitor, "growthResistanceIIITableId", ReferenceTarget::MaterialTable, card.growthResistanceIIITableId);
    if (card.nodeSetId > 0)
        visitId(visitor, "node1", ReferenceTarget::Set, card.nodeSetId);
    else
        visitContactRuns(card.nodes, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
