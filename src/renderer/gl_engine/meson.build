source_file = [
    'tvgGlCommon.h',
    'tvgGlCompositor.h',
    'tvgGlGeometry.h',
    'tvgGlRenderData.h',
    'tvgGlRenderTarget.h',
    'tvgGlRenderTask.h',
    'tvgGlRenderer.h',
    'tvgGlShaderSrc.h',
    'tvgGlShaders.h',
    'tvgGlShaderTypes.h',
    'tvgGlCommon.cpp',
    'tvgGlCompositor.cpp',
    'tvgGlGeometry.cpp',
    'tvgGlRenderData.cpp',
    'tvgGlRenderTarget.cpp',
    'tvgGlRenderTask.cpp',
    'tvgGlRenderer.cpp',
    'tvgGlShaderSrc.cpp',
    'tvgGlShaders.cpp',
    'tvgGlShaderTypes.cpp'
]

if host_machine.system() == 'darwin'
    gl_dep = declare_dependency(link_args: ['-framework', 'OpenGL'])
    compiler_flags += ['-Wno-deprecated']     #for deprecated opengl
elif cc.get_id() == 'emscripten'
    target_opengles = true
    gl_dep = dependency('GLESv3', required: false)
else
    #find a opengl library with fallbacks
    gl_dep = dependency('GL', required: false)

    if not gl_dep.found()
        gl_dep = meson.get_compiler('cpp').find_library('GL', required: false)
    endif

    if not gl_dep.found()
        gl_dep = dependency('GLESv3', required: false)
        target_opengles = true
    endif

    if not gl_dep.found()
        gl_dep = meson.get_compiler('cpp').find_library('GLESv3')
        target_opengles = true
    endif
endif

if target_opengles
    gl_target_profile = '-DTHORVG_GL_TARGET_GLES=1'
else
    gl_target_profile = '-DTHORVG_GL_TARGET_GL=1'
endif

engine_dep += [declare_dependency(
    compile_args        : gl_target_profile,
    dependencies        : gl_dep,
    include_directories : include_directories('.'),
    sources             : source_file,
)]
