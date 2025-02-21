add_rules("mode.release", "mode.debug")


add_repositories("zeromake https://github.com/zeromake/xrepo.git")

add_requires("stb", "handmade_math", "sokol-shdc", "sokol")

add_requires("imgui 1.x", {configs={backend="none", freetype=true}})

if is_plat("windows") then
    add_cflags("/TC", {tools = {"clang_cl", "cl"}})
    add_cxxflags("/EHsc", {tools = {"clang_cl", "cl"}})
    add_defines("UNICODE", "_UNICODE")
    add_defines("SOKOL_WIN32_FORCE_MAIN")
end

if is_plat("mingw") then
    add_ldflags("-static")
end

add_includedirs("libs/sokol")
set_encodings("utf-8")
set_languages("c99")

if is_plat("windows", "mingw") then
	-- add_defines("SOKOL_GLCORE")
    add_defines("SOKOL_D3D11")
elseif is_plat("macosx") then
    add_defines("SOKOL_METAL")
elseif is_plat("wasm") then 
    add_defines("SOKOL_GLES3")
else
	add_defines("SOKOL_GLCORE")
end

local function split(input, delimiter)
    if (delimiter == "") then return false end
    local pos, arr = 0, {}
    for st, sp in function() return string.find(input, delimiter, pos, true) end do
        table.insert(arr, string.sub(input, pos, st - 1))
        pos = sp + 1
    end
    table.insert(arr, string.sub(input, pos))
    return arr
end

target("shader")
    set_kind("object")
    add_packages("sokol-shdc")
    add_rules("@sokol-shdc/shader")
    for _, f in ipairs(os.files("src/**/*.glsl")) do
        add_files(f)
    end
target_end()

target("sokol_wrapper")
    set_kind("static")
    add_packages("sokol")
    if is_plat("macosx") then
        add_files("libs/sokol/sokol.m")
        add_frameworks(
            "Appkit",
            "QuartzCore",
            "CoreGraphics",
            "Metal",
            "Metalkit"
        )
    else
        add_files("libs/sokol/sokol.c")
    end
target_end()

target("dbgui")
    set_kind("static")
    add_packages("sokol", "imgui")
    set_languages("c++17")
    add_files("libs/dbgui/dbgui.cc")

for _, dir in ipairs(os.filedirs("src/*")) do
    if os.isdir(dir) and path.basename(dir) ~= "data" then
        local includedir = path.join("$(buildir)/sokol_shader", path.basename(dir))
        local arr = split(path.basename(dir), '-')
        local targetname = arr[1].."-"..arr[2]
        for _, f in ipairs(os.files(dir.."/*.c")) do
            target(targetname.."-"..(split(path.basename(f), '-')[1]))
                add_packages("stb", "sokol", "handmade_math")
                add_files(f)
                if is_plat("windows", "mingw") then
		            add_files("src/resource.rc")
                elseif is_plat("wasm") then
                    add_ldflags("-sMAX_WEBGL_VERSION=2")
                    set_extension(".js")
                    add_ldflags("-sSTACK_SIZE=512KB")
                    add_ldflags("-sTOTAL_MEMORY="..(1024*1024*32))
                    add_ldflags("-sALLOW_MEMORY_GROWTH=1")
                    add_ldflags("-sMALLOC=\"emmalloc\"")
                end
                add_includedirs(includedir)
                add_includedirs("libs")
                add_deps("sokol_wrapper", "shader", "dbgui")
                set_rundir("src/data")
                add_defines("USE_DBG_UI")
            target_end()
        end
    end
end
