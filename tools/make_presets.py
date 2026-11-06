#!/usr/bin/env python3
"""
make_presets.py: writes StairwellToHeaven's factory presets (Source/Assets/*.xml).

Each preset is a complete APVTS state (root tag "Parameters", one PARAM per
parameter, real values; choices as their index, switches as 0/1): the
defaults below, changed only where the preset says. Keep DEFAULTS in step
with createParameters() in Source/PluginProcessor.cpp.

    python3 tools/make_presets.py
"""
import os

DEFAULTS = {
    "mode": 3, "decay": 2.5, "size": 50, "predelay": 20, "predelaySync": 0, "predelayDiv": 5,
    "diffusion": 80, "damping": 40, "dampFreq": 5000, "bass": 1.2, "crossover": 300,
    "modRate": 0.7, "modDepth": 30, "erLate": 60, "width": 100, "lowCut": 20, "highCut": 20000,
    "shimmer": 50, "shimmerPitch": 36, "shimmer2": 0, "shimmerPitch2": 31,
    "freeze": 0, "mix": 30, "output": 0,
}

ROOM, STAIRWELL, PLATE, HALL, HEAVEN = range(5)

def pitch(semitones):
    """The shimmer interval choice for a number of semitones (-24 .. +24)."""
    return semitones + 24

PRESETS = {
    "Init": {},
    "Small Room": dict(mode=ROOM, decay=0.8, size=40, predelay=5, diffusion=85, damping=50,
                       dampFreq=6000, bass=1.0, erLate=40, mix=25),
    "Concrete Stairwell": dict(mode=STAIRWELL, decay=2.2, size=60, predelay=3, diffusion=55, damping=25,
                               dampFreq=7000, bass=0.9, erLate=45, modDepth=15, mix=30),
    "Vocal Plate": dict(mode=PLATE, decay=1.8, size=50, predelay=25, diffusion=90, damping=35,
                        dampFreq=7500, bass=0.9, lowCut=180, mix=25),
    "Big Hall": dict(mode=HALL, decay=3.5, size=70, predelay=30, diffusion=75, damping=50,
                     dampFreq=4500, bass=1.4, mix=30),
    "Stone Cathedral": dict(mode=HALL, decay=8.0, size=100, predelay=45, diffusion=70, damping=60,
                            dampFreq=3500, bass=1.8, crossover=250, modDepth=20, mix=35),
    "Heavens Door": dict(mode=HEAVEN, decay=6.0, size=80, predelay=40, shimmer=55, shimmerPitch=pitch(12),
                         damping=30, dampFreq=6000, lowCut=200, mix=40),
    "Fifth Heaven": dict(mode=HEAVEN, decay=9.0, size=90, predelay=30, shimmer=45, shimmerPitch=pitch(7),
                         modDepth=40, damping=35, lowCut=150, mix=40),
    "Choir Loft": dict(mode=HEAVEN, decay=7.0, size=85, predelay=35, shimmer=40, shimmerPitch=pitch(12),
                       shimmer2=30, shimmerPitch2=pitch(7), damping=40, dampFreq=5500, lowCut=180,
                       modDepth=35, mix=40),
    "Undertow": dict(mode=HEAVEN, decay=10.0, size=100, predelay=20, shimmer=35, shimmerPitch=pitch(-12),
                     shimmer2=20, shimmerPitch2=pitch(-5), damping=55, dampFreq=4000, bass=1.6, mix=35),
    "Synced Slapback Room": dict(mode=ROOM, decay=1.2, size=55, predelaySync=1, predelayDiv=8,
                                 erLate=50, mix=30),
}

def write(folder, name, values):
    params = dict(DEFAULTS, **values)
    lines = ['<?xml version="1.0" encoding="UTF-8"?>', '',
             f'<Parameters presetName="{name}">']
    lines += [f'  <PARAM id="{k}" value="{v:g}"/>' for k, v in params.items()]
    lines += ['</Parameters>', '']
    path = os.path.join(folder, name.replace(" ", "_") + ".xml")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    return path

if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    folder = os.path.join(here, "..", "Source", "Assets")
    for name, values in PRESETS.items():
        print(write(folder, name, values))
