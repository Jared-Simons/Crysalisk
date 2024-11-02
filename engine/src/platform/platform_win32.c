#include "core/event.h"
#include "platform.h"
#include <string.h>

#ifdef PLAT_WIN32

#include "core/input.h"
#include "core/logging.h"
#include "core/memory.h"

#include <stdlib.h> // NOTE: For memory functions

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
    out_window->internal_state = memory_allocate(sizeof(win32_window_state), MEMORY_TAG_CORE);
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
    switch (u_msg) {
    case WM_CLOSE: {
        event_data_t data;
        event_system_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
        break;
    }

    // win32 input handling
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        b8 pressed = (u_msg == WM_KEYDOWN) || (u_msg == WM_SYSKEYDOWN);
        input_system_process_key(w_param, pressed);
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP: {
        b8 pressed = (u_msg == WM_LBUTTONDOWN) || (u_msg == WM_RBUTTONDOWN) || (u_msg == WM_MBUTTONDOWN);

        // Convert the win32 mouse code to a platform-agnostic code.
        u8 converted_mouse_code = 0;
        switch (u_msg) {
        case 513:
        case 514:
            converted_mouse_code = MOUSE_BUTTON_LEFT;
            break;

        case 516:
        case 517:
            converted_mouse_code = MOUSE_BUTTON_RIGHT;
            break;

        case 519:
        case 520:
            converted_mouse_code = MOUSE_BUTTON_MIDDLE;
            break;

        default:
            converted_mouse_code = 0;
            break;
        }
        input_system_process_mouse_button(converted_mouse_code, pressed);
        break;
    }

    default:
        return DefWindowProcA(hwnd, u_msg, w_param, l_param);
        break;
    }

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

void platform_log_message(u8 log_level, const char* message) {
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    static u8 log_colors[6] = {64, 4, 6, 3, 5, 8};
    SetConsoleTextAttribute(output_handle, log_colors[log_level]);
    DWORD bytes_written = 0;
    u64 message_length = strlen(message);
    WriteConsoleA(output_handle, message, (DWORD)message_length, &bytes_written, 0);
    // Clear the console back to light gray after printing the message
    SetConsoleTextAttribute(output_handle, 15);
}

void* platform_allocate(u64 size) {
    return malloc(size);
}

void platform_free(void* block) {
    free(block);
}

void* platform_memcpy(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}

void platform_memset(void* dest, i32 value, u64 size) {
    memset(dest, value, size);
}

#endif // Windows platform layer
