#pragma once
#include "oor/api/CardEdits.hpp"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <exception>
#include <vector>

namespace oor::wasm
{
using emscripten::val;
// Runs `work`; a C++ exception is thrown again as a JavaScript Error.
template <typename Work>
auto guarded(Work work) -> decltype(work())
{
    try
    {
        return work();
    }
    catch (const std::exception& error)
    {
        val::global("Error").new_(std::string(error.what())).throw_();
    }
    catch (...)
    {
        val::global("Error").new_(std::string("unknown C++ error")).throw_();
    }
    __builtin_unreachable();
}

// A member of a JavaScript options object, or `fallback` when the object or the member is missing.
template <typename T>
T option(const val& options, const char* name, T fallback)
{
    if (options.isUndefined() || options.isNull())
        return fallback;
    const val member = options[name];
    if (member.isUndefined() || member.isNull())
        return fallback;
    return member.as<T>();
}

// Copies of C++ vectors as JavaScript typed arrays and arrays.
template <typename T>
val typedArray(const char* type, const T* data, std::size_t count)
{
    return val::global(type).new_(emscripten::typed_memory_view(count, data));
}
inline val int32Array(const std::vector<std::int32_t>& values)
{
    return typedArray("Int32Array", values.data(), values.size());
}
inline val float64Array(const std::vector<double>& values)
{
    return typedArray("Float64Array", values.data(), values.size());
}
inline val textArray(const std::vector<std::string>& texts)
{
    val array = val::array();
    for (const std::string& text : texts)
        array.call<void>("push", text);
    return array;
}

template <typename T>
std::vector<T> vectorOf(const val& array)
{
    if (array.isUndefined() || array.isNull())
        return {};
    return emscripten::vecFromJSArray<T>(array);
}

inline val diagnosticsArray(const std::vector<api::DiagnosticText>& diagnostics)
{
    val array = val::array();
    for (const api::DiagnosticText& diagnostic : diagnostics)
    {
        val item = val::object();
        item.set("severity", diagnostic.severity);
        item.set("code", diagnostic.code);
        item.set("message", diagnostic.message);
        item.set("location", diagnostic.location);
        array.call<void>("push", item);
    }
    return array;
}

inline val fieldValue(const api::FieldValue& value)
{
    switch (value.kind)
    {
    case api::FieldValue::Kind::Number:
        return val(value.number);
    case api::FieldValue::Kind::Numbers:
        return float64Array(value.numbers);
    case api::FieldValue::Kind::Text:
        return val(value.text);
    case api::FieldValue::Kind::Texts:
        return textArray(value.texts);
    case api::FieldValue::Kind::Missing:
        break;
    }
    return val::null();
}

inline val cardObject(const api::CardView& card)
{
    val object = val::object();
    object.set("type", card.type);
    object.set("id", card.id);
    object.set("key", card.key);
    val values = val::object();
    for (const auto& [name, value] : card.values)
        values.set(name, fieldValue(value));
    object.set("values", values);
    object.set("fields", textArray(card.fields));
    object.set("text", card.text);
    object.set("comment", card.comment);
    return object;
}
} // namespace oor::wasm
