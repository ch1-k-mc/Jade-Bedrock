# NOTICE

**Jade (unofficial Bedrock port)** is an unofficial port of the Java Edition mod **Jade** to Minecraft: Bedrock Edition (LeviLamina). It is not affiliated with the original authors, Mojang or Microsoft.

## License

This port (the C++ source code, the bundled images, language files and theme JSON files) is distributed under the same license as the original: **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**. The full text is in [LICENSE.md](LICENSE.md) (copied from the original). Summary: https://creativecommons.org/licenses/by-nc-sa/4.0/

- **Attribution**: the original authors and copyright notices are kept below and at the top of each source file.
- **Changes**: the original Java code was ported to C++ and adapted to Bedrock Edition. Each source file states the original file it is based on. Files that say "Part of Jade (unofficial Bedrock port)" were written for this port.
- **NonCommercial**: this port must not be sold or used for commercial purposes.
- **ShareAlike**: modified versions must be distributed under the same license.

## Original work

| Work | Authors | License | Source | Used |
|------|---------|---------|--------|------|
| Jade | Snownee and contributors (based on HWYLA by TehNut, which is based on WAILA by ProfMobius) | CC BY-NC-SA 4.0 | https://github.com/Snownee/Jade (branch `26.3-fabric`, commit `3dc11bb55cfda18de24747adb5177097ed4081b5`) | Structure, classes and logic ported to C++. Sprites (`textures/gui/sprites`), themes (`jade_themes`) and language files (en_us, ja_jp, zh_cn, zh_tw) are bundled; missing translations were added for this port |

## Third-party code included in the original

The license texts are in [licenses/](licenses/).

| Work | Copyright | License | Where |
|------|-----------|---------|-------|
| csscolor4j | silentsoft | Apache-2.0 (`licenses/csscolor4j.txt`) | `src/api/ui/Color.*` (color parsing for themes) |
| Modern Industrialization | Copyright (c) 2020 Azercoco & Technici4n | MIT (`licenses/Modern-Industrialization.txt`) | `src/util/FluidTextHelper.*` (fluid amount text) |
| Just Enough Items | Copyright (c) 2014-2015 mezz | MIT (`licenses/JustEnoughItems.txt`) | Fluid drawing in `src/overlay/DisplayHelper.*`; the rendering and translation helpers in `src/platform/` come from a Bedrock port of JEI by the same author as this port, under MIT |
| The One Probe | Copyright (c) 2016 McJty | MIT (`licenses/TheOneProbe.txt`) | The "One Probe" theme style |
| Create | Copyright (c) 2019 simibubi | MIT (`licenses/Create.txt`) | The "Create" theme style |

## Not included

No files of Minecraft are included. The icon of this port (`docs/images/icon.png`) is original artwork.
