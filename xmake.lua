add_rules("mode.release", "mode.debug")

package("sokol-shdc")
    set_kind("binary")
    set_license("MIT")
    set_homepage("https://github.com/floooh/sokol-tools")
    local hash = "74c6bb111d51121fa288f1787c9df5a56545bbfd"
    local url_prefix = "https://github.com/floooh/sokol-tools-bin/raw/"..hash.."/bin/"
    local url_suffix = "win32/sokol-shdc.exe"
    if is_host("macosx") then
        if os.arch() == "arm64" then
            url_suffix = "osx_arm64/sokol-shdc"
            add_versions("latest", "ae4064824ea079d10cdc5e0aef8e3a11308ef4acc0b64add651194620f5f7037")
        else
            url_suffix = "osx/sokol-shdc"
            add_versions("latest", "b8f263b9e08f6e62bd6f0c061922243c8b00cc2a02770f47185d65bf8f2eddfc")
        end
    elseif is_host("linux") then 
        url_suffix = "linux/sokol-shdc"
        add_versions("latest", "fffc93a057ae27fbdf98822a87a7419cdcda3163a3842b65da2a14b886cc15a5")
    else
        add_versions("latest", "ff0faf3547996078b037816387b3c84874b447b29f0f1a09088b1c06822c91bd")
    end
    set_urls(url_prefix..url_suffix)
    on_install(function (package) 
        local bin = package:installdir("bin")
        if is_host("windows") then
            os.cp("../sokol-shdc.exe", bin)
        else
            os.run("chmod 755 ../sokol-shdc")
            os.cp("../sokol-shdc", bin)
        end
    end)
    on_test(function (package)
        os.vrun("sokol-shdc --help")
    end)
package_end()

rule("sokol_shader")
    set_extensions(".glsl")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        import("lib.detect.find_tool")
        local sokolshdc = find_tool("sokol-shdc", {check = "--help"})
        local targetfile = path.relative(sourcefile, "src")
        batchcmds:mkdir(path.join("$(buildir)/sokol_shader", path.directory(targetfile)))
        local targetfile = vformat(path.join("$(buildir)/sokol_shader", targetfile..".h"))
        batchcmds:vrunv(sokolshdc.program, {
            "--ifdef",
            "-l",
            "hlsl5:glsl330:glsl300es:metal_macos:metal_ios",
            "--input",
            sourcefile,
            "--output",
            targetfile,
        })
        batchcmds:show_progress(opt.progress, "${color.build.object}glsl %s", sourcefile)
        batchcmds:add_depfiles(sourcefile)
    end)
rule_end()

add_rules("sokol_shader")

add_repositories("zeromake https://github.com/zeromake/xrepo.git")

add_requires("stb", "sokol =2023.10.27-alpha", "handmade_math", "sokol-shdc")

add_requires("imgui 1.x", {configs={backend="none"}})

if is_plat("windows") then
    add_defines("SOKOL_WIN32_FORCE_MAIN")
    add_cxflags("/utf-8")
elseif is_plat("mingw") then
    add_ldflags("-static")
end
add_languages("c++20")

add_includedirs("libs/sokol")

if is_plat("windows", "mingw") then
	-- add_defines("SOKOL_GLCORE33")
    add_defines("SOKOL_D3D11")
elseif is_plat("macosx") then
    add_defines("SOKOL_METAL")
elseif is_plat("wasm") then 
    add_defines("SOKOL_GLES3")
else
	add_defines("SOKOL_GLCORE33")
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
