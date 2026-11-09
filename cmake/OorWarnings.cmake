# Compiler warnings applied to every library target through oor_enable_warnings().
#
# Spec: -Wall -Wextra -Wpedantic -Werror. The code is written for the GCC 12 subset of C++20 even when a newer GCC
# builds it.

function(oor_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(OOR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# MinGW: link the C++ and GCC runtimes statically so that a built extension or test needs no MinGW DLL.
function(oor_link_static_runtime target)
    if(MINGW)
        target_link_options(${target} PRIVATE -static-libgcc -static-libstdc++)
    endif()
endfunction()
