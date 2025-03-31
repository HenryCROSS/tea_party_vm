add_rules("mode.debug", "mode.release")

set_toolchains("clang")

set_languages("c++23")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

target("tea_party_vm")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/scanner/*.cpp")
    add_files("src/parser/*.cpp")
    add_files("src/repl/*.cpp")
    add_files("src/vm/*.cpp")
    add_includedirs("src")

set_optimize("faster")
