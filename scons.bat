@echo OFF

echo ==========================================
echo  Batch: %~n0%~x0 %*
echo  Working dir: %cd%
echo ==========================================

set PROJECT_ROOT=%~dp0
set SCONS_FOLDER=%PROJECT_ROOT%\scons

call python %SCONS_FOLDER%\scons.py -f %*