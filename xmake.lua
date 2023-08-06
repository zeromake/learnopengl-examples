add_rules("mode.release", "mode.debug")

rule("sokol_shader")
    set_extensions(".glsl")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        local targetfile = path.relative(sourcefile, "src")
        batchcmds:mkdir(path.join("$(buildir)/sokol_shader", path.directory(targetfile)))
        local targetfile = vformat(path.join("$(buildir)/sokol_shader", targetfile..".h"))
        batchcmds:vrunv("sokol-shdc", {
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

add_requires("stb", "sokol", "handmade_math")

if is_plat("windows") then
    add_cxflags("/utf-8")
elseif is_plat("mingw") then
    add_ldflags("-static")
end

if is_plat("windows", "mingw") then
    add_defines("SOKOL_D3D11")
elseif is_plat("macosx") then
    add_defines("SOKOL_METAL")
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
    set_kind("static")
    for _, f in ipairs(os.files("src/**/*.glsl")) do
        add_files(f)
    end
target_end()

target("sokol_wrapper")
    set_kind("static")
    add_packages("sokol")
    add_files("libs/sokol/sokol.c")
target_end()

for _, dir in ipairs(os.filedirs("src/*")) do
    if os.isdir(dir) and path.basename(dir) ~= "data" then
        local includedir = path.join("$(buildir)/sokol_shader", path.basename(dir))
        local targetname = path.basename(dir)
        for _, f in ipairs(os.files(dir.."/*.c")) do
            target(targetname.."-"..(split(path.basename(f), '-')[1]))
                add_packages("stb", "sokol", "handmade_math")
                add_files(f)
                if is_plat("windows", "mingw") then
		            add_files("src/resource.rc")
                end
                add_includedirs(includedir)
                add_deps("sokol_wrapper", "shader")
            target_end()
        end
    end
end
