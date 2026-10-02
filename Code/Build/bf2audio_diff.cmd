@echo off
rem [CHOSEN, not recovered] https://github.com/B2BF/OpenBF2/issues/7
rem The preserved differential host shares one retail ABI between both DLL runs.
echo RETIRED: bf2audio_diff requires the removed retail-compatible rebuilt audio DLL.
echo Use bf2audio_smoke.cmd for the native DLL. Differential testing needs separate reference and native hosts.
exit /b 2
