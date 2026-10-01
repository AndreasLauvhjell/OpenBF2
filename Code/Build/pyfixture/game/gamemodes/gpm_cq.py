# SCAFFOLDING - the e10 probe's minimal game mode. The REAL mods/bf2/python/game/gamemodes/gpm_cq
# imports the engine's `host`/`bf2` modules, which are the not-yet-recovered host-module surface
# (53 functions, createHostModule). Until that batch lands, this fixture keeps the interpreter side
# of initializeGameMode honest: import succeeds, init() runs, deinit is bound.
def init():
    pass
def deinit():
    pass
