# ergo — ZMK split keyboard (left + right)

A custom wireless split ergo keyboard running ZMK (main / Zephyr 4.1) on
two Seeed XIAO nRF52840 controllers.

- **Left half** — split *peripheral*. EC11 rotary encoder + MCP23008 column expander.
- **Right half** — split *central*. Azoteq IQS5XX (TPS43) trackpad + MCP23008.
- The right half is the central because ZMK pointing devices must run on the
  central; putting the trackpad side in charge avoids the fragile
  split‑peripheral input‑relay path entirely. The encoder on the peripheral
  is forwarded to the central automatically over the split link.

## Layout

```
.github/workflows/build.yml          GitHub Actions runner
build.yaml                           Builds ergo_left + ergo_right (+ settings_reset)
zephyr/module.yml                    Registers this repo as a ZMK module (board_root: .)
boards/shields/ergo/                 Single shield folder for BOTH halves
    Kconfig.shield                   Defines SHIELD_ERGO_LEFT and SHIELD_ERGO_RIGHT
    Kconfig.defconfig                Name + central role (right) + split (both)
    ergo.dtsi                        Shared: i2c0, pinctrl, combined matrix transform
    ergo_left.overlay                Left kscan (5 cols), encoder, col-offset 0
    ergo_right.overlay               Right kscan (6 cols), trackpad, col-offset 5
    ergo.keymap                      ONE unified keymap (25 left + 36 right keys)
    ergo.zmk.yml                      Metadata (siblings: ergo_left, ergo_right)
config/
    west.yml                         ZMK main + AYM1607 Azoteq driver
    ergo.conf                        Shared Kconfig (drivers/features)
```

## What was broken before, and what changed

| Problem in the previous repo | Fix |
| --- | --- |
| Shields lived in `config/boards/shields/…`, the deprecated location, while `zephyr/module.yml` pointed `board_root` at the repo root — so the module found nothing. | Moved both halves into a proper module at `boards/shields/ergo/`. |
| `ergo_left/Kconfig.shield` was a copy of the defconfig and **never defined `SHIELD_ERGO_LEFT`**, so the left half could not be selected/built. | One correct `Kconfig.shield` defines both `SHIELD_ERGO_LEFT` and `SHIELD_ERGO_RIGHT`. |
| Two separate shield folders + two separate keymaps. A split keyboard uses **one** keymap on the central. | Single `ergo/` folder, single `ergo.keymap`, combined matrix transform. |
| `CONFIG_ZMK_SPLIT=y` was set but **no `ZMK_SPLIT_ROLE_CENTRAL`** anywhere, and `build.yaml` built the halves as two unrelated keyboards. | Central role assigned to the right in `Kconfig.defconfig`; both halves share one logical keyboard. |
| Halves had independent matrix transforms with no offset, so their key positions collided. | Combined transform in `ergo.dtsi`; left at logical cols 0–4, right shifted with `col-offset = <5>`. |
| Left overlay still set `ngpios = <8>` (removed from the MCP23xxx binding in Zephyr 4.0). | Removed. |
| Board override file set `compatible = "nordic,nrf-twim"` on `&i2c0` (redundant) and duplicated pinctrl. | Removed the board override; `i2c0`/pinctrl defined once in `ergo.dtsi`. |
| Right `kscan` used `GPIO_ACTIVE_HIGH \| GPIO_PULL_DOWN` while the left used `GPIO_ACTIVE_LOW \| GPIO_PULL_UP` — opposite polarity for the same `col2row` wiring. | Normalised the right to match the left. **Verify against your diodes** (see below). |

Note: `CONFIG_GPIO_MCP230XX` *is* a valid symbol — it selects the underlying
`GPIO_MCP23XXX` driver — so it was kept. The `microchip,mcp23008` compatible is
correct for Zephyr 4.x.

## Things to verify against your actual hardware

These are assumptions I could not check without your board:

1. **Row polarity.** Left rows use `ACTIVE_LOW | PULL_UP`; right rows use
   `ACTIVE_HIGH | PULL_DOWN` (the two hand-built halves use opposite conventions).
   If the right matrix reads nothing, that's the first place to look.
2. **Trackpad reset.** `reset-gpios` is intentionally omitted so `gpio0 2` is free
   for the encoder A pin. The IQS5xx self-resets on power; if you ever need a hard
   reset line you'd have to free another pin for it (the XIAO is out of spare GPIO).
3. **Encoder.** Defined in the shared `ergo.dtsi` so the central also gets an
   enabled EC11 node (required, or the central disables its sensor thread).
3. **Board target.** `build.yaml` uses `xiao_ble//zmk`. The `//zmk` variant is
   required; bare `xiao_ble` fails with "board is not set up for ZMK". Very old
   configs used `seeeduino_xiao_ble`.
4. **The left transform is column‑major** (one visual row per matrix column),
   preserved exactly as your original left half. If the left half's keys land in
   the wrong spots, that wiring assumption is the place to look.

## Build & flash

1. Push to GitHub; the Actions workflow builds automatically.
2. Download the `firmware` artifact — it contains `ergo_left.uf2`,
   `ergo_right.uf2`, and `settings_reset.uf2`.
3. First time / after config changes that touch both halves:
   - Unpair any old "Ergo" entries from the host's Bluetooth menu.
   - Flash `settings_reset.uf2` to **each** XIAO (double‑tap reset, drag the UF2).
   - Flash `ergo_left.uf2` to the left, `ergo_right.uf2` to the right.
   - Power both on; they pair to each other and the right (central) advertises to the host.

## Honest caveat

This config is structured to ZMK's current split conventions and the individual
pieces are internally consistent, but it has **not been compiled here** — a full
ZMK/Zephyr build wasn't available in the environment it was assembled in. Expect
to iterate on the hardware‑specific items listed above. The trackpad
(input/pointing) path is the most likely to need a tweak; if a first build fails,
comment out the trackpad node + listener in `ergo_right.overlay` and the
`CONFIG_*POINTING*`/`*AZOTEQ*` lines in `config/ergo.conf` to confirm the keyboard
matrix builds green, then re‑enable the trackpad.
