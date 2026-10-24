# Native Windows validation

Validated on 2026-10-24 with portable w64devkit GCC 16.2.0, Windows x64.

- Image binding behavior test compiled and executed successfully.
- Frame detour compiled with warnings treated as errors.
- Executable frame-hook test rejected a modified signature, installed the expected
  17-byte prologue detour, and returned successfully through 100 guest-ABI calls.
- Guest memory helper compiled against the pinned Supermedo source headers.
- Loader integration patch passed git apply --check against the pinned source.

The executable hook test uses synthetic code with the game's verified prologue.
It does not load Bloodborne, exercise its threads, or prove GPU cache persistence.
The next milestone is a diagnostic tick inside a complete Supermedo build.

The toolchain and upstream checkout are local workspace dependencies, outside the
project repository. No game installation was changed.
