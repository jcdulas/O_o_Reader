#include "oor/query/operators/ExpressionEvaluator.hpp"

#include "ExpressionNode.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace oor::query
{

namespace
{

// The values of an expression over one chunk of rows. A `scalar` holds one value for every row (a constant).
struct Vec
{
    ValueType type = ValueType::Bool;
    bool scalar = false;
    std::vector<std::uint8_t> bools;
    std::vector<std::int64_t> ints;
    std::vector<double> reals;
    std::vector<std::string_view> texts;
    std::vector<std::uint8_t> valid; // empty: every value is there; else one flag per value (one for a scalar)

    [[nodiscard]] bool validAt(std::size_t index) const noexcept
    {
        return valid.empty() || valid[scalar ? 0 : index] != 0;
    }
};

using Resolved = std::unordered_map<std::string, const QueryColumn*>;

// A number operand as doubles: the vector itself when it is one, else a converted copy.
struct RealOperand
{
    RealOperand(const Vec& vec, std::size_t count)
    {
        const std::size_t length = vec.scalar ? 1 : count;
        stride = vec.scalar ? 0 : 1;
        if (vec.type == ValueType::Double)
        {
            data = vec.reals.data();
            return;
        }
        storage.resize(length);
        for (std::size_t index = 0; index < length; ++index)
        {
            storage[index] =
                vec.type == ValueType::Int ? static_cast<double>(vec.ints[index]) : (vec.bools[index] != 0 ? 1.0 : 0.0);
        }
        data = storage.data();
    }
    std::vector<double> storage;
    const double* data = nullptr;
    std::size_t stride = 1;
};

// A Bool or Int operand as integers.
struct IntOperand
{
    IntOperand(const Vec& vec, std::size_t count)
    {
        const std::size_t length = vec.scalar ? 1 : count;
        stride = vec.scalar ? 0 : 1;
        if (vec.type == ValueType::Int)
        {
            data = vec.ints.data();
            return;
        }
        storage.resize(length);
        for (std::size_t index = 0; index < length; ++index)
            storage[index] = vec.bools[index] != 0 ? 1 : 0;
        data = storage.data();
    }
    std::vector<std::int64_t> storage;
    const std::int64_t* data = nullptr;
    std::size_t stride = 1;
};

// One flag per row (or one for a scalar result): a value is there when it is there in both operands.
std::vector<std::uint8_t> mergeValidity(const Vec& left, const Vec& right, std::size_t count, bool scalarResult)
{
    if (left.valid.empty() && right.valid.empty())
        return {};
    const std::size_t length = scalarResult ? 1 : count;
    std::vector<std::uint8_t> valid(length);
    for (std::size_t index = 0; index < length; ++index)
        valid[index] = left.validAt(index) && right.validAt(index) ? 1 : 0;
    return valid;
}

template <typename Source, typename Target, typename Function>
void applyBinary(std::size_t length, const Source* left, std::size_t leftStride, const Source* right,
                 std::size_t rightStride, Target* out, Function function)
{
    for (std::size_t index = 0; index < length; ++index)
        out[index] = function(left[index * leftStride], right[index * rightStride]);
}

class ChunkEvaluator
{
public:
    explicit ChunkEvaluator(const Resolved& columns) : m_columns(columns) {}

    [[nodiscard]] Vec evaluate(const Expression::Node& node, std::size_t begin, std::size_t end) const
    {
        const std::size_t count = end - begin;
        switch (node.kind)
        {
        case NodeKind::Literal:
            return literal(node.literal);
        case NodeKind::Column:
            return load(*m_columns.at(node.name), begin, end);
        case NodeKind::Negate:
            return negate(evaluate(*node.left, begin, end), count);
        case NodeKind::Not:
            return logicalNot(evaluate(*node.left, begin, end), count);
        case NodeKind::Binary:
        {
            const Vec left = evaluate(*node.left, begin, end);
            const Vec right = evaluate(*node.right, begin, end);
            switch (node.op)
            {
            case BinaryOp::And:
            case BinaryOp::Or:
                return logic(node.op, left, right, count);
            case BinaryOp::Equal:
            case BinaryOp::NotEqual:
            case BinaryOp::Less:
            case BinaryOp::LessEqual:
            case BinaryOp::Greater:
            case BinaryOp::GreaterEqual:
                return compare(node.op, left, right, count);
            default:
                return arithmetic(node.op, left, right, count);
            }
        }
        case NodeKind::In:
            return membership(node, evaluate(*node.left, begin, end), count);
        case NodeKind::IsNull:
            return isNull(evaluate(*node.left, begin, end), count, node.negated);
        case NodeKind::Call:
            return call(node.function, evaluate(*node.arguments.front(), begin, end), count);
        }
        return {};
    }

private:
    static Vec literal(const Literal& value)
    {
        Vec vec;
        vec.scalar = true;
        switch (value.kind)
        {
        case Literal::Kind::Bool:
            vec.type = ValueType::Bool;
            vec.bools.assign(1, value.boolean ? 1 : 0);
            break;
        case Literal::Kind::Int:
            vec.type = ValueType::Int;
            vec.ints.assign(1, value.integer);
            break;
        case Literal::Kind::Double:
            vec.type = ValueType::Double;
            vec.reals.assign(1, value.real);
            break;
        case Literal::Kind::Text:
            vec.type = ValueType::Text;
            vec.texts.assign(1, std::string_view(value.text));
            break;
        case Literal::Kind::Null:
            vec.type = ValueType::Double;
            vec.reals.assign(1, 0.0);
            vec.valid.assign(1, 0);
            break;
        }
        return vec;
    }

    static Vec load(const QueryColumn& column, std::size_t begin, std::size_t end)
    {
        const std::size_t count = end - begin;
        Vec vec;
        switch (column.type())
        {
        case ColumnType::Float64:
        {
            vec.type = ValueType::Double;
            const auto values = column.as<double>();
            vec.reals.assign(values.begin() + static_cast<std::ptrdiff_t>(begin),
                             values.begin() + static_cast<std::ptrdiff_t>(end));
            break;
        }
        case ColumnType::Float32:
        {
            vec.type = ValueType::Double;
            const auto values = column.as<float>();
            vec.reals.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.reals[row] = static_cast<double>(values[begin + row]);
            break;
        }
        case ColumnType::Int32:
        {
            vec.type = ValueType::Int;
            const auto values = column.as<std::int32_t>();
            vec.ints.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.ints[row] = values[begin + row];
            break;
        }
        case ColumnType::UInt32:
        {
            vec.type = ValueType::Int;
            const auto values = column.as<std::uint32_t>();
            vec.ints.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.ints[row] = values[begin + row];
            break;
        }
        case ColumnType::UInt8:
        {
            vec.type = ValueType::Int;
            const auto values = column.as<std::uint8_t>();
            vec.ints.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.ints[row] = values[begin + row];
            break;
        }
        case ColumnType::Int64:
        {
            vec.type = ValueType::Int;
            const auto values = column.as<std::int64_t>();
            vec.ints.assign(values.begin() + static_cast<std::ptrdiff_t>(begin),
                            values.begin() + static_cast<std::ptrdiff_t>(end));
            break;
        }
        case ColumnType::Bool:
            vec.type = ValueType::Bool;
            vec.bools.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.bools[row] = column.boolAt(begin + row) ? 1 : 0;
            break;
        case ColumnType::Utf8:
        case ColumnType::Dictionary:
            vec.type = ValueType::Text;
            vec.texts.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.texts[row] = column.textAt(begin + row);
            break;
        default:
            throw QueryTypeError("a list column cannot be used in an expression");
        }
        if (column.hasNulls())
        {
            vec.valid.resize(count);
            for (std::size_t row = 0; row < count; ++row)
                vec.valid[row] = column.isNull(begin + row) ? 0 : 1;
        }
        return vec;
    }

    static Vec negate(const Vec& operand, std::size_t count)
    {
        Vec out;
        out.scalar = operand.scalar;
        out.valid = operand.valid;
        const std::size_t length = operand.scalar ? 1 : count;
        if (operand.type == ValueType::Double)
        {
            out.type = ValueType::Double;
            out.reals.resize(length);
            for (std::size_t index = 0; index < length; ++index)
                out.reals[index] = -operand.reals[index];
        }
        else
        {
            const IntOperand values(operand, count);
            out.type = ValueType::Int;
            out.ints.resize(length);
            for (std::size_t index = 0; index < length; ++index)
                out.ints[index] = -values.data[index];
        }
        return out;
    }

    static Vec logicalNot(const Vec& operand, std::size_t count)
    {
        Vec out;
        out.type = ValueType::Bool;
        out.scalar = operand.scalar;
        out.valid = operand.valid;
        const std::size_t length = operand.scalar ? 1 : count;
        out.bools.resize(length);
        for (std::size_t index = 0; index < length; ++index)
            out.bools[index] = operand.bools[index] != 0 ? 0 : 1;
        return out;
    }

    // Three-valued logic: false and null is false, true or null is true.
    static Vec logic(BinaryOp op, const Vec& left, const Vec& right, std::size_t count)
    {
        Vec out;
        out.type = ValueType::Bool;
        out.scalar = left.scalar && right.scalar;
        const std::size_t length = out.scalar ? 1 : count;
        out.bools.resize(length);
        bool anyNull = false;
        std::vector<std::uint8_t> valid(length, 1);
        for (std::size_t index = 0; index < length; ++index)
        {
            const bool leftValid = left.validAt(index);
            const bool rightValid = right.validAt(index);
            const bool leftValue = left.bools[left.scalar ? 0 : index] != 0;
            const bool rightValue = right.bools[right.scalar ? 0 : index] != 0;
            bool value = false;
            bool known = true;
            if (op == BinaryOp::And)
            {
                if ((leftValid && !leftValue) || (rightValid && !rightValue))
                    value = false;
                else if (leftValid && rightValid)
                    value = true;
                else
                    known = false;
            }
            else
            {
                if ((leftValid && leftValue) || (rightValid && rightValue))
                    value = true;
                else if (leftValid && rightValid)
                    value = false;
                else
                    known = false;
            }
            out.bools[index] = value ? 1 : 0;
            valid[index] = known ? 1 : 0;
            anyNull = anyNull || !known;
        }
        if (anyNull)
            out.valid = std::move(valid);
        return out;
    }

    static Vec compare(BinaryOp op, const Vec& left, const Vec& right, std::size_t count)
    {
        Vec out;
        out.type = ValueType::Bool;
        out.scalar = left.scalar && right.scalar;
        const std::size_t length = out.scalar ? 1 : count;
        out.bools.resize(length);
        out.valid = mergeValidity(left, right, count, out.scalar);
        const std::size_t leftStride = left.scalar ? 0 : 1;
        const std::size_t rightStride = right.scalar ? 0 : 1;
        if (left.type == ValueType::Text)
        {
            for (std::size_t index = 0; index < length; ++index)
            {
                const int order = left.texts[index * leftStride].compare(right.texts[index * rightStride]);
                out.bools[index] = decide(op, order) ? 1 : 0;
            }
            return out;
        }
        const bool integers = left.type != ValueType::Double && right.type != ValueType::Double;
        if (integers)
        {
            const IntOperand a(left, count);
            const IntOperand b(right, count);
            for (std::size_t index = 0; index < length; ++index)
            {
                const std::int64_t x = a.data[index * a.stride];
                const std::int64_t y = b.data[index * b.stride];
                out.bools[index] = decide(op, x < y ? -1 : (x > y ? 1 : 0)) ? 1 : 0;
            }
            return out;
        }
        const RealOperand a(left, count);
        const RealOperand b(right, count);
        for (std::size_t index = 0; index < length; ++index)
        {
            const double x = a.data[index * a.stride];
            const double y = b.data[index * b.stride];
            bool result = false;
            switch (op)
            {
            case BinaryOp::Equal:
                result = x == y;
                break;
            case BinaryOp::NotEqual:
                result = x != y;
                break;
            case BinaryOp::Less:
                result = x < y;
                break;
            case BinaryOp::LessEqual:
                result = x <= y;
                break;
            case BinaryOp::Greater:
                result = x > y;
                break;
            default:
                result = x >= y;
                break;
            }
            out.bools[index] = result ? 1 : 0;
        }
        return out;
    }

    static bool decide(BinaryOp op, int order) noexcept
    {
        switch (op)
        {
        case BinaryOp::Equal:
            return order == 0;
        case BinaryOp::NotEqual:
            return order != 0;
        case BinaryOp::Less:
            return order < 0;
        case BinaryOp::LessEqual:
            return order <= 0;
        case BinaryOp::Greater:
            return order > 0;
        default:
            return order >= 0;
        }
    }

    static Vec arithmetic(BinaryOp op, const Vec& left, const Vec& right, std::size_t count)
    {
        Vec out;
        out.scalar = left.scalar && right.scalar;
        const std::size_t length = out.scalar ? 1 : count;
        out.valid = mergeValidity(left, right, count, out.scalar);
        const bool integers = left.type != ValueType::Double && right.type != ValueType::Double &&
                              op != BinaryOp::Divide && op != BinaryOp::Power;
        if (integers)
        {
            const IntOperand a(left, count);
            const IntOperand b(right, count);
            out.type = ValueType::Int;
            out.ints.resize(length);
            std::int64_t* target = out.ints.data();
            switch (op)
            {
            case BinaryOp::Add:
                applyBinary(length, a.data, a.stride, b.data, b.stride, target,
                            [](std::int64_t x, std::int64_t y) { return x + y; });
                break;
            case BinaryOp::Subtract:
                applyBinary(length, a.data, a.stride, b.data, b.stride, target,
                            [](std::int64_t x, std::int64_t y) { return x - y; });
                break;
            case BinaryOp::Multiply:
                applyBinary(length, a.data, a.stride, b.data, b.stride, target,
                            [](std::int64_t x, std::int64_t y) { return x * y; });
                break;
            default: // modulo: a zero divisor gives a null
            {
                if (out.valid.empty())
                    out.valid.assign(length, 1);
                for (std::size_t index = 0; index < length; ++index)
                {
                    const std::int64_t y = b.data[index * b.stride];
                    if (y == 0)
                    {
                        out.valid[index] = 0;
                        target[index] = 0;
                    }
                    else
                    {
                        // The sign follows the divisor, as in Python.
                        const std::int64_t x = a.data[index * a.stride];
                        std::int64_t result = x % y;
                        if (result != 0 && ((result < 0) != (y < 0)))
                            result += y;
                        target[index] = result;
                    }
                }
                break;
            }
            }
            return out;
        }
        const RealOperand a(left, count);
        const RealOperand b(right, count);
        out.type = ValueType::Double;
        out.reals.resize(length);
        double* target = out.reals.data();
        switch (op)
        {
        case BinaryOp::Add:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target, [](double x, double y) { return x + y; });
            break;
        case BinaryOp::Subtract:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target, [](double x, double y) { return x - y; });
            break;
        case BinaryOp::Multiply:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target, [](double x, double y) { return x * y; });
            break;
        case BinaryOp::Divide:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target, [](double x, double y) { return x / y; });
            break;
        case BinaryOp::Power:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target,
                        [](double x, double y) { return std::pow(x, y); });
            break;
        default:
            applyBinary(length, a.data, a.stride, b.data, b.stride, target,
                        [](double x, double y)
                        {
                            const double result = std::fmod(x, y);
                            return (result != 0.0 && ((result < 0.0) != (y < 0.0))) ? result + y : result;
                        });
            break;
        }
        return out;
    }

    static Vec membership(const Expression::Node& node, const Vec& operand, std::size_t count)
    {
        Vec out;
        out.type = ValueType::Bool;
        out.scalar = operand.scalar;
        out.valid = operand.valid;
        const std::size_t length = operand.scalar ? 1 : count;
        out.bools.assign(length, 0);
        if (operand.type == ValueType::Text)
        {
            std::vector<std::string_view> wanted;
            for (const Literal& literal : node.list)
            {
                if (literal.kind == Literal::Kind::Text)
                    wanted.emplace_back(literal.text);
            }
            std::sort(wanted.begin(), wanted.end());
            for (std::size_t index = 0; index < length; ++index)
            {
                const bool found = std::binary_search(wanted.begin(), wanted.end(), operand.texts[index]);
                out.bools[index] = found != node.negated ? 1 : 0;
            }
            return out;
        }
        bool allIntegers = operand.type != ValueType::Double;
        for (const Literal& literal : node.list)
            allIntegers = allIntegers && literal.kind != Literal::Kind::Double;
        if (allIntegers)
        {
            std::vector<std::int64_t> wanted;
            for (const Literal& literal : node.list)
            {
                if (literal.kind == Literal::Kind::Int)
                    wanted.push_back(literal.integer);
                else if (literal.kind == Literal::Kind::Bool)
                    wanted.push_back(literal.boolean ? 1 : 0);
            }
            std::sort(wanted.begin(), wanted.end());
            const IntOperand values(operand, count);
            for (std::size_t index = 0; index < length; ++index)
            {
                const bool found = std::binary_search(wanted.begin(), wanted.end(), values.data[index * values.stride]);
                out.bools[index] = found != node.negated ? 1 : 0;
            }
            return out;
        }
        std::vector<double> wanted;
        for (const Literal& literal : node.list)
        {
            if (literal.kind == Literal::Kind::Int)
                wanted.push_back(static_cast<double>(literal.integer));
            else if (literal.kind == Literal::Kind::Double)
                wanted.push_back(literal.real);
            else if (literal.kind == Literal::Kind::Bool)
                wanted.push_back(literal.boolean ? 1.0 : 0.0);
        }
        const RealOperand values(operand, count);
        for (std::size_t index = 0; index < length; ++index)
        {
            const double value = values.data[index * values.stride];
            const bool found = std::find(wanted.begin(), wanted.end(), value) != wanted.end();
            out.bools[index] = found != node.negated ? 1 : 0;
        }
        return out;
    }

    static Vec isNull(const Vec& operand, std::size_t count, bool negated)
    {
        Vec out;
        out.type = ValueType::Bool;
        out.scalar = operand.scalar;
        const std::size_t length = operand.scalar ? 1 : count;
        out.bools.resize(length);
        for (std::size_t index = 0; index < length; ++index)
            out.bools[index] = (!operand.validAt(index)) != negated ? 1 : 0;
        return out;
    }

    static Vec call(Function function, const Vec& operand, std::size_t count)
    {
        Vec out;
        out.scalar = operand.scalar;
        const std::size_t length = operand.scalar ? 1 : count;
        switch (function)
        {
        case Function::IsNull:
            return isNull(operand, count, false);
        case Function::IsNan:
        {
            out.type = ValueType::Bool;
            out.bools.assign(length, 0);
            if (operand.type == ValueType::Double)
            {
                for (std::size_t index = 0; index < length; ++index)
                    out.bools[index] = std::isnan(operand.reals[index]) && operand.validAt(index) ? 1 : 0;
            }
            return out;
        }
        case Function::Sqrt:
        {
            const RealOperand values(operand, count);
            out.type = ValueType::Double;
            out.valid = operand.valid;
            out.reals.resize(length);
            for (std::size_t index = 0; index < length; ++index)
                out.reals[index] = std::sqrt(values.data[index * values.stride]);
            return out;
        }
        case Function::Abs:
            out.valid = operand.valid;
            if (operand.type == ValueType::Double)
            {
                out.type = ValueType::Double;
                out.reals.resize(length);
                for (std::size_t index = 0; index < length; ++index)
                    out.reals[index] = std::fabs(operand.reals[index]);
            }
            else
            {
                const IntOperand values(operand, count);
                out.type = ValueType::Int;
                out.ints.resize(length);
                for (std::size_t index = 0; index < length; ++index)
                {
                    const std::int64_t value = values.data[index * values.stride];
                    out.ints[index] = value < 0 ? -value : value;
                }
            }
            return out;
        }
        return out;
    }

    const Resolved& m_columns;
};

// Finds the columns an expression reads and checks its types: nothing runs on data if this fails.
Resolved resolveColumns(const Expression& expression, const ColumnLookup& lookup)
{
    Resolved resolved;
    for (const std::string& name : expression.columns())
    {
        const QueryColumn* column = lookup(name);
        if (column == nullptr)
            throw QueryError("unknown column '" + name + "' in '" + expression.toString() + "'");
        resolved.emplace(name, column);
    }
    return resolved;
}

std::optional<ColumnType> typeOf(const Resolved& resolved, const std::string& name)
{
    const ColumnType type = resolved.at(name)->type();
    if (type == ColumnType::FixedList || type == ColumnType::List)
        return std::nullopt;
    return type;
}

} // namespace

ValueType ExpressionEvaluator::resultType(const Expression& expression, const ColumnLookup& lookup)
{
    const Resolved resolved = resolveColumns(expression, lookup);
    return expression.resultType([&](const std::string& name) { return typeOf(resolved, name); });
}

QueryColumn ExpressionEvaluator::evaluate(const Expression& expression, const ColumnLookup& lookup,
                                          std::size_t rowCount, const ExecutionContext& context)
{
    const Resolved resolved = resolveColumns(expression, lookup);
    const ValueType type = expression.resultType([&](const std::string& name) { return typeOf(resolved, name); });
    const ChunkEvaluator evaluator(resolved);
    const Expression::Node& root = *expression.root();

    std::vector<std::uint8_t> flags(rowCount, 1);
    std::atomic<bool> anyNull{false};
    std::vector<std::uint8_t> bools;
    std::vector<std::int64_t> ints;
    std::vector<double> reals;
    std::vector<std::string_view> texts;
    switch (type)
    {
    case ValueType::Bool:
        bools.resize(rowCount);
        break;
    case ValueType::Int:
        ints.resize(rowCount);
        break;
    case ValueType::Double:
        reals.resize(rowCount);
        break;
    case ValueType::Text:
        texts.resize(rowCount);
        break;
    }
    context.parallel(rowCount, EXPRESSION_CHUNK,
                     [&](base::IndexRange range)
                     {
                         const Vec vec = evaluator.evaluate(root, range.begin, range.end);
                         const std::size_t count = range.end - range.begin;
                         for (std::size_t index = 0; index < count; ++index)
                         {
                             const std::size_t at = vec.scalar ? 0 : index;
                             switch (type)
                             {
                             case ValueType::Bool:
                                 bools[range.begin + index] = vec.bools[at];
                                 break;
                             case ValueType::Int:
                                 ints[range.begin + index] = vec.type == ValueType::Int ? vec.ints[at] : vec.bools[at];
                                 break;
                             case ValueType::Double:
                                 reals[range.begin + index] = vec.type == ValueType::Double ? vec.reals[at]
                                                              : vec.type == ValueType::Int
                                                                  ? static_cast<double>(vec.ints[at])
                                                                  : static_cast<double>(vec.bools[at]);
                                 break;
                             case ValueType::Text:
                                 texts[range.begin + index] = vec.texts[at];
                                 break;
                             }
                             if (!vec.validAt(index))
                             {
                                 flags[range.begin + index] = 0;
                                 anyNull.store(true, std::memory_order_relaxed);
                             }
                         }
                     });

    QueryColumn column;
    switch (type)
    {
    case ValueType::Bool:
        column = QueryColumn::boolean(bools);
        break;
    case ValueType::Int:
        column = QueryColumn::primitive(std::move(ints));
        break;
    case ValueType::Double:
        if (anyNull.load())
        {
            // a null of a column of floats is NaN as well, so that an array of the column shows it
            for (std::size_t row = 0; row < rowCount; ++row)
            {
                if (flags[row] == 0)
                    reals[row] = std::numeric_limits<double>::quiet_NaN();
            }
        }
        column = QueryColumn::primitive(std::move(reals));
        break;
    case ValueType::Text:
        column = QueryColumn::utf8(texts);
        break;
    }
    if (anyNull.load())
    {
        std::int64_t nullCount = 0;
        ColumnBuffer bits = validityFromFlags(flags, nullCount);
        column = column.withValidity(std::move(bits), nullCount);
    }
    return column;
}

std::vector<std::uint32_t> ExpressionEvaluator::selectRows(const Expression& expression, const ColumnLookup& lookup,
                                                           std::size_t rowCount, const ExecutionContext& context)
{
    const Resolved resolved = resolveColumns(expression, lookup);
    const ValueType type = expression.resultType([&](const std::string& name) { return typeOf(resolved, name); });
    if (type != ValueType::Bool)
        throw QueryTypeError("'" + expression.toString() + "' is a " + std::string(valueTypeName(type)) +
                             ", not a condition");
    const ChunkEvaluator evaluator(resolved);
    const Expression::Node& root = *expression.root();
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(rowCount, EXPRESSION_CHUNK);
    std::vector<std::vector<std::uint32_t>> kept(chunks.size());
    context.parallel(rowCount, EXPRESSION_CHUNK,
                     [&](base::IndexRange range)
                     {
                         const Vec vec = evaluator.evaluate(root, range.begin, range.end);
                         // The chunks are cut as `chunks` is, so the one that starts at range.begin is its index.
                         std::vector<std::uint32_t>& rows = kept[range.begin / EXPRESSION_CHUNK];
                         const std::size_t count = range.end - range.begin;
                         rows.reserve(count);
                         for (std::size_t index = 0; index < count; ++index)
                         {
                             const std::size_t at = vec.scalar ? 0 : index;
                             if (vec.bools[at] != 0 && vec.validAt(index))
                                 rows.push_back(static_cast<std::uint32_t>(range.begin + index));
                         }
                     });
    std::size_t total = 0;
    for (const auto& rows : kept)
        total += rows.size();
    std::vector<std::uint32_t> result;
    result.reserve(total);
    for (const auto& rows : kept)
        result.insert(result.end(), rows.begin(), rows.end());
    return result;
}

} // namespace oor::query
