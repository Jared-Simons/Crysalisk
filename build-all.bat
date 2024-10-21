@echo off

REM Build Engine
CALL make ASSEMBLY=engine EXTENSION=dll

REM Build Testapp
CALL make ASSEMBLY=testapp EXTENSION=exe ADDL_INCLUDE_DIRS=-Iengine\src ADDL_LINKER_FLAGS=-lbin\engine.lib
