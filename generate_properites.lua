function main()
    local package_file = '.xmake/'..os.host()..'/'..os.arch()..'/cache/package'
    local packages = io.load(package_file)
    local all_sysincludedirs = {}
    for name, package in pairs(packages) do
        local sysincludedirs = package.sysincludedirs
        if sysincludedirs == nil then
            goto continue
        end
        if type(sysincludedirs) == 'table' then
            for _, sysincludedir in ipairs(sysincludedirs) do
                table.insert(all_sysincludedirs, sysincludedir)
            end
        else
            table.insert(all_sysincludedirs, sysincludedirs)
        end
        ::continue::
    end
    for _, sysincludedir in ipairs(all_sysincludedirs) do
        print(string.format('"%s",', sysincludedir:gsub('\\', '/')))
    end
end