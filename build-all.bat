REM Build Engine
CALL clang engine\src\test.c engine\src\core\engine.c engine\src\core\logging.c -Iengine\src\ -g -shared -fdeclspec -o bin\engine.dll -DDLL_EXPORT

REM Build Testapp
CALL clang testapp\src\main.c -Iengine\src\ -g -o bin\testapp.exe -lbin\engine.lib
