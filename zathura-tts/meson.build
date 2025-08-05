giggproject('zathura-tts', 'c',
  version: '1.0.0',
  meson_version: '>=0.61',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
)

# Plugin information
plugin_name = 'zathura-tts'
plugin_api_version = '6'

# Dependencies
# Use installed Zathura headers
zathura_inc = include_directories('/usr/local/include/zathura')
glib_dep = dependency('glib-2.0', version: '>=2.50')
gtk_dep = dependency('gtk+-3.0', version: '>=3.22')

# Use the installed girara library
girara_dep = dependency('girara-gtk3', method: 'pkg-config', required: true)

# Get the plugin directory from zathura pkg-config
plugin_dir = '/usr/local/lib/x86_64-linux-gnu/zathura'

# Create a dummy zathura dependency since we're using local headers
zathura_dep = declare_dependency(
  include_directories: zathura_inc,
  compile_args: ['-DZATHURA_API_VERSION=6', '-DZATHURA_ABI_VERSION=7']
)

# Optional dependencies for TTS engines
speechd_dep = dependency('speech-dispatcher', required: false)

# Build configuration
conf_data = configuration_data()
conf_data.set_quoted('PLUGIN_NAME', plugin_name)
conf_data.set_quoted('PLUGIN_VERSION', meson.project_version())
conf_data.set_quoted('PLUGIN_API_VERSION', plugin_api_version)
conf_data.set('HAVE_SPEECHD', speechd_dep.found())

# Generate config header
config_h = configure_file(
  output: 'config.h',
  configuration: conf_data
)

# Include directories
inc = include_directories('.')

# Plugin sources
plugin_sources = [
  'src/plugin.c',
  'src/tts-engine.c',
  'src/tts-text-extractor.c',
  'src/tts-audio-controller.c',
  'src/tts-ui-controller.c',
  'src/tts-config.c',
  'src/tts-error.c',
  # Note: zathura-stubs.c is NOT included - Zathura provides these functions at runtime
]

# Plugin dependencies
plugin_dependencies = [
  zathura_dep,
  girara_dep,
  glib_dep,
  gtk_dep,
]

if speechd_dep.found()
  plugin_dependencies += speechd_dep
endif

# Build the plugin
plugin = shared_library(
  plugin_name,
  plugin_sources,
  dependencies: plugin_dependencies,
  include_directories: inc,
  install: true,
  install_dir: plugin_dir,
  name_prefix: '',
  # Allow undefined symbols - Zathura will provide them at runtime
  link_args: ['-Wl,--unresolved-symbols=ignore-all'],
  override_options: ['b_lundef=false'],
)

# Plugin metadata file
plugin_desktop = configure_file(
  input: 'data/org.pwmt.zathura-tts.desktop.in',
  output: 'org.pwmt.zathura-tts.desktop',
  configuration: conf_data,
  install: true,
  install_dir: plugin_dir,
)

# Tests subdirectory (conditional)
if get_option('tests')
  subdir('tests')
endif

# Summary
summary({
  'Plugin name': plugin_name,
  'Version': meson.project_version(),
  'API version': plugin_api_version,
  'Speech Dispatcher': speechd_dep.found(),
}, section: 'Configuration')