#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "support/ModelFileRoundTrip.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

namespace oor::testing
{

// Reads a model file given as text (docs/model-format.md): records with or without section lines; records before the
// first section line are cards of [model]. The file is written in one scratch folder per process, reused by every
// call: making and removing a folder for each read costs more than the read itself. An Include record names a file of
// that folder.
inline model::Model readModelText(const std::string& text, bool strict = false)
{
    static const TestFiles FILES;
    const std::filesystem::path path = FILES.write("model.oos", text);
    model::ModelReadOptions options;
    options.strict = strict;
    model::Model model = model::ModelFileReader::read(path, options);
    logModelFileRoundTrip(model, text);
    return model;
}

// Writes a model as the text of one model file.
inline std::string writeModelText(const model::Model& model, bool omitDefaults = true)
{
    model::ModelFileWriteOptions options;
    options.omitDefaults = omitDefaults;
    return model::ModelFileWriter::toString(model, options);
}

// Writes the model as a model file and reads that again strictly: the same card tables, and writing the model read
// back gives the same text (a fixed point).
inline void requireModelFileRoundTrip(const model::Model& model, bool omitDefaults = true)
{
    const std::string text = writeModelText(model, omitDefaults);
    INFO(text);
    const model::Model again = readModelText(text, true);
    REQUIRE(cardTablesText(again) == cardTablesText(model));
    REQUIRE(writeModelText(again, omitDefaults) == text);
}

} // namespace oor::testing
