@echo off

REM Build Engine
CALL make -f "makefile.engine.mak"

REM Build Testapp    
CALL make -f "makefile.testapp.mak"
