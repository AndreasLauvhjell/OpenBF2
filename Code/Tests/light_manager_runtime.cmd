@echo off
rem [CHOSEN, not recovered] https://github.com/B2BF/OpenBF2/issues/7
rem The preserved fixture uses retail host object offsets and VC7.1 host vectors.
echo RETIRED: light_manager_runtime requires the removed retail plugin ABI.
echo Port its host observers to native engine interfaces before restoring this runner.
exit /b 2
