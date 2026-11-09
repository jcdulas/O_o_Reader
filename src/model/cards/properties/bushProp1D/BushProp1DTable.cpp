#include "oor/model/cards/properties/bushProp1D/BushProp1DTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 9;
constexpr int FIELDS_PER_LINE = 8;

// The fields from `first` to `last` must be blank (padding of a continuation line).
void requireBlank(FieldReader& reader, int first, int last, const char* name)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "this field of a " + std::string(reader.cardName()) + " must be blank",
                         name);
            return;
        }
    }
}

// TYPE: TABLE (blank too) or EQUAT.
BushProp1DDefinition readDefinition(FieldReader& reader, int field)
{
    const std::string_view type = reader.text(field);
    if (type.empty() || (type == "Table"))
        return BushProp1DDefinition::Table;
    if ((type == "Equation"))
        return BushProp1DDefinition::Equation;
    reader.error("card.bad-value", std::string("kind") + " must be TABLE or EQUAT, found \"" + std::string(type) + "\"",
                 "kind");
    return BushProp1DDefinition::Table;
}

const char* definitionName(BushProp1DDefinition type)
{
    return type == BushProp1DDefinition::Equation ? "Equation" : "Table";
}

// The SHOCKA continuation, its first line starting at `field` (the second line follows it).
BushProp1DShock readShock(FieldReader& reader, int field)
{
    BushProp1DShock shock;
    shock.present = true;
    shock.type = readDefinition(reader, field + 1);
    shock.dampingTension = reader.requiredReal(field + 2, "dampingTension");
    shock.dampingCompression = reader.real(field + 3, "dampingCompression", shock.dampingTension);
    shock.velocityExponentTension = reader.real(field + 4, "velocityExponentTension", 1.0);
    shock.velocityExponentCompression =
        reader.real(field + 5, "velocityExponentCompression", shock.velocityExponentTension);
    shock.scaleTableId = reader.integer(field + 6, "scaleTableId", 0);
    requireBlank(reader, field + 7, field + 9, "scaleTableId");
    shock.scaleTensionEquationId = reader.integer(field + 10, "scaleTensionEquationId", 0);
    shock.scaleCompressionEquationId =
        reader.integer(field + 11, "scaleCompressionEquationId", shock.scaleTensionEquationId);
    shock.scaleTensionDerivativeEquationId = reader.integer(field + 12, "scaleTensionDerivativeEquationId", 0);
    shock.scaleCompressionDerivativeEquationId =
        reader.integer(field + 13, "scaleCompressionDerivativeEquationId", shock.scaleTensionDerivativeEquationId);
    requireBlank(reader, field + 14, field + 15, "scaleCompressionDerivativeEquationId");
    return shock;
}

// The SPRING or DAMPER continuation, its line starting at `field`.
BushProp1DCurve readCurve(FieldReader& reader, int field, const char* derivative, const char* compressionDerivative)
{
    BushProp1DCurve curve;
    curve.present = true;
    curve.type = readDefinition(reader, field + 1);
    curve.tensionCurveId = reader.requiredInteger(field + 2, "tensionCurveId");
    curve.compressionCurveId = reader.integer(field + 3, "compressionCurveId", curve.tensionCurveId);
    curve.tensionDerivativeId = reader.integer(field + 4, derivative, 0);
    curve.compressionDerivativeId = reader.integer(field + 5, compressionDerivative, curve.tensionDerivativeId);
    requireBlank(reader, field + 6, field + 7, compressionDerivative);
    return curve;
}

// The GENER continuation, its line starting at `field`.
BushProp1DGeneral readGeneral(FieldReader& reader, int field)
{
    BushProp1DGeneral general;
    general.present = true;
    if (const std::string_view type = reader.text(field + 1); !type.empty() && !(type == "Equation"))
        reader.error(
            "card.bad-value",
            "the " + std::string("kind") + " of a GENER is EQUAT or blank, found \"" + std::string(type) + "\"",
            "kind");
    general.tensionCurveId = reader.requiredInteger(field + 2, "tensionCurveId");
    general.compressionCurveId = reader.integer(field + 3, "compressionCurveId", general.tensionCurveId);
    general.tensionDisplacementDerivativeId = reader.requiredInteger(field + 4, "tensionDisplacementDerivativeId");
    general.compressionDisplacementDerivativeId =
        reader.integer(field + 5, "compressionDisplacementDerivativeId", general.tensionDisplacementDerivativeId);
    general.tensionVelocityDerivativeId = reader.requiredInteger(field + 6, "tensionVelocityDerivativeId");
    general.compressionVelocityDerivativeId =
        reader.integer(field + 7, "compressionVelocityDerivativeId", general.tensionVelocityDerivativeId);
    return general;
}

// What the IDs of a continuation point to: a TABLEDi for a table, a DEQATN for an equation.
ReferenceTarget targetOf(BushProp1DDefinition type)
{
    return type == BushProp1DDefinition::Table ? ReferenceTarget::DynamicTable : ReferenceTarget::DesignEquation;
}

} // namespace

std::optional<BushProp1DCard> BushProp1DFormat::read(FieldReader& reader)
{
    BushProp1DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stiffness = reader.real(2, "stiffness", 0.0);
    card.damping = reader.real(3, "damping", 0.0);
    card.mass = reader.real(4, "mass", 0.0);
    card.stressFactor = reader.real(6, "stressFactor", 0.0);
    card.strainFactor = reader.real(7, "strainFactor", 0.0);
    requireBlank(reader, 5, 5, "mass");
    requireBlank(reader, 8, 8, "strainFactor");

    // The continuations: each starts a line, SHOCKA takes two.
    int field = FIRST_GROUP_FIELD;
    while (field <= reader.lastUsedField())
    {
        const std::string_view name = reader.text(field);
        int lines = 1;
        const auto is = [&](const char* word) { return (name == word); };
        bool repeated = false;
        if (name.empty())
        {
            requireBlank(reader, field, field + FIELDS_PER_LINE - 1, "ShockAbsorber");
        }
        else if (is("ShockAbsorber"))
        {
            repeated = card.shock.present;
            card.shock = readShock(reader, field);
            lines = 2;
        }
        else if (is("Spring"))
        {
            repeated = card.spring.present;
            card.spring =
                readCurve(reader, field, "tensionDisplacementDerivativeId", "compressionDisplacementDerivativeId");
        }
        else if (is("Damper"))
        {
            repeated = card.damper.present;
            card.damper = readCurve(reader, field, "tensionVelocityDerivativeId", "compressionVelocityDerivativeId");
        }
        else if (is("General"))
        {
            repeated = card.general.present;
            card.general = readGeneral(reader, field);
        }
        else
        {
            reader.error("card.bad-value",
                         "a continuation of a " + std::string(reader.cardName()) +
                             " starts with ShockAbsorber, Spring, Damper or General, found \"" + std::string(name) +
                             "\"",
                         "ShockAbsorber");
        }
        if (repeated)
            reader.error("card.bad-value",
                         "a continuation of a " + std::string(reader.cardName()) + " may be given only once: \"" +
                             std::string(name) + "\"",
                         "ShockAbsorber");
        field += lines * FIELDS_PER_LINE;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushProp1DFormat::check(const BushProp1DCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.stiffness < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "stiffness"};
    if (card.damping < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "damping"};
    if (card.mass < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "mass"};
    if (card.stressFactor < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "stressFactor"};
    if (card.strainFactor < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "strainFactor"};

    if (card.shock.present)
    {
        const BushProp1DShock& shock = card.shock;
        if (shock.dampingTension < 0.0 || shock.dampingCompression < 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " and " +
                                   CardProblem::fieldNamed("dampingCompression") + " must not be negative",
                               "dampingTension"};
        if (shock.scaleTableId < 0 || shock.scaleTensionEquationId < 0 || shock.scaleCompressionEquationId < 0 ||
            shock.scaleTensionDerivativeEquationId < 0 || shock.scaleCompressionDerivativeEquationId < 0)
            return CardProblem{"card.bad-id", "the table and equation IDs must not be negative", "scaleTableId"};
        if (shock.type == BushProp1DDefinition::Table)
        {
            if (shock.scaleTableId <= 0)
                return CardProblem{"card.missing-field",
                                   std::string(CardProblem::FIELD) + " is required with " +
                                       CardProblem::fieldNamed("kind") + " = TABLE",
                                   "scaleTableId"};
            if (shock.scaleTensionEquationId != 0 || shock.scaleCompressionEquationId != 0 ||
                shock.scaleTensionDerivativeEquationId != 0 || shock.scaleCompressionDerivativeEquationId != 0)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + ", " +
                                       CardProblem::fieldNamed("scaleCompressionEquationId") + ", " +
                                       CardProblem::fieldNamed("scaleTensionDerivativeEquationId") + " and " +
                                       CardProblem::fieldNamed("scaleCompressionDerivativeEquationId") + " belong to " +
                                       CardProblem::fieldNamed("kind") + " = EQUAT",
                                   "scaleTensionEquationId"};
        }
        else
        {
            if (shock.scaleTensionEquationId <= 0)
                return CardProblem{"card.missing-field",
                                   std::string(CardProblem::FIELD) + " is required with " +
                                       CardProblem::fieldNamed("kind") + " = EQUAT",
                                   "scaleTensionEquationId"};
            if (shock.scaleTensionDerivativeEquationId <= 0)
                return CardProblem{"card.missing-field",
                                   std::string(CardProblem::FIELD) + " is required with " +
                                       CardProblem::fieldNamed("kind") + " = EQUAT",
                                   "scaleTensionDerivativeEquationId"};
            if (shock.scaleTableId != 0)
                return CardProblem{
                    "card.bad-value",
                    std::string(CardProblem::FIELD) + " belongs to " + CardProblem::fieldNamed("kind") + " = TABLE",
                    "scaleTableId"};
        }
    }
    const BushProp1DCurve* const curves[2] = {&card.spring, &card.damper};
    for (int index = 0; index < 2; ++index)
    {
        const BushProp1DCurve& curve = *curves[index];
        if (!curve.present)
            continue;
        const char* const derivativeField =
            index == 0 ? "tensionDisplacementDerivativeId" : "tensionVelocityDerivativeId";
        if (curve.tensionCurveId <= 0)
            return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required",
                               "tensionCurveId"};
        if (curve.compressionCurveId <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be greater than 0",
                               "compressionCurveId"};
        if (curve.tensionDerivativeId < 0 || curve.compressionDerivativeId < 0)
            return CardProblem{"card.bad-id", "the equation IDs must not be negative", derivativeField};
        if (curve.type == BushProp1DDefinition::Table)
        {
            if (curve.tensionDerivativeId != 0 || curve.compressionDerivativeId != 0)
                return CardProblem{"card.bad-value",
                                   "the derivative equations belong to " + CardProblem::fieldNamed("kind") + " = EQUAT",
                                   derivativeField};
        }
        else if (curve.tensionDerivativeId <= 0 || curve.compressionDerivativeId <= 0)
        {
            return CardProblem{
                "card.missing-field",
                "the derivative equations are required with " + CardProblem::fieldNamed("kind") + " = EQUAT",
                derivativeField};
        }
    }
    if (card.general.present)
    {
        const BushProp1DGeneral& general = card.general;
        if (general.tensionCurveId <= 0 || general.compressionCurveId <= 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " is required (and " +
                                   CardProblem::fieldNamed("compressionCurveId") + ", which defaults to it)",
                               "tensionCurveId"};
        if (general.tensionDisplacementDerivativeId <= 0 || general.compressionDisplacementDerivativeId <= 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " is required (and " +
                                   CardProblem::fieldNamed("compressionDisplacementDerivativeId") +
                                   ", which defaults to it)",
                               "tensionDisplacementDerivativeId"};
        if (general.tensionVelocityDerivativeId <= 0 || general.compressionVelocityDerivativeId <= 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " is required (and " +
                                   CardProblem::fieldNamed("compressionVelocityDerivativeId") +
                                   ", which defaults to it)",
                               "tensionVelocityDerivativeId"};
    }
    return std::nullopt;
}

void BushProp1DFormat::write(const BushProp1DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.stiffness, 0.0);
    writer.realUnlessDefault(3, card.damping, 0.0);
    writer.realUnlessDefault(4, card.mass, 0.0);
    writer.realUnlessDefault(6, card.stressFactor, 0.0);
    writer.realUnlessDefault(7, card.strainFactor, 0.0);
    int field = FIRST_GROUP_FIELD;
    if (card.shock.present)
    {
        const BushProp1DShock& shock = card.shock;
        writer.text(field, "ShockAbsorber");
        writer.textUnlessDefault(field + 1, definitionName(shock.type), "Table");
        writer.real(field + 2, shock.dampingTension);
        // CVC, EXPVT and EXPVC have defaults that depend on other fields: always written.
        writer.real(field + 3, shock.dampingCompression);
        writer.real(field + 4, shock.velocityExponentTension);
        writer.real(field + 5, shock.velocityExponentCompression);
        if (shock.type == BushProp1DDefinition::Table)
        {
            writer.integer(field + 6, shock.scaleTableId);
        }
        else
        {
            writer.integer(field + 10, shock.scaleTensionEquationId);
            writer.integer(field + 11, shock.scaleCompressionEquationId);
            writer.integer(field + 12, shock.scaleTensionDerivativeEquationId);
            writer.integer(field + 13, shock.scaleCompressionDerivativeEquationId);
        }
        field += 2 * FIELDS_PER_LINE;
    }
    for (const BushProp1DCurve* curve : {&card.spring, &card.damper})
    {
        if (!curve->present)
            continue;
        writer.text(field, curve == &card.spring ? "Spring" : "Damper");
        writer.textUnlessDefault(field + 1, definitionName(curve->type), "Table");
        writer.integer(field + 2, curve->tensionCurveId);
        writer.integer(field + 3, curve->compressionCurveId); // defaults to IDT: always written
        if (curve->type == BushProp1DDefinition::Equation)
        {
            writer.integer(field + 4, curve->tensionDerivativeId);
            writer.integer(field + 5, curve->compressionDerivativeId);
        }
        field += FIELDS_PER_LINE;
    }
    if (card.general.present)
    {
        const BushProp1DGeneral& general = card.general;
        writer.text(field, "General");
        writer.integer(field + 2, general.tensionCurveId);
        writer.integer(field + 3, general.compressionCurveId);
        writer.integer(field + 4, general.tensionDisplacementDerivativeId);
        writer.integer(field + 5, general.compressionDisplacementDerivativeId);
        writer.integer(field + 6, general.tensionVelocityDerivativeId);
        writer.integer(field + 7, general.compressionVelocityDerivativeId);
    }
}

std::uint32_t BushProp1DTable::append(const BushProp1DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_damping.push(card.damping);
    m_mass.push(card.mass);
    m_stressFactor.push(card.stressFactor);
    m_strainFactor.push(card.strainFactor);
    m_groups.push(
        static_cast<std::uint8_t>((card.shock.present ? HAS_SHOCKA : 0) | (card.spring.present ? HAS_SPRING : 0) |
                                  (card.damper.present ? HAS_DAMPER : 0) | (card.general.present ? HAS_GENER : 0)));
    m_shockType.push(static_cast<std::uint8_t>(card.shock.type));
    m_shock.push({card.shock.dampingTension, card.shock.dampingCompression, card.shock.velocityExponentTension,
                  card.shock.velocityExponentCompression});
    m_shockIds.push({card.shock.scaleTableId, card.shock.scaleTensionEquationId, card.shock.scaleCompressionEquationId,
                     card.shock.scaleTensionDerivativeEquationId, card.shock.scaleCompressionDerivativeEquationId});
    m_springType.push(static_cast<std::uint8_t>(card.spring.type));
    m_springIds.push({card.spring.tensionCurveId, card.spring.compressionCurveId, card.spring.tensionDerivativeId,
                      card.spring.compressionDerivativeId});
    m_damperType.push(static_cast<std::uint8_t>(card.damper.type));
    m_damperIds.push({card.damper.tensionCurveId, card.damper.compressionCurveId, card.damper.tensionDerivativeId,
                      card.damper.compressionDerivativeId});
    m_generalIds.push({card.general.tensionCurveId, card.general.compressionCurveId,
                       card.general.tensionDisplacementDerivativeId, card.general.compressionDisplacementDerivativeId,
                       card.general.tensionVelocityDerivativeId, card.general.compressionVelocityDerivativeId});
    return row;
}

BushProp1DCard BushProp1DTable::card(std::uint32_t row) const
{
    BushProp1DCard card;
    card.id = m_id.get(row);
    card.stiffness = m_stiffness.get(row);
    card.damping = m_damping.get(row);
    card.mass = m_mass.get(row);
    card.stressFactor = m_stressFactor.get(row);
    card.strainFactor = m_strainFactor.get(row);
    const std::uint8_t groups = m_groups.get(row);
    card.shock.present = (groups & HAS_SHOCKA) != 0;
    card.spring.present = (groups & HAS_SPRING) != 0;
    card.damper.present = (groups & HAS_DAMPER) != 0;
    card.general.present = (groups & HAS_GENER) != 0;
    card.shock.type = static_cast<BushProp1DDefinition>(m_shockType.get(row));
    const std::array<double, 4> shock = m_shock.get(row);
    card.shock.dampingTension = shock[0];
    card.shock.dampingCompression = shock[1];
    card.shock.velocityExponentTension = shock[2];
    card.shock.velocityExponentCompression = shock[3];
    const std::array<base::EntityId, 5> shockIds = m_shockIds.get(row);
    card.shock.scaleTableId = shockIds[0];
    card.shock.scaleTensionEquationId = shockIds[1];
    card.shock.scaleCompressionEquationId = shockIds[2];
    card.shock.scaleTensionDerivativeEquationId = shockIds[3];
    card.shock.scaleCompressionDerivativeEquationId = shockIds[4];
    card.spring.type = static_cast<BushProp1DDefinition>(m_springType.get(row));
    const std::array<base::EntityId, 4> spring = m_springIds.get(row);
    card.spring.tensionCurveId = spring[0];
    card.spring.compressionCurveId = spring[1];
    card.spring.tensionDerivativeId = spring[2];
    card.spring.compressionDerivativeId = spring[3];
    card.damper.type = static_cast<BushProp1DDefinition>(m_damperType.get(row));
    const std::array<base::EntityId, 4> damper = m_damperIds.get(row);
    card.damper.tensionCurveId = damper[0];
    card.damper.compressionCurveId = damper[1];
    card.damper.tensionDerivativeId = damper[2];
    card.damper.compressionDerivativeId = damper[3];
    const std::array<base::EntityId, 6> general = m_generalIds.get(row);
    card.general.tensionCurveId = general[0];
    card.general.compressionCurveId = general[1];
    card.general.tensionDisplacementDerivativeId = general[2];
    card.general.compressionDisplacementDerivativeId = general[3];
    card.general.tensionVelocityDerivativeId = general[4];
    card.general.compressionVelocityDerivativeId = general[5];
    return card;
}

void BushProp1DTable::store(std::uint32_t row, const BushProp1DCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_damping.set(row, card.damping);
    m_mass.set(row, card.mass);
    m_stressFactor.set(row, card.stressFactor);
    m_strainFactor.set(row, card.strainFactor);
    m_groups.set(row, static_cast<std::uint8_t>(
                          (card.shock.present ? HAS_SHOCKA : 0) | (card.spring.present ? HAS_SPRING : 0) |
                          (card.damper.present ? HAS_DAMPER : 0) | (card.general.present ? HAS_GENER : 0)));
    m_shockType.set(row, static_cast<std::uint8_t>(card.shock.type));
    m_shock.set(row, {card.shock.dampingTension, card.shock.dampingCompression, card.shock.velocityExponentTension,
                      card.shock.velocityExponentCompression});
    m_shockIds.set(row,
                   {card.shock.scaleTableId, card.shock.scaleTensionEquationId, card.shock.scaleCompressionEquationId,
                    card.shock.scaleTensionDerivativeEquationId, card.shock.scaleCompressionDerivativeEquationId});
    m_springType.set(row, static_cast<std::uint8_t>(card.spring.type));
    m_springIds.set(row, {card.spring.tensionCurveId, card.spring.compressionCurveId, card.spring.tensionDerivativeId,
                          card.spring.compressionDerivativeId});
    m_damperType.set(row, static_cast<std::uint8_t>(card.damper.type));
    m_damperIds.set(row, {card.damper.tensionCurveId, card.damper.compressionCurveId, card.damper.tensionDerivativeId,
                          card.damper.compressionDerivativeId});
    m_generalIds.set(row,
                     {card.general.tensionCurveId, card.general.compressionCurveId,
                      card.general.tensionDisplacementDerivativeId, card.general.compressionDisplacementDerivativeId,
                      card.general.tensionVelocityDerivativeId, card.general.compressionVelocityDerivativeId});
}

void BushProp1DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_damping.keepRows(rows);
    m_mass.keepRows(rows);
    m_stressFactor.keepRows(rows);
    m_strainFactor.keepRows(rows);
    m_groups.keepRows(rows);
    m_shockType.keepRows(rows);
    m_shock.keepRows(rows);
    m_shockIds.keepRows(rows);
    m_springType.keepRows(rows);
    m_springIds.keepRows(rows);
    m_damperType.keepRows(rows);
    m_damperIds.keepRows(rows);
    m_generalIds.keepRows(rows);
}

void BushProp1DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const std::uint8_t groups = m_groups.get(row);
    const auto visit = [&](const char* field, ReferenceTarget target, base::EntityId id)
    { return id > 0 ? visitor.visit(field, target, id) : id; };
    if ((groups & HAS_SHOCKA) != 0)
    {
        const ReferenceTarget target = targetOf(static_cast<BushProp1DDefinition>(m_shockType.get(row)));
        std::array<base::EntityId, 5> ids = m_shockIds.get(row);
        ids[0] = visit("scaleTableId", target, ids[0]);
        ids[1] = visit("scaleTensionEquationId", target, ids[1]);
        ids[2] = visit("scaleCompressionEquationId", target, ids[2]);
        ids[3] = visit("scaleTensionDerivativeEquationId", target, ids[3]);
        ids[4] = visit("scaleCompressionDerivativeEquationId", target, ids[4]);
        m_shockIds.set(row, ids);
    }
    if ((groups & HAS_SPRING) != 0)
    {
        const ReferenceTarget target = targetOf(static_cast<BushProp1DDefinition>(m_springType.get(row)));
        std::array<base::EntityId, 4> ids = m_springIds.get(row);
        ids[0] = visit("tensionCurveId", target, ids[0]);
        ids[1] = visit("compressionCurveId", target, ids[1]);
        // The derivative of a curve given by equations is an equation too (of a table: a table that is not typed
        // apart).
        const ReferenceTarget derivative = target == ReferenceTarget::DesignEquation ? target : ReferenceTarget::Other;
        ids[2] = visit("tensionDisplacementDerivativeId", derivative, ids[2]);
        ids[3] = visit("compressionDisplacementDerivativeId", derivative, ids[3]);
        m_springIds.set(row, ids);
    }
    if ((groups & HAS_DAMPER) != 0)
    {
        const ReferenceTarget target = targetOf(static_cast<BushProp1DDefinition>(m_damperType.get(row)));
        std::array<base::EntityId, 4> ids = m_damperIds.get(row);
        ids[0] = visit("tensionCurveId", target, ids[0]);
        ids[1] = visit("compressionCurveId", target, ids[1]);
        const ReferenceTarget derivative = target == ReferenceTarget::DesignEquation ? target : ReferenceTarget::Other;
        ids[2] = visit("tensionVelocityDerivativeId", derivative, ids[2]);
        ids[3] = visit("compressionVelocityDerivativeId", derivative, ids[3]);
        m_damperIds.set(row, ids);
    }
    if ((groups & HAS_GENER) != 0)
    {
        // The equations of a GENER are DEQATN entries.
        std::array<base::EntityId, 6> ids = m_generalIds.get(row);
        const char* const names[6] = {"tensionCurveId",
                                      "compressionCurveId",
                                      "tensionDisplacementDerivativeId",
                                      "compressionDisplacementDerivativeId",
                                      "tensionVelocityDerivativeId",
                                      "compressionVelocityDerivativeId"};
        for (std::size_t index = 0; index < 6; ++index)
            ids[index] = visit(names[index], ReferenceTarget::DesignEquation, ids[index]);
        m_generalIds.set(row, ids);
    }
}

} // namespace oor::model
