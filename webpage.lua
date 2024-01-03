import("core.base.json")


local GitHubExamplesURL = "https://github.com/zeromake/learnopengl-examples/tree/master/src"
local WebpageDeployDir = "docs"
local items = json.loadfile("index.json")

local assets = {
    "awesomeface.png",
    "backpack.mtl",
    "backpack.obj",
    "backpack_diffuse.jpg",
    "backpack_normal.png",
    "backpack_specular.jpg",
    "brickwall.jpg",
    "brickwall_normal.jpg",
    "container.jpg",
    "container2.png",
    "container2_specular.png",
    "grass.png",
    "marble.jpg",
    "mars.png",
    "metal.png",
    "planet.mtl",
    "planet.obj",
    "rock.mtl",
    "rock.obj",
    "rock.png",
    "skybox_right.jpg",
    "skybox_left.jpg",
    "skybox_top.jpg",
    "skybox_bottom.jpg",
    "skybox_front.jpg",
    "skybox_back.jpg",
    "transparent_window.png",
    "uv_grid.png",
    "wood.png"
}

local webpageAssets = {
    'dummy.jpg',
    'favicon.png',
    'fontello.woff',
    'fontello.woff2'
}

if not os.exists(WebpageDeployDir) then
    os.mkdir(WebpageDeployDir)
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
local indexContent = io.readfile("webpage/index.html")
local wasmContent = io.readfile("webpage/wasm.html")

local function example_wasm(dir, example, content)
    local filename = example[2]
    local source = example[3]
    local glsl = example[4]
    local glsl_hidden = "hidden"
    local glsl_url = "."
    local src_url = GitHubExamplesURL.."/"..dir.."/"..source
    local src_path = GitHubExamplesURL.."/"..dir.."/"..source..".js"
    if glsl ~= "" then
        glsl_hidden = ""
        glsl_url = GitHubExamplesURL.."/"..dir.."/"..glsl
    end
    local arr = split(filename, '-')
    local progname = arr[1].."-"..arr[2].."-"..arr[3]
    content = content:gsub('$%{name%}', filename)
    content = content:gsub('$%{prog%}', progname)
    content = content:gsub('$%{source%}', src_url)
    content = content:gsub('$%{glsl%}', glsl_url)
    content = content:gsub('$%{hidden%}', glsl_hidden)
    io.writefile("docs/"..filename..".html", content)
end

local content = ''
for _, chapter in ipairs(items) do
    local chapter_title = chapter[1]
    local lessons = chapter[2]
    content = content..format('<h2>%s</i></h2>\n', chapter_title)
    for _, lesson in ipairs(lessons) do
        local lesson_title = lesson[1]
        local lesson_link = lesson[2]
        local dir = lesson[3]
        local examples = lesson[4]
        content = content..'<article>\n'
        content = content..format(
            '<section class="header"><h3><a href="%s">%s <i class="icon-link-ext"></i></a></h3></section>\n',
            lesson_link,
            lesson_title)
        content = content..'<section class="group examples">\n'
        for _, example in ipairs(examples) do
            local name = example[1]
            local filename = example[2]
            local url = format("%s.html", filename)
            local img_name = filename .. '.jpg'
            local img_path = 'webpage/' .. img_name
            if not os.exists(img_path) then
                img_name = 'dummy.jpg'
                img_path = 'webpage/dummy.jpg'
            end
            os.cp(img_path, "docs/")
            content = content..'<figure class="col-15">\n'
            content = content..format('<figcaption><h4>%s</h4></figcaption>\n', name)
            content = content..format('<div><img class="responsive" src="%s" alt=""></div>\n', img_name)
            content = content..format('<a href="%s">Read More</a>\n', url)
            content = content..'</figure>\n'
            example_wasm(dir, example, wasmContent)
        end
        content = content..'</section>\n'
        content = content..'</article>\n'
    end
    content = content..'<hr>\n'
end

indexContent = indexContent:gsub('$%{samples%}', content):gsub('$%{date%}', os.date('!%Y-%m-%d %H:%M:%S +00:00', os.time()))
io.writefile("docs/index.html", indexContent)

for _, asset in ipairs(assets) do
    os.cp(path.join("src/data", asset), "docs/")
end
for _, asset in ipairs(webpageAssets) do
    os.cp(path.join("webpage", asset), "docs/")
end

os.cp("build/wasm/wasm64/release/*.js", "docs/")
os.cp("build/wasm/wasm64/release/*.wasm", "docs/")