# One test executable per module; every Catch2 test case is a separate ctest test, so the 1-second timeout applies
# to each of them. Test sources are found by pattern: a new *Test.cpp needs no edit here.
include(Catch)

set(OOR_TEST_MODULES base io deck model results analysis query api)

# Each discovered test runs in its own process and has a one-second wall-clock limit.
set(OOR_TEST_TIMEOUT 1 CACHE STRING "Time limit of each C++ test, in seconds")

foreach(module IN LISTS OOR_TEST_MODULES)
    file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${module}/*Test.cpp")
    if(test_sources)
        add_executable(oor_${module}_tests ${test_sources})
        target_link_libraries(oor_${module}_tests PRIVATE oor::${module} Catch2::Catch2WithMain)
        target_include_directories(oor_${module}_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        # where the validation models are (tests/cpp/model/ValidationModelsTest.cpp)
        target_compile_definitions(oor_${module}_tests PRIVATE
            OOR_VALIDATION_DIR="${PROJECT_SOURCE_DIR}/validation/models")
        oor_enable_warnings(oor_${module}_tests)
        oor_link_static_runtime(oor_${module}_tests)
        catch_discover_tests(oor_${module}_tests
            DISCOVERY_MODE PRE_TEST
            PROPERTIES TIMEOUT ${OOR_TEST_TIMEOUT})
    endif()
endforeach()
