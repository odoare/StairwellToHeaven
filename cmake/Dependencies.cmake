# ──────────────────────────────────────────────────────────────────────────
# Dependencies.cmake — StairwellToHeaven's pinned dependencies.
#
# Included after project() by the root CMakeLists.txt. Resolution (sibling
# clone or pinned commit) is FxmeDeps.cmake's: locally the sibling clones
# (../JUCE, ../FxmeTools, ../clap-juce-extensions) win; release builds
# configure with -DFXME_USE_SIBLINGS=OFF and use the pins below.
# ──────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/FxmeDeps.cmake)

fxme_dependency(JUCE https://github.com/juce-framework/JUCE 9.0.4 SHALLOW)
# TODO before the first release: bump to the FxmeTools commit that adds
# dsp/TankReverb.h and dsp/EarlyReflections.h (not pushed when this was
# written; the sibling clone has them).
fxme_dependency(FxmeTools https://github.com/odoare/FxmeTools
    e4807f4a26d0c8fee20f06f9297fdda2a369f394)
# CLAP format (fxmetools_add_clap). Its last tag predates JUCE 9 support:
# pinned to a commit. Brings the clap and clap-helpers submodules along.
fxme_dependency(ClapJuceExtensions https://github.com/free-audio/clap-juce-extensions
    7adee3a1bd4684d4caa5601100e364abccff4b4d
    SIBLING clap-juce-extensions)

add_subdirectory(${JUCE_DIR} JUCE)

# Registers the `FxmeTools` JUCE module, FxmeCore, fxmetools_attach() and
# fxmetools_add_clap().
include(${FXMETOOLS_DIR}/cmake/FxmeTools.cmake)
