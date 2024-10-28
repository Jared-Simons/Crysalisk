@echo off

REM clean engine
CALL make -f "makefile.engine.mak" clean

REM clean testapp
CALL make -f "makefile.testapp.mak" clean
