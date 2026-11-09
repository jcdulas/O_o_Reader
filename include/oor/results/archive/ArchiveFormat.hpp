#pragma once

#include <string_view>

// The names of the HDF5 results archive: the file format this library writes for results and custom outputs.
// docs/archive-format.md describes it in full; this header is the single place where its names are spelled.
//
// In short (version 3):
//
//   /                       attributes: format, format_version, producer, created_at, title, subtitle, label
//   /index/tables           one row per result table (compound): where its group is and what it holds
//   /index/named            one row per named result (compound)
//   /data/tables/<n>/       a result table: values (steps, rows, components[, 2]) and the labels of its axes
//   /data/named/<n>/        a named result: its arrays and text labels
//
// Design choices, for a web viewer:
// - the index is one small dataset: opening an archive reads it once, without walking thousands of groups;
// - the values of a table are one dataset of shape (steps, rows, components), chunked by whole steps and compressed
//   (shuffle + deflate), so that showing one step of one table reads and inflates only that step;
// - the precision of the source is kept (float32 stays float32): an archive is about half the size of float64 data;
// - every name is plain English and every dataset is a plain HDF5 type, so any HDF5 tool can read an archive.
namespace oor::results::archive
{

inline constexpr std::string_view FORMAT_NAME = "oor-results-archive";
inline constexpr std::string_view FORMAT_VERSION = "3";

// Attributes of the root group (fixed-length UTF-8 strings).
inline constexpr std::string_view ATTRIBUTE_FORMAT = "format";
inline constexpr std::string_view ATTRIBUTE_FORMAT_VERSION = "format_version";
inline constexpr std::string_view ATTRIBUTE_PRODUCER = "producer";
inline constexpr std::string_view ATTRIBUTE_CREATED_AT = "created_at";
inline constexpr std::string_view ATTRIBUTE_TITLE = "title";
inline constexpr std::string_view ATTRIBUTE_SUBTITLE = "subtitle";
inline constexpr std::string_view ATTRIBUTE_LABEL = "label";

// The index datasets and the groups of the data.
inline constexpr std::string_view TABLE_INDEX = "/index/tables";
inline constexpr std::string_view NAMED_INDEX = "/index/named";
inline constexpr std::string_view TABLE_GROUPS = "/data/tables";
inline constexpr std::string_view NAMED_GROUPS = "/data/named";
// The steps of every table, one after the other in the order of /index/tables (the columns step_offset and step_count
// of the index say where the steps of a table are): listing an archive reads them once instead of one small dataset a
// table. NaN where a table has no such value; the columns has_eigenvalues, has_eigenvalues_imag and has_cycles say
// which tables have them.
inline constexpr std::string_view INDEX_STEPS = "/index/steps";
inline constexpr std::string_view INDEX_EIGENVALUES = "/index/eigenvalues";
inline constexpr std::string_view INDEX_EIGENVALUES_IMAG = "/index/eigenvalues_imag";
inline constexpr std::string_view INDEX_CYCLES = "/index/cycles";

// The datasets of a table group.
inline constexpr std::string_view VALUES = "values";
inline constexpr std::string_view COMPONENTS = "components";
inline constexpr std::string_view STEPS = "steps";
inline constexpr std::string_view EIGENVALUES = "eigenvalues";
inline constexpr std::string_view EIGENVALUES_IMAG = "eigenvalues_imag";
inline constexpr std::string_view CYCLES = "cycles";
inline constexpr std::string_view IDS = "ids";
inline constexpr std::string_view POINT_TYPES = "point_types";
inline constexpr std::string_view NODE_IDS = "node_ids";
inline constexpr std::string_view ELEMENT_IDS = "element_ids";
inline constexpr std::string_view LAYERS = "layers";
inline constexpr std::string_view POSITIONS = "positions";
// Text labels: TEXT_NAMES lists them; texts/<i>/dictionary and texts/<i>/codes hold the i-th.
inline constexpr std::string_view TEXT_NAMES = "text_names";
inline constexpr std::string_view TEXTS = "texts";
inline constexpr std::string_view DICTIONARY = "dictionary";
inline constexpr std::string_view CODES = "codes";

// The datasets of a named result group: ARRAY_NAMES, ARRAY_DESCRIPTIONS, ARRAY_TYPES ("int32", "float64",
// "complex64", ...) and ARRAY_SHAPES ("3,4"; "" for a single number) list the arrays, arrays/<i> holds the i-th (a
// complex array has a last dimension of 2); ATTRIBUTES is a (n, 2) table of (key, value) texts.
inline constexpr std::string_view ARRAY_NAMES = "array_names";
inline constexpr std::string_view ARRAY_TYPES = "array_types";
inline constexpr std::string_view ARRAY_SHAPES = "array_shapes";
inline constexpr std::string_view ARRAY_DESCRIPTIONS = "array_descriptions";
inline constexpr std::string_view ARRAYS = "arrays";
inline constexpr std::string_view ATTRIBUTES = "attributes";

} // namespace oor::results::archive
