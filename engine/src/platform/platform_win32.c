#include "platform.h"

#ifdef PLAT_WIN32

#include "core/logging.h"
#include <stdlib.h> // TODO: temp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct win32_platform_state {
    HMODULE hmodule;
    HINSTANCE hinstance;
} win32_platform_state;

typedef struct win32_window_state {
    HWND hwnd;
} win32_window_state;

static platform_state* state_ptr = 0;

LRESULT CALLBACK pfn_wnd_proc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

b8 platform_initialize(u64* memory_requirement, void* state, platform_state_config config) {
    // Get the memory requirement for the platform state
    *memory_requirement = sizeof(platform_state) + sizeof(win32_platform_state);

    if (state == NULL) {
        return true;
    }

    platform_state* plat_state = state;
    plat_state->internal_state = state + sizeof(platform_state);
    win32_platform_state* win32_state = plat_state->internal_state;

    win32_state->hmodule = GetModuleHandleA(0);
    win32_state->hinstance = GetModuleHandleA(0);

    state_ptr = state;

    // Create the main window for the engine.
    platform_create_window(config.application_name, &plat_state->main_window);

    return true;
}

b8 platform_update() {
    if (!platform_process_messages(&state_ptr->main_window)) {
        return false;
    }

    return true;
}

b8 platform_create_window(const char* window_title, window* out_window) {
    if (!out_window) {
        LOG_ERROR("platform_create_window requires a valid pointer to out_window. Window creation failed.");
        return false;
    }

    WNDCLASS window_class = {};
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = pfn_wnd_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = ((win32_platform_state*)state_ptr->internal_state)->hinstance;
    window_class.hIcon = 0;
    window_class.hCursor = 0;
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "crysalisk_window_class";

    // Register the window class.
    RegisterClassA(&window_class);

    // Create the window.
    out_window->internal_state = malloc(sizeof(win32_window_state));
    win32_window_state* win32_window = out_window->internal_state;

    win32_window->hwnd = CreateWindowExA(0, "crysalisk_window_class", window_title,
                                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, window_class.hInstance,
                                         NULL);

    if (!win32_window->hwnd) {
        LOG_ERROR("Failed to create win32 window!");
        return false;
    }

    ShowWindow(win32_window->hwnd, 1); // 1 for normal show

    return true;
}

LRESULT CALLBACK pfn_wnd_proc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
    return DefWindowProcA(hwnd, u_msg, w_param, l_param);
}

b8 platform_process_messages(window* window) {
    MSG msg;
    while (PeekMessageA(&msg, ((win32_window_state*)window->internal_state)->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

#endif // Windows platform layer
