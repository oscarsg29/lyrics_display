# lyrics_display
project using a stm32 blue pill that uses a small lcd display to show the lyrics of a playing song

## Host unit tests

High-level, hardware-independent logic is available as the `unit_tests` target
in the main CMake project. The target configures and runs the native CTest
project in `tests/`, compiling `Core/App/app_logic.c` with the host compiler.

```sh
cmake --preset Debug
cmake --build --preset UnitTests
```

The underlying host test project can still be run directly with
`ctest --test-dir build/host-tests --verbose` after the target has configured
it. Use `--verbose` when you want to see every individual test case printed by
the executable.
