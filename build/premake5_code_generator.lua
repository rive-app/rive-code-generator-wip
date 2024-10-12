rive = '../rive-runtime'

-- dofile(path.join(path.getabsolute(rive) .. '/build', 'premake5.lua'))
-- dofile(path.join(path.getabsolute(rive) .. '/build', 'rive_build_config.lua'))
dofile(path.join(path.getabsolute(rive) .. '/premake5_v2.lua'))

project('rive_code_generator')
do
    kind('ConsoleApp')
    language('C++')
    cppdialect('C++17')

    includedirs({
        '../include',
        rive .. '/include',
        -- miniaudio,
        -- yoga,
        '../external/',
    })

    -- links({ 'rive', 'rive_harfbuzz', 'rive_sheenbidi', 'rive_yoga' })
    links({ 'rive' })

    files({
        '../src/**.cpp',
        rive .. '/utils/**.cpp',
    })

    -- buildoptions({ '-Wall', '-fno-exceptions', '-fno-rtti' })
    buildoptions({ '-Wall', '-fexceptions', '-fno-rtti' })

    -- filter({ 'system:macosx' })
    -- do
    --     links({
    --         'Cocoa.framework',
    --         -- 'CoreText.framework', 
    --     })
    -- end
end