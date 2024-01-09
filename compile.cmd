cmake -B ../out -S . -G "MinGW Makefiles"
IF %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed.
    exit /b
)

cmake --build ../out
