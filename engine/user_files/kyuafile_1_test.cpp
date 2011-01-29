// Copyright 2011 Google Inc.
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

// TODO(jmmv): These tests ought to be written in Lua.  Rewrite when we have a
// Lua binding.

#include <fstream>

#include <atf-c++.hpp>

#include "engine/user_files/common.hpp"
#include "utils/fs/operations.hpp"
#include "utils/fs/path.hpp"
#include "utils/lua/operations.hpp"
#include "utils/lua/wrap.hpp"

namespace fs = utils::fs;
namespace lua = utils::lua;
namespace user_files = engine::user_files;


ATF_TEST_CASE_WITHOUT_HEAD(empty);
ATF_TEST_CASE_BODY(empty)
{
    std::ofstream output("test.lua");
    ATF_REQUIRE(output);
    output << "syntax('kyuafile', 1)\n";
    output.close();

    lua::state state;
    user_files::do_user_file(state, fs::path("test.lua"));
    lua::do_string(state, "assert(table.maxn(kyuafile.TEST_PROGRAMS) == 0)");
}


ATF_TEST_CASE_WITHOUT_HEAD(some_test_programs);
ATF_TEST_CASE_BODY(some_test_programs)
{
    std::ofstream output("test.lua");
    ATF_REQUIRE(output);
    output << "syntax('kyuafile', 1)\n";
    output << "AtfTestProgram {name='test1'}\n";
    output << "AtfTestProgram {name='test3'}\n";
    output << "AtfTestProgram {name='test2'}\n";
    output << "AtfTestProgram {name='/a/b/foo'}\n";
    output.close();

    lua::state state;
    user_files::do_user_file(state, fs::path("test.lua"));
    lua::do_string(state, "assert(table.maxn(kyuafile.TEST_PROGRAMS) == 4)");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[1] == 'test1')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[2] == 'test3')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[3] == 'test2')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[4] == '/a/b/foo')");
}


ATF_TEST_CASE_WITHOUT_HEAD(include_nested);
ATF_TEST_CASE_BODY(include_nested)
{
    {
        std::ofstream output("root.lua");
        ATF_REQUIRE(output);
        output << "syntax('kyuafile', 1)\n";
        output << "AtfTestProgram {name='test1'}\n";
        output << "AtfTestProgram {name='test2'}\n";
        output << "include('dir/test.lua')\n";
        output.close();
    }

    {
        fs::mkdir(fs::path("dir"), 0755);
        std::ofstream output("dir/test.lua");
        ATF_REQUIRE(output);
        output << "syntax('kyuafile', 1)\n";
        output << "AtfTestProgram {name='test1'}\n";
        output << "include('foo/test.lua')\n";
        output.close();
    }

    {
        fs::mkdir(fs::path("dir/foo"), 0755);
        std::ofstream output("dir/foo/test.lua");
        ATF_REQUIRE(output);
        output << "syntax('kyuafile', 1)\n";
        output << "AtfTestProgram {name='bar'}\n";
        output << "AtfTestProgram {name='baz'}\n";
        output << "AtfTestProgram {name='/a/b/c'}\n";
        output.close();
    }

    lua::state state;
    user_files::do_user_file(state, fs::path("root.lua"));
    lua::do_string(state, "assert(table.maxn(kyuafile.TEST_PROGRAMS) == 6)");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[1] == 'test1')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[2] == 'test2')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[3] == 'dir/test1')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[4] == 'dir/foo/bar')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[5] == 'dir/foo/baz')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[6] == '/a/b/c')");
}


ATF_TEST_CASE_WITHOUT_HEAD(include_same_dir);
ATF_TEST_CASE_BODY(include_same_dir)
{
    {
        std::ofstream output("main.lua");
        ATF_REQUIRE(output);
        output << "syntax('kyuafile', 1)\n";
        output << "AtfTestProgram {name='test1'}\n";
        output << "AtfTestProgram {name='test2'}\n";
        output << "include('second.lua')\n";
        output.close();
    }

    {
        std::ofstream output("second.lua");
        ATF_REQUIRE(output);
        output << "syntax('kyuafile', 1)\n";
        output << "AtfTestProgram {name='test12'}\n";
        output.close();
    }

    lua::state state;
    user_files::do_user_file(state, fs::path("main.lua"));
    lua::do_string(state, "assert(table.maxn(kyuafile.TEST_PROGRAMS) == 3)");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[1] == 'test1')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[2] == 'test2')");
    lua::do_string(state, "assert(kyuafile.TEST_PROGRAMS[3] == 'test12')");
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, empty);
    ATF_ADD_TEST_CASE(tcs, some_test_programs);
    ATF_ADD_TEST_CASE(tcs, include_nested);
    ATF_ADD_TEST_CASE(tcs, include_same_dir);
}