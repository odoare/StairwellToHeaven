# Licensing

Stairwell to Heaven is a JUCE plugin built on
[FxmeTools](https://github.com/odoare/FxmeTools), which is split into a
framework-free half and a JUCE half under different licences. Each source
file states its licence in its header as an SPDX identifier.

```
Source/ and Tests/    AGPL-3.0-or-later, or commercial terms (LicenseRef-FXME-Commercial)
```

The plugin compiles against JUCE, which is dual-licensed (AGPLv3 or
commercial), so the plugin is offered under the AGPL version 3 or later
(full text in `LICENSE`), or under commercial terms available from the
author (Olivier Doaré, github.com/odoare).

The DSP it uses from FxmeTools' JUCE-free core (`fxme::TankReverb`,
`fxme::EarlyReflections` and the rest of `core/`) is under the GNU LGPL
version 3 or later; see FxmeTools' own `LICENSE.md`.
