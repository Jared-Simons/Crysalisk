REM Build Engine
CALL clang engine\src\core\engine.c engine\src\core\logging.c engine\src\platform\platform_win32.c -Iengine\src\ -g -shared -fdeclspec -o bin\engine.dll -luser32.lib -DPLAT_WIN32 -DDLL_EXPORT -D_CRT_SECURE_NO_WARNINGS

REM Build Testapp
CALL clang testapp\src\main.c -Iengine\src\ -g -o bin\testapp.exe -lbin\engine.lib
