# Be naughty, and set many things globally

# We only support Clang because otherwise ASan is pain to work with
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(ERROR "Only clang is supported")
endif()

# Force debug build
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# Set C++ 20
set(CMAKE_CXX_STANDARD 20 CACHE STRING "CXX version")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force use the mold linker for *much* faster iteration times
# It also has much better error messages.
add_link_options(-fuse-ld=mold)

# Disable bad ideas
add_compile_options(-fno-rtti -fno-exceptions)

# Enable maximal debug info
add_compile_options(-ggdb3)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Tell the compiler where to find LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})

# Tell the compiler to define LLVM macros
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Get all of LLVM's cmake stuff, so we can find things like FileCheck
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # prevent stupid "convenience" targets from being added
include(CTest)
enable_testing()

# Check the architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(SOURCE_DIR "reference-binaries/arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(SOURCE_DIR "reference-binaries/x86")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Define the binary files
set(BINARY_FILES ${SOURCE_DIR}/lexer ${SOURCE_DIR}/parser ${SOURCE_DIR}/semant)

# Copy reference binaries
file(COPY ${BINARY_FILES} DESTINATION ${CMAKE_BINARY_DIR})
# Brute force permission issues
file(CHMOD ${CMAKE_BINARY_DIR}/lexer FILE_PERMISSIONS WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ)
file(CHMOD ${CMAKE_BINARY_DIR}/parser FILE_PERMISSIONS WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ)
file(CHMOD ${CMAKE_BINARY_DIR}/semant FILE_PERMISSIONS WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ)

function(run_test)
    cmake_parse_arguments(
            RUN_TEST
            "FAIL;REFOUT"
            "FILE"
            ""
            ${ARGN}
    )
    get_filename_component(raw_dir ${RUN_TEST_FILE} DIRECTORY)
    get_filename_component(dir ${raw_dir} NAME)
    get_filename_component(file ${RUN_TEST_FILE} NAME_WLE)
    set(test "${dir}_${file}")
    set(test_pretty "${dir}/${file}")

    add_test(NAME "AST ${test_pretty}" COMMAND sh -c "./lexer ${RUN_TEST_FILE} | ./parser | ./semant > ${test}.ast" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("AST ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.ast"
    )

    add_test(NAME "No Crash ${test_pretty}" COMMAND "$<TARGET_FILE:cgen>" -d "${test}.ast" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("No Crash ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.no_crash"
            FIXTURES_REQUIRED "${test}.ast"
            REQUIRED_FILES "${test}.ast"
            LABELS "NO_CRASH"
            TIMEOUT 5
    )

    add_test (NAME "Gen ${test_pretty}" COMMAND sh -c "$<TARGET_FILE:cgen> -d ${test}.ast > ${test}.ll" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("Gen ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.gen"
            FIXTURES_REQUIRED "${test}.no_crash"
            REQUIRED_FILES "${test}.ast"
            LABELS "GEN"
    )

    add_test(NAME "Verify ${test_pretty}" COMMAND opt -S -p verify "${test}.ll" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("Verify ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.valid"
            FIXTURES_REQUIRED "${test}.gen"
            REQUIRED_FILES "${test}.ll"
            LABELS "VALID"
    )

    add_test(NAME "Optimize ${test_pretty}" COMMAND opt --O3 -S "${test}.ll" -o "${test}.o3.ll" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("Optimize ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.o3"
            FIXTURES_REQUIRED "${test}.valid"
            REQUIRED_FILES "${test}.ll"
            LABELS "OPT"
    )

    add_test(NAME "Compile ${test_pretty}" COMMAND clang -ggdb3 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common -fuse-ld=mold -o "${test}.bin" "${test}.o3.ll" "$<TARGET_FILE:coolrt>" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("Compile ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.compile"
            FIXTURES_REQUIRED "${test}.o3"
            REQUIRED_FILES "${test}.o3.ll"
            LABELS "COMPILE"
    )

    add_test(NAME "Run ${test_pretty}" COMMAND "${test}.bin" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("Run ${test_pretty}" PROPERTIES
            FIXTURES_SETUP "${test}.run"
            FIXTURES_REQUIRED "${test}.compile"
            REQUIRED_FILES "${test}.bin"
            LABELS "RUN"
            TIMEOUT 1
    )

    if (${RUN_TEST_FAIL})
        message(STATUS "${test_pretty} is supposed to fail")
        set_tests_properties("Run ${test_pretty}" PROPERTIES WILL_FAIL True)
    endif()

    if (${RUN_TEST_REFOUT})
        message(STATUS "Validating output of ${test_pretty}")
        add_test(NAME "Output ${test_pretty}" COMMAND sh -c "./${test}.bin > ${test}.out" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties("Run ${test_pretty}" PROPERTIES
                FIXTURES_SETUP "${test}.out"
                FIXTURES_REQUIRED "${test}.run"
                REQUIRED_FILES "${test}.bin"
                LABELS "OUT"
        )

        add_test(NAME "Check ${test_pretty}" COMMAND diff -u -E -Z -b -B "${test}.out" "${raw_dir}/${file}.refout" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties("Check ${test_pretty}" PROPERTIES
                FIXTURES_REQUIRED "${test}.out"
                REQUIRED_FILES "${test}.out"
                LABELS "CHECK"
                RUN_SERIAL True
        )
    endif()

endfunction()
