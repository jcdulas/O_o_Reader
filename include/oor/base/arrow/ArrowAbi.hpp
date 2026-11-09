#pragma once

#include <cstdint>

// The Arrow C Data Interface (https://arrow.apache.org/docs/format/CDataInterface.html): two plain C structs that
// any Arrow consumer (pandas, polars, pyarrow) can read without copy. The definitions below are the ones of the
// specification, verbatim; this project depends on the format, not on the Arrow library.
//
// NOLINTBEGIN(readability-identifier-naming)
#ifndef ARROW_C_DATA_INTERFACE
#define ARROW_C_DATA_INTERFACE

#define ARROW_FLAG_DICTIONARY_ORDERED 1
#define ARROW_FLAG_NULLABLE 2
#define ARROW_FLAG_MAP_KEYS_SORTED 4

extern "C"
{
    struct ArrowSchema
    {
        const char* format;
        const char* name;
        const char* metadata;
        std::int64_t flags;
        std::int64_t n_children;
        struct ArrowSchema** children;
        struct ArrowSchema* dictionary;
        void (*release)(struct ArrowSchema*);
        void* private_data;
    };

    struct ArrowArray
    {
        std::int64_t length;
        std::int64_t null_count;
        std::int64_t offset;
        std::int64_t n_buffers;
        std::int64_t n_children;
        const void** buffers;
        struct ArrowArray** children;
        struct ArrowArray* dictionary;
        void (*release)(struct ArrowArray*);
        void* private_data;
    };
}

#endif // ARROW_C_DATA_INTERFACE

// The Arrow C stream interface (https://arrow.apache.org/docs/format/CStreamInterface.html): a source of arrays that
// share one schema, read one batch at a time. Also verbatim from the specification.
#ifndef ARROW_C_STREAM_INTERFACE
#define ARROW_C_STREAM_INTERFACE

extern "C"
{
    struct ArrowArrayStream
    {
        int (*get_schema)(struct ArrowArrayStream*, struct ArrowSchema* out);
        int (*get_next)(struct ArrowArrayStream*, struct ArrowArray* out);
        const char* (*get_last_error)(struct ArrowArrayStream*);
        void (*release)(struct ArrowArrayStream*);
        void* private_data;
    };
}

#endif // ARROW_C_STREAM_INTERFACE
// NOLINTEND(readability-identifier-naming)
