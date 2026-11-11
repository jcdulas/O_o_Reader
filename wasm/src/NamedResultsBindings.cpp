#include "NamedResultsBindings.hpp"

#include "BindingSupport.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/results/core/NamedResultKinds.hpp"

#include <cmath>
#include <limits>

namespace oor::wasm
{
namespace
{
bool present(const val& value)
{
    return !value.isUndefined() && !value.isNull();
}

val metaObject(const results::NamedResultMeta& meta)
{
    auto result = val::object();
    result.set("kind", meta.kind);
    result.set("name", meta.name);
    result.set("subcase", meta.subcase);
    result.set("analysisCode", meta.analysisCode);
    result.set("designCycle", meta.designCycle);
    auto attributes = val::object();
    for (const auto& [name, value] : meta.attributes)
        attributes.set(name, value);
    result.set("attributes", attributes);
    return result;
}

val named(const api::Results& results)
{
    return guarded(
        [&]
        {
            auto array = val::array();
            for (const auto& meta : results.named())
                array.call<void>("push", metaObject(meta));
            return array;
        });
}

val readNamed(const api::Results& source, double index)
{
    return guarded(
        [&]
        {
            if (!std::isfinite(index) || index < 0 || index >= double(source.named().size()) ||
                std::floor(index) != index)
                throw base::UsageError("named result index is out of range");
            const auto data = source.readNamed(static_cast<std::size_t>(index));
            auto result = metaObject(source.named()[static_cast<std::size_t>(index)]);
            auto arrays = val::object();
            for (const auto& array : data->arrays)
            {
                auto value = val::object();
                value.set("type", std::string(results::arrayTypeName(array.type)));
                auto shape = val::array();
                for (const auto count : array.shape)
                    shape.call<void>("push", double(count));
                value.set("shape", shape);
                switch (array.type)
                {
                case results::ArrayType::Int32:
                    value.set("values",
                              typedArray("Int32Array", array.values<std::int32_t>().data(), array.valueCount()));
                    break;
                case results::ArrayType::Int64:
                    value.set("values",
                              typedArray("BigInt64Array", array.values<std::int64_t>().data(), array.valueCount()));
                    break;
                case results::ArrayType::Float32:
                case results::ArrayType::Complex64:
                    value.set("values", typedArray("Float32Array", array.values<float>().data(),
                                                   array.byteCount() / sizeof(float)));
                    break;
                case results::ArrayType::Float64:
                case results::ArrayType::Complex128:
                    value.set("values", typedArray("Float64Array", array.values<double>().data(),
                                                   array.byteCount() / sizeof(double)));
                    break;
                }
                arrays.set(array.name, value);
            }
            result.set("arrays", arrays);
            auto texts = val::object();
            for (const auto& text : data->texts)
                texts.set(text.name, textArray(text.values()));
            result.set("texts", texts);
            return result;
        });
}

void addNamed(api::Results& target, const val& input)
{
    guarded(
        [&]
        {
            results::NamedResultOrigin origin;
            origin.name = option<std::string>(input, "name", "");
            origin.subcase = option<std::int32_t>(input, "subcase", 1);
            origin.analysisCode = option<std::int32_t>(input, "analysisCode", 0);
            origin.designCycle = option<std::int32_t>(input, "designCycle", -1);
            if (const auto attributes = input["attributes"]; present(attributes))
            {
                const auto keys = val::global("Object").call<val>("keys", attributes);
                for (unsigned i = 0, n = keys["length"].as<unsigned>(); i < n; ++i)
                {
                    const auto key = keys[i].as<std::string>();
                    origin.attributes.emplace_back(key, attributes[key].as<std::string>());
                }
            }
            std::vector<results::NamedArrayInput> arrays;
            std::vector<std::vector<double>> doubles;
            std::vector<std::vector<float>> singles;
            std::vector<std::vector<std::int32_t>> integers;
            const auto source = input["arrays"];
            const auto keys = val::global("Object").call<val>("keys", source);
            for (unsigned i = 0, n = keys["length"].as<unsigned>(); i < n; ++i)
            {
                const auto key = keys[i].as<std::string>();
                const auto value = source[key];
                results::NamedArrayInput array;
                array.name = key;
                const auto shape = emscripten::convertJSArrayToNumberVector<double>(value["shape"]);
                for (const double count : shape)
                {
                    if (!std::isfinite(count) || count < 0 || count > 9007199254740991.0 || std::floor(count) != count)
                        throw base::UsageError("named array extents must be non-negative safe integers");
                    array.shape.push_back(static_cast<std::size_t>(count));
                }
                const auto type = option<std::string>(value, "type", "float64");
                if (type == "int32")
                {
                    const auto values = emscripten::convertJSArrayToNumberVector<double>(value["values"]);
                    auto& buffer = integers.emplace_back();
                    for (const double number : values)
                    {
                        if (!std::isfinite(number) || std::floor(number) != number ||
                            number < std::numeric_limits<std::int32_t>::min() ||
                            number > std::numeric_limits<std::int32_t>::max())
                            throw base::UsageError("int32 named arrays require signed 32-bit integers");
                        buffer.push_back(static_cast<std::int32_t>(number));
                    }
                    array.type = results::ArrayType::Int32;
                    array.bytes = std::as_bytes(std::span(buffer));
                }
                else if (type == "float32")
                {
                    auto& buffer =
                        singles.emplace_back(emscripten::convertJSArrayToNumberVector<float>(value["values"]));
                    array.type = results::ArrayType::Float32;
                    array.bytes = std::as_bytes(std::span(buffer));
                }
                else if (type == "float64")
                {
                    auto& buffer =
                        doubles.emplace_back(emscripten::convertJSArrayToNumberVector<double>(value["values"]));
                    array.type = results::ArrayType::Float64;
                    array.bytes = std::as_bytes(std::span(buffer));
                }
                else
                    throw base::UsageError("named input arrays support int32, float32 and float64");
                arrays.push_back(std::move(array));
            }
            std::vector<results::NamedTextInput> texts;
            if (const auto sourceTexts = input["texts"]; present(sourceTexts))
            {
                const auto names = val::global("Object").call<val>("keys", sourceTexts);
                for (unsigned i = 0, n = names["length"].as<unsigned>(); i < n; ++i)
                {
                    const auto name = names[i].as<std::string>();
                    std::vector<std::string> values;
                    const auto entries = sourceTexts[name];
                    for (unsigned j = 0, count = entries["length"].as<unsigned>(); j < count; ++j)
                        values.push_back(entries[j].as<std::string>());
                    texts.push_back({name, std::move(values)});
                }
            }
            target.addNamed(input["kind"].as<std::string>(), origin, arrays, texts);
        });
}

val catalogue()
{
    auto array = val::array();
    for (const auto& kind : results::allNamedKinds())
    {
        auto item = val::object();
        item.set("kind", std::string(kind.kind));
        item.set("description", std::string(kind.description));
        auto fields = val::array();
        for (const auto& field : kind.arrays)
        {
            auto entry = val::object();
            entry.set("name", std::string(field.name));
            entry.set("type", std::string(field.type));
            entry.set("shape", std::string(field.shape));
            entry.set("text", field.text);
            entry.set("optional", field.optional);
            fields.call<void>("push", entry);
        }
        item.set("arrays", fields);
        array.call<void>("push", item);
    }
    return array;
}
} // namespace

void bindNamedResults(emscripten::class_<api::Results>& binding)
{
    binding.function("named", &named).function("readNamed", &readNamed).function("addNamed", &addNamed);
    emscripten::function("namedResultCatalogue", &catalogue);
}
} // namespace oor::wasm
