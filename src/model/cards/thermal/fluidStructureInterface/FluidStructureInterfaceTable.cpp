#include "oor/model/cards/thermal/fluidStructureInterface/FluidStructureInterfaceTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

constexpr double NORMAL_OF_DIFF = 1.0;
constexpr double NORMAL_OF_IDENT = 0.001;

} // namespace

FluidStructureInterfaceTable::FluidStructureInterfaceTable()
    : m_interfaceKind("DifferenceForm"),
      m_interfaceInput("None"),
      m_normalTolerance(NORMAL_OF_DIFF),
      m_methodName("SurfaceCoupling"),
      m_skinTolerance(0.5),
      m_skinToleranceIncrement(0.75),
      m_interfaceTolerance(0.5),
      m_wholeStructureSet("No"),
      m_searchUnit("Relative")
{
}

std::optional<FluidStructureInterfaceCard> FluidStructureInterfaceFormat::read(FieldReader& reader)
{
    FluidStructureInterfaceCard card;
    card.interfaceKind = readKeyword(reader, 1, "interfaceKind", {"DifferenceForm", "IdenticalMesh"}, "DifferenceForm");
    card.interfaceInput = readKeyword(reader, 2, "interfaceInput", {"None", "NodesList", "ElemsList", "All"}, "None");
    card.fluidSetId = reader.integer(3, "fluidSetId", 0);
    card.structureSetId = reader.integer(4, "structureSetId", 0);
    // format reference: the fluid normal tolerance is 1.0, or 0.001 for an IDENT interface.
    card.normalTolerance =
        reader.real(5, "normalTolerance", card.interfaceKind == "IdenticalMesh" ? NORMAL_OF_IDENT : NORMAL_OF_DIFF);
    card.methodName = readKeyword(reader, 6, "methodName",
                                  {"SurfaceCoupling", "ScaledSurfaceCoupling", "ClosedPressure"}, "SurfaceCoupling");
    if (card.methodName == "ClosedPressure")
    {
        requireUnusedBlank(reader, {}, 6);
    }
    else
    {
        card.skinTolerance = reader.real(7, "skinTolerance", 0.5);
        card.skinToleranceIncrement = reader.real(8, "skinToleranceIncrement", 0.75);
        card.interfaceTolerance = reader.real(9, "interfaceTolerance", 0.5);
        card.wholeStructureSet = readKeyword(reader, 10, "wholeStructureSet", {"No", "Yes"}, "No");
        card.searchUnit = readKeyword(reader, 11, "searchUnit", {"Relative", "Absolute"}, "Relative");
        requireUnusedBlank(reader, {}, 11);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidStructureInterfaceFormat::check(const FluidStructureInterfaceCard& card)
{
    if (const auto problem = checkKeyword(card.interfaceKind, "interfaceKind", {"DifferenceForm", "IdenticalMesh"}))
        return problem;
    if (const auto problem =
            checkKeyword(card.interfaceInput, "interfaceInput", {"None", "NodesList", "ElemsList", "All"}))
        return problem;
    if (const auto problem =
            checkKeyword(card.methodName, "methodName", {"SurfaceCoupling", "ScaledSurfaceCoupling", "ClosedPressure"}))
        return problem;
    if (card.fluidSetId < -1 || card.structureSetId < -1)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("structureSetId") +
                               " are -1 or a set ID (or blank)",
                           "fluidSetId"};
    if (!(card.normalTolerance > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "normalTolerance"};
    if (card.methodName == "ClosedPressure")
    {
        // ClosedPressure has no skin growth, tolerance or search unit and accepts All or None.
        if (card.interfaceInput == "NodesList" || card.interfaceInput == "ElemsList")
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is All or None for " +
                                   CardProblem::fieldNamed("methodName") + " ClosedPressure",
                               "interfaceInput"};
        if (card.skinTolerance != 0.5 || card.skinToleranceIncrement != 0.75 || card.interfaceTolerance != 0.5 ||
            card.wholeStructureSet != "No" || card.searchUnit != "Relative")
            return CardProblem{
                "card.bad-value",
                CardProblem::fieldNamed("skinTolerance") + ", " + CardProblem::fieldNamed("skinToleranceIncrement") +
                    ", " + CardProblem::fieldNamed("interfaceTolerance") + ", " +
                    CardProblem::fieldNamed("wholeStructureSet") + " and " + CardProblem::fieldNamed("searchUnit") +
                    " go with " + std::string(CardProblem::FIELD) + " SurfaceCoupling or ScaledSurfaceCoupling",
                "methodName"};
        return std::nullopt;
    }
    if (!(card.skinTolerance > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "skinTolerance"};
    if (!(card.skinToleranceIncrement > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "skinToleranceIncrement"};
    if (!(card.interfaceTolerance > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "interfaceTolerance"};
    if (const auto problem = checkKeyword(card.wholeStructureSet, "wholeStructureSet", {"No", "Yes"}))
        return problem;
    return checkKeyword(card.searchUnit, "searchUnit", {"Relative", "Absolute"});
}

void FluidStructureInterfaceFormat::write(const FluidStructureInterfaceCard& card, FieldWriter& writer)
{
    writer.textUnlessDefault(1, card.interfaceKind, "DifferenceForm");
    writer.textUnlessDefault(2, card.interfaceInput, "None");
    writer.integerUnlessDefault(3, card.fluidSetId, 0);
    writer.integerUnlessDefault(4, card.structureSetId, 0);
    // NORMAL defaults to 1.0 or 0.001 by INTER: always written.
    writer.real(5, card.normalTolerance);
    writer.textUnlessDefault(6, card.methodName, "SurfaceCoupling");
    if (card.methodName == "ClosedPressure")
        return;
    writer.realUnlessDefault(7, card.skinTolerance, 0.5);
    writer.realUnlessDefault(8, card.skinToleranceIncrement, 0.75);
    writer.realUnlessDefault(9, card.interfaceTolerance, 0.5);
    writer.textUnlessDefault(10, card.wholeStructureSet, "No");
    writer.textUnlessDefault(11, card.searchUnit, "Relative");
}

std::uint32_t FluidStructureInterfaceTable::append(const FluidStructureInterfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_interfaceKind.push(card.interfaceKind);
    m_interfaceInput.push(card.interfaceInput);
    m_fluidSetId.push(card.fluidSetId);
    m_structureSetId.push(card.structureSetId);
    m_normalTolerance.push(card.normalTolerance);
    m_methodName.push(card.methodName);
    m_skinTolerance.push(card.skinTolerance);
    m_skinToleranceIncrement.push(card.skinToleranceIncrement);
    m_interfaceTolerance.push(card.interfaceTolerance);
    m_wholeStructureSet.push(card.wholeStructureSet);
    m_searchUnit.push(card.searchUnit);
    return row;
}

FluidStructureInterfaceCard FluidStructureInterfaceTable::card(std::uint32_t row) const
{
    FluidStructureInterfaceCard card;
    card.interfaceKind = m_interfaceKind.get(row);
    card.interfaceInput = m_interfaceInput.get(row);
    card.fluidSetId = m_fluidSetId.get(row);
    card.structureSetId = m_structureSetId.get(row);
    card.normalTolerance = m_normalTolerance.get(row);
    card.methodName = m_methodName.get(row);
    card.skinTolerance = m_skinTolerance.get(row);
    card.skinToleranceIncrement = m_skinToleranceIncrement.get(row);
    card.interfaceTolerance = m_interfaceTolerance.get(row);
    card.wholeStructureSet = m_wholeStructureSet.get(row);
    card.searchUnit = m_searchUnit.get(row);
    return card;
}

void FluidStructureInterfaceTable::store(std::uint32_t row, const FluidStructureInterfaceCard& card)
{
    m_interfaceKind.set(row, card.interfaceKind);
    m_interfaceInput.set(row, card.interfaceInput);
    m_fluidSetId.set(row, card.fluidSetId);
    m_structureSetId.set(row, card.structureSetId);
    m_normalTolerance.set(row, card.normalTolerance);
    m_methodName.set(row, card.methodName);
    m_skinTolerance.set(row, card.skinTolerance);
    m_skinToleranceIncrement.set(row, card.skinToleranceIncrement);
    m_interfaceTolerance.set(row, card.interfaceTolerance);
    m_wholeStructureSet.set(row, card.wholeStructureSet);
    m_searchUnit.set(row, card.searchUnit);
}

void FluidStructureInterfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_interfaceKind.keepRows(rows);
    m_interfaceInput.keepRows(rows);
    m_fluidSetId.keepRows(rows);
    m_structureSetId.keepRows(rows);
    m_normalTolerance.keepRows(rows);
    m_methodName.keepRows(rows);
    m_skinTolerance.keepRows(rows);
    m_skinToleranceIncrement.keepRows(rows);
    m_interfaceTolerance.keepRows(rows);
    m_wholeStructureSet.keepRows(rows);
    m_searchUnit.keepRows(rows);
}

void FluidStructureInterfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId fluidSetId = m_fluidSetId.get(row); fluidSetId > 0)
        m_fluidSetId.set(row, visitor.visit("fluidSetId", ReferenceTarget::Set, fluidSetId));
    if (const base::EntityId structureSetId = m_structureSetId.get(row); structureSetId > 0)
        m_structureSetId.set(row, visitor.visit("structureSetId", ReferenceTarget::Set, structureSetId));
}

void FluidStructureInterfaceTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-fluid-structure-interface",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
