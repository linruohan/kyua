// Copyright 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

extern "C" {
#include <sys/stat.h>

#include <unistd.h>
}

#include <iostream>
#include <sstream>

#include <atf-c++.hpp>

#include "engine/exceptions.hpp"
#include "engine/test_program.hpp"
#include "utils/env.hpp"
#include "utils/fs/operations.hpp"
#include "utils/fs/path.hpp"

namespace detail = engine::detail;
namespace fs = utils::fs;


namespace {


/// Gets the path to the atf-specific helpers.
///
/// \param tc A pointer to the currently running test case.
///
/// \return The path to the helpers binary.
static fs::path
atf_helpers(const atf::tests::tc* test_case)
{
    return fs::path(test_case->get_config_var("srcdir")) /
        "test_program_atf_helpers";
}


/// Gets the path to the plain (generic binary, no framework) helpers.
///
/// \param tc A pointer to the currently running test case.
///
/// \return The path to the helpers binary.
static fs::path
plain_helpers(const atf::tests::tc* test_case)
{
    return fs::path(test_case->get_config_var("srcdir")) /
        "test_program_plain_helpers";
}


/// Instantiates a test case.
///
/// \param path The test program.
/// \param name The test case name.
/// \param props The raw properties to pass to the test case.
///
/// \return The new test case.
static engine::test_case
make_test_case(const char* path, const char* name,
               const engine::properties_map& props = engine::properties_map())
{
    const engine::test_case_id id(fs::path(path), name);
    return engine::test_case::from_properties(id, props);
}


}  // anonymous namespace


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__empty);
ATF_TEST_CASE_BODY(parse_test_cases__empty)
{
    const std::string text = "";
    std::istringstream input(text);
    ATF_REQUIRE_THROW_RE(engine::format_error, "expecting Content-Type",
        detail::parse_test_cases(fs::path("test-program"), input));
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__invalid_header);
ATF_TEST_CASE_BODY(parse_test_cases__invalid_header)
{
    {
        const std::string text =
            "Content-Type: application/X-atf-tp; version=\"1\"\n";
        std::istringstream input(text);
        ATF_REQUIRE_THROW_RE(engine::format_error, "expecting.*blank line",
            detail::parse_test_cases(fs::path("test-program"), input));
    }

    {
        const std::string text =
            "Content-Type: application/X-atf-tp; version=\"1\"\nfoo\n";
        std::istringstream input(text);
        ATF_REQUIRE_THROW_RE(engine::format_error, "expecting.*blank line",
            detail::parse_test_cases(fs::path("test-program"), input));
    }

    {
        const std::string text =
            "Content-Type: application/X-atf-tp; version=\"2\"\n\n";
        std::istringstream input(text);
        ATF_REQUIRE_THROW_RE(engine::format_error, "expecting Content-Type",
            detail::parse_test_cases(fs::path("test-program"), input));
    }
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__no_test_cases);
ATF_TEST_CASE_BODY(parse_test_cases__no_test_cases)
{
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n\n";
    std::istringstream input(text);
    ATF_REQUIRE_THROW_RE(engine::format_error, "No test cases",
        detail::parse_test_cases(fs::path("test-program"), input));
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__one_test_case_simple);
ATF_TEST_CASE_BODY(parse_test_cases__one_test_case_simple)
{
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test-case\n";
    std::istringstream input(text);
    const engine::test_cases_vector tests = detail::parse_test_cases(
        fs::path("test-program"), input);

    const engine::test_case test1 = make_test_case("test-program", "test-case");

    ATF_REQUIRE_EQ(1, tests.size());
    ATF_REQUIRE(test1 == tests[0]);
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__one_test_case_complex);
ATF_TEST_CASE_BODY(parse_test_cases__one_test_case_complex)
{
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: first\n"
        "descr: This is the description\n"
        "timeout: 500\n";
    std::istringstream input(text);
    const engine::test_cases_vector tests = detail::parse_test_cases(
        fs::path("test-program"), input);

    engine::properties_map props1;
    props1["descr"] = "This is the description";
    props1["timeout"] = "500";
    const engine::test_case test1 = make_test_case("test-program", "first",
                                                   props1);

    ATF_REQUIRE_EQ(1, tests.size());
    ATF_REQUIRE(test1 == tests[0]);
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__one_test_case_invalid_syntax);
ATF_TEST_CASE_BODY(parse_test_cases__one_test_case_invalid_syntax)
{
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n\n"
        "descr: This is the description\n"
        "ident: first\n";
    std::istringstream input(text);
    ATF_REQUIRE_THROW_RE(engine::format_error, "preceeded.*identifier",
        detail::parse_test_cases(fs::path("test-program"), input));
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__one_test_case_invalid_properties);
ATF_TEST_CASE_BODY(parse_test_cases__one_test_case_invalid_properties)
{
    // Inject a single invalid property that makes test_case::from_properties()
    // raise a particular error message so that we can validate that such
    // function was called.  We do intensive testing separately, so it is not
    // necessary to redo it here.
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n\n"
        "ident: first\n"
        "require.progs: bin/ls\n";
    std::istringstream input(text);
    ATF_REQUIRE_THROW_RE(engine::format_error, "Relative path 'bin/ls'",
        detail::parse_test_cases(fs::path("test-program"), input));
}


ATF_TEST_CASE_WITHOUT_HEAD(parse_test_cases__many_test_cases);
ATF_TEST_CASE_BODY(parse_test_cases__many_test_cases)
{
    const std::string text =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: first\n"
        "descr: This is the description\n"
        "\n"
        "ident: second\n"
        "timeout: 500\n"
        "descr: Some text\n"
        "\n"
        "ident: third\n";
    std::istringstream input(text);
    const engine::test_cases_vector tests = detail::parse_test_cases(
        fs::path("test-program"), input);

    engine::properties_map props1;
    props1["descr"] = "This is the description";
    const engine::test_case test1 = make_test_case("test-program", "first",
                                                   props1);

    engine::properties_map props2;
    props2["descr"] = "Some text";
    props2["timeout"] = "500";
    const engine::test_case test2 = make_test_case("test-program", "second",
                                                   props2);

    engine::properties_map props3;
    const engine::test_case test3 = make_test_case("test-program", "third",
                                                   props3);

    ATF_REQUIRE_EQ(3, tests.size());
    ATF_REQUIRE(test1 == tests[0]);
    ATF_REQUIRE(test2 == tests[1]);
    ATF_REQUIRE(test3 == tests[2]);
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__missing_test_program);
ATF_TEST_CASE_BODY(load_test_cases__missing_test_program)
{
    ATF_REQUIRE_THROW(engine::error, engine::load_test_cases(fs::path(
        "/non-existent")));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__abort);
ATF_TEST_CASE_BODY(load_test_cases__abort)
{
    utils::setenv("HELPER", "abort_test_cases_list");
    ATF_REQUIRE_THROW_RE(engine::error, "test program failed",
                         engine::load_test_cases(plain_helpers(this)));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__empty);
ATF_TEST_CASE_BODY(load_test_cases__empty)
{
    utils::setenv("HELPER", "empty_test_cases_list");
    ATF_REQUIRE_THROW_RE(engine::error, "Invalid header",
                         engine::load_test_cases(plain_helpers(this)));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__zero_test_cases);
ATF_TEST_CASE_BODY(load_test_cases__zero_test_cases)
{
    utils::setenv("HELPER", "zero_test_cases");
    ATF_REQUIRE_THROW_RE(engine::error, "No test cases",
                         engine::load_test_cases(plain_helpers(this)));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__absolute_path);
ATF_TEST_CASE_BODY(load_test_cases__absolute_path)
{
    const engine::test_cases_vector test_cases =
        engine::load_test_cases(atf_helpers(this));
    ATF_REQUIRE_EQ(3, test_cases.size());
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__relative_path);
ATF_TEST_CASE_BODY(load_test_cases__relative_path)
{
    ATF_REQUIRE(::mkdir("dir", 0755) != -1);
    ATF_REQUIRE(::symlink(atf_helpers(this).c_str(),
                          "dir/test_program_atf_helpers") != -1);
    const engine::test_cases_vector test_cases =
        engine::load_test_cases(fs::path("dir/test_program_atf_helpers"));
    ATF_REQUIRE_EQ(3, test_cases.size());
}


ATF_TEST_CASE_WITHOUT_HEAD(load_test_cases__basename_only);
ATF_TEST_CASE_BODY(load_test_cases__basename_only)
{
    ATF_REQUIRE(::symlink(atf_helpers(this).c_str(),
                          "test_program_atf_helpers") != -1);
    const engine::test_cases_vector test_cases =
        engine::load_test_cases(fs::path("test_program_atf_helpers"));
    ATF_REQUIRE_EQ(3, test_cases.size());
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__empty);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__invalid_header);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__no_test_cases);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__one_test_case_simple);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__one_test_case_complex);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__one_test_case_invalid_syntax);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__one_test_case_invalid_properties);
    ATF_ADD_TEST_CASE(tcs, parse_test_cases__many_test_cases);

    ATF_ADD_TEST_CASE(tcs, load_test_cases__missing_test_program);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__abort);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__empty);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__zero_test_cases);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__absolute_path);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__relative_path);
    ATF_ADD_TEST_CASE(tcs, load_test_cases__basename_only);
}