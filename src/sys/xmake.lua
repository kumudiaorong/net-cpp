add_deps("xsl_log_ctl")

target("xsl_sys")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    on_package(function(package) end)
end
