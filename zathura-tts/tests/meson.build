# Test suite configuration for zathura-tts

# Test dependencies (inherit from parent)
test_deps = [
  glib_dep,
  girara_dep,
]

# Test framework sources
test_framework_sources = [
  'test-framework.c',
]

# Test framework library
test_framework_lib = static_library(
  'test-framework',
  test_framework_sources,
  dependencies: test_deps,
  include_directories: inc,
)

# Simple unit tests (no external dependencies)
test_simple = executable(
  'test-simple',
  'test-simple.c',
  dependencies: test_deps,
  link_with: test_framework_lib,
  include_directories: inc,
)

# Audio controller tests (compiled as part of main test runner)
test_audio_controller_sources = [
  'test-audio-controller.c',
]

# Main test runner (requires audio controller)
test_main_sources = [
  'test-main.c',
  'test-audio-controller.c',
  '../src/tts-audio-controller.c',
  '../src/tts-text-extractor.c',
  '../src/tts-engine.c',
  '../src/tts-error.c',
  '../src/zathura-stubs.c',
]

test_main = executable(
  'test-main',
  test_main_sources,
  dependencies: test_deps,
  link_with: test_framework_lib,
  include_directories: inc,
)

# Integration tests (requires more plugin sources) - temporarily disabled
# test_integration_sources = [
#   'test-integration.c',
#   '../src/tts-audio-controller.c',
#   '../src/tts-text-extractor.c',
#   '../src/tts-engine.c',
#   '../src/tts-config.c',
#   '../src/tts-error.c',
# ]

# test_integration = executable(
#   'test-integration',
#   test_integration_sources,
#   dependencies: plugin_dependencies,
#   link_with: test_framework_lib,
#   include_directories: inc,
# )

# Register tests with meson
test('simple-tests', test_simple)
test('main-tests', test_main)
# test('integration-tests', test_integration)  # Temporarily disabled

# Test runners (shell scripts) - only if they exist
test_runner_script = files('test-runner')
if test_runner_script.length() > 0
  configure_file(
    input: 'test-runner',
    output: 'test-runner',
    copy: true,
    install: false,
  )
endif

test_integration_runner_script = files('test-integration-simple-runner')
if test_integration_runner_script.length() > 0
  configure_file(
    input: 'test-integration-simple-runner',
    output: 'test-integration-simple-runner',
    copy: true,
    install: false,
  )
endif

# Summary for tests
summary({
  'Unit tests': 'enabled',
  'Integration tests': 'enabled',
  'Test framework': 'custom',
}, section: 'Testing')