includes("wheel")
target("utils")
    set_kind("static")
    add_files("*.cpp")
    add_includedirs("$(projectdir)/include")
