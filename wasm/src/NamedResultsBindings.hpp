#pragma once
#include "oor/api/Results.hpp"

#include <emscripten/bind.h>

namespace oor::wasm
{
void bindNamedResults(emscripten::class_<api::Results>& binding);
}
