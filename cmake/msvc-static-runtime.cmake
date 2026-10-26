# Links the MSVC runtime statically (no Visual C++ Redistributable needed).
# CI passes this via the CMAKE_TOOLCHAIN_FILE environment variable, so it also applies to
# JUCE's juceaide helper, which is configured in a separate CMake run during setup.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
