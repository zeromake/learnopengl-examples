local files = os.files("build/sokol_shader.bak/**/*.glsl.h")

-- local files = {"build/sokol_shader.bak/5-4-point-shadows/1-omnidirectional-depth.glsl.h"}

local function parse_yaml_like_to_lua_table(yaml_text)
    local result = {}
    local current_table = result
    local stack = {}
    local prev_level = 1

    for line in yaml_text:gmatch("[^\r\n]+") do
        local indent = line:match("^%s*")
        local level = (#indent / 4) - 1
        while #stack > level do
            current_table = table.remove(stack)
        end
        local key, value = line:match("^%s*(.*)%s*%:%s*(.*)$")
        if key == nil then
            key, value = line:match("^%s*(.*)%s*=>%s*(.*)$")
        end
        if key == nil then
            goto continue
        end
        key = key:trim()
        if string.startswith(key, 'Shader program: ') then
            key = string.sub(key, 18, -2)
            value = nil
        elseif string.startswith(key, 'Get shader desc') then
            goto continue
        elseif string.startswith(key, 'C struct') then
            goto continue
        elseif string.startswith(key, 'Attributes') then
            goto continue
        elseif string.startswith(key, 'Uniform block') then
            goto continue
        elseif string.startswith(key, 'Vertex shader') then
            key = value
            value = nil
        elseif string.startswith(key, 'Fragment shader') then
            key = value
            value = nil
        elseif string.startswith(key, 'Bind slot') then
            key, value = string.match(value, 'SLOT_(%S+)%s*=>%s*(%d+)')
            key = "SLOT_"..key
        elseif string.startswith(key, 'ATTR_') then
            key = "ATTR_"..string.sub(key, 7+(#current_table._key))
        end
        if value == nil then
            current_table[key] = {
                _key = key,
            }
            table.insert(stack, current_table)
            current_table = current_table[key]
        else
            current_table[key] = tonumber(value)
        end
        ::continue::
    end
    return result
end

for _, f in ipairs(files) do
    local content = io.readfile(f)
    local start = string.find(content, "=========\n")+10
    local stop = string.find(content, "%*/")
    content = string.sub(content, start, stop-2)
    local bind = parse_yaml_like_to_lua_table(content)
    print(bind)
    local target = path.join('src', string.sub(f, 24, -3))
    local target_content = io.readfile(target)
    local links = {}
    local block2program = {}
    local current_block = nil
    for line in target_content:gmatch("[^\r\n]+") do
        if string.startswith(line, "@program ") then
            local program, vs_name, fs_name = line:match("@program (%S+) (%S+) (%S+)")
            links[vs_name] = program
            links[fs_name] = program
        elseif string.startswith(line, "@vs ") then
            current_block = string.match(line, "@vs (%S+)")
        elseif string.startswith(line, "@fs ") then
            current_block = string.match(line, "@fs (%S+)")
        elseif string.startswith(line, "@include_block ") then
            local block = line:match("@include_block (%S+)")
            block2program[block] = current_block
        end
    end
    current_block = nil
    local current_program = nil
    print("Processed "..target)
    local w = io.open(target, "wb")
    for line in target_content:gmatch("[^\r\n]*") do
        if string.startswith(line, "@vs ") then
            local name = string.match(line, "@vs (%S+)")
            current_block = name
            current_program = links[name]
        elseif string.startswith(line, "@block ") then
            local name = string.match(line, "@block (%S+)")
            current_block = block2program[name]
            current_program = links[current_block]
        elseif string.startswith(line, "@fs ") then
            local name = string.match(line, "@fs (%S+)")
            current_block = name
            current_program = links[name]
        elseif string.startswith(line, "uniform ") then
            local name = string.match(line, "uniform ([^%s%[]+)")
            if name == 'texture2D' then
                name = string.match(line, "uniform +texture2D +(%S+);")
            elseif name == 'sampler2D' then
                name = string.match(line, "uniform +sampler2D +(%S+);")
            elseif name == 'sampler' then
                name = string.match(line, "uniform +sampler +(%S+);")
            elseif name == 'textureCube' then
                name = string.match(line, "uniform +textureCube +(%S+);")
            end
            -- print(current_program, current_block, name)
            local slot = bind[current_program][current_block]['SLOT_'..name]
            if slot == nil then
                print("Slot not found for "..name)
            else
                line = string.format('layout(binding = %d) ', slot)..line
                -- print("Slot find for "..name)
            end
        end
        w:write(line.."\n")
    end
    w:close()
    -- break
end
