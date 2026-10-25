# Building the experimental add-on

The release includes a separate `corresponding-source.zip`: the complete modified
Supermedo tree (including initialized Git submodule sources), World Warper sources,
tests, and the existing upstream copyright/licence notices. Upstream's separate
benchmark save/recording assets under `launcher/assets/tests` are omitted; they
are not required to compile the executable or run World Warper. No game data is included.
The executable is GPL-3.0-or-later; upstream and third-party notices remain applicable.
Runtime DLLs are **not redistributed** by this add-on; they come from the user's
existing bbport installation.

1. Extract the source archive. Enter `modified-supermedo` in an **MSYS2 CLANG64** shell.
2. Install the dependencies listed in its `packaging/windows/README.md`. The source
   archive has the pinned submodule files already present; it does not require Git
   metadata or downloading another fork's source.
3. Run `bash build.sh`. Windows builds use RelWithDebInfo, PGO off, LTO off and
   Clang/LLVM. The modified build script limits Ninja to two jobs to fit small-RAM hosts.
4. The executable is `out/bb-probe.exe`. Copy it into the portable add-on's `bin` folder.
5. Run the World Warper Python tests with
   `python -m unittest discover -s tests -v` from `world-warper`.
   Native regression commands are in `.github/workflows/test.yml` there.

Base: `Supermedo/bloodborne_pc`, `windows-v1.9.4`, commit
`3fabfa7a7af67a34a6dfc630b6ec2539b68fad4d`. The source archive contains the exact
modified files used for the release, not only links to upstream. `BUILD.json`
records the executable hash. Rendering performance depends on toolchain, driver,
hardware and settings; test outcomes are not a universal FPS guarantee.

For development from Git, `ports/supermedo/gpu/shim/` contains the overlays,
`ports/supermedo/src/` the runtime input replacements, and
`ports/supermedo/patches/` the image/compile integration patches. The archive's
`modified-supermedo` tree is the ready-to-build integration reference.
