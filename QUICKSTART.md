# World Warper: start here

World Warper is an **experimental shader-cache warm-up tool** for Supermedo's
Bloodborne PC port. It automates equipment, lantern and boss encounter passes
to warm the cache before normal play.

## Requirements

- Windows x64, a working **Supermedo bbport v1.9.4** installation, and your own
  prepared Bloodborne **1.09** game files. Run the normal game successfully first.
- Download the `windows-x64.zip` asset from this project's GitHub prerelease;
  extract it into a normal writable folder. Do **not** overwrite your bbport folder.
- Download [EonaCatNL's original 100-percent test save, mod 619/file 3156](https://www.nexusmods.com/bloodborne/mods/619?tab=files&file_id=3156)
  as a ZIP. Keep the download unchanged; **do not install it into your live save**.
  The author forbids reuploads, so it cannot be bundled without their permission.

## Run it

1. Close Bloodborne. Double-click **Start-WorldWarper.cmd** in the extracted folder.
2. Paste your `bbport-windows` folder, then your game-data folder containing
   `eboot.bin` and `dvdroot_ps4`. Paste the downloaded save ZIP path when asked.
   These locations are remembered; no Python or compiler installation is needed.
3. Choose **1** for the **full warm-up: weapons/armour + all 44 lanterns + boss/phase
   batch**, automatically in that order. Choose **2** for the four-entrance diagnostic,
   **3** for bosses only, or **4** for equipment/lanterns only.
4. Leave the controls alone. It selects Continue and closes automatically. Full
   warm-up relaunches once between lanterns and bosses, resetting the test save.
   To cancel the whole job, close the command window and then the game window.
5. Start Bloodborne normally through your usual launcher afterward. Keep the same
   renderer settings/driver for the most useful cache reuse.

Boss batch requests ranged and close observation and lower-health stages. It
checks known script events, but some entrances, phases and actor swaps remain
unverified or incomplete. Gascoigne has separate axe and beast windows; the
entrance diagnostic intentionally does **not** run those phases. Micolash,
Gehrman, Moon Presence and Chalice bosses are not covered.

## Saves, progress and reports

- Your live save is never selected. The imported original becomes a local seed;
  **a disposable copy is reset from it before every game process**, including retries.
- GPU caches go to your existing bbport `user` folder. Nothing clears them.
- `build/fresh-character/smoke.log` contains the latest run; older logs and working
  saves are retained there. `*-report.json` separates confirmed events from failures.
- Progress resumes unfinished tours. Finishing all stops allows the next launch
  to start a new tour. An attempted or quiet stop does not prove complete coverage.
- Missing DLLs: use the complete prepared **v1.9.4** installation, including its
  `bin` folder. This add-on deliberately reuses those runtime libraries.
- Unsupported save ZIP: use the exact original download linked above. Setup checks
  both gameplay files against known hashes before importing anything.
- To check setup without starting the game, run `powershell -NoProfile
  -ExecutionPolicy Bypass -File .\Start-WorldWarper.ps1 -SetupOnly` in the extracted
  folder. All prompts stay in the command window, not the game launcher.
- Online play is disabled **only for the automated tour**. Normal launcher settings
  and personal saves are not changed.

When reporting a problem, send `smoke.log` and the relevant report JSON, not your
personal save. Read [current limitations](docs/STATUS.md) before a boss batch.
