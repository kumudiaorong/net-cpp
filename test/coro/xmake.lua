add_packages("gtest","fmt")
add_includedirs("$(projectdir)/test/include", { public = true })

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_tests(name)
        -- add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE",{public = true})
        on_package(function(package) end)
end
