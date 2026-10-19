#pragma once
#include "client.h"
#include "render.h"
#include "render_gl.h"
#include <cstring>
#include <string>
#include <windows.h>

#define PV_MPV_FUNCTIONS(X)                                                    \
  X(mpv_request_log_messages)                                                  \
  X(mpv_create)                                                                \
  X(mpv_initialize)                                                            \
  X(mpv_terminate_destroy)                                                     \
  X(mpv_set_option_string)                                                     \
  X(mpv_set_property)                                                          \
  X(mpv_set_property_string) X(mpv_get_property_string) X(mpv_get_property)    \
      X(mpv_free) X(mpv_wait_event) X(mpv_command_async)                       \
          X(mpv_set_wakeup_callback) X(mpv_error_string)                       \
              X(mpv_render_context_create) X(mpv_render_context_free)          \
                  X(mpv_render_context_update)                                 \
                      X(mpv_render_context_set_update_callback)                \
                          X(mpv_render_context_render)                         \
                              X(mpv_render_context_report_swap)                \
                                  X(mpv_get_time_us)
struct MpvApi {
  HMODULE module = nullptr;
#define PV_DECLARE(n) decltype(&::n) n = nullptr;
  PV_MPV_FUNCTIONS(PV_DECLARE)
#undef PV_DECLARE
  bool load(const std::wstring &path, std::wstring &error) {
    module = LoadLibraryExW(path.c_str(), nullptr,
                            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
      error = L"Não foi possível carregar libmpv-2.dll x64. Reinstale o "
              L"aplicativo e confira a quarentena do antivírus sem desativar a "
              L"proteção. Código Windows: " +
              std::to_wstring(GetLastError());
      return false;
    }
#define PV_LOAD(n)                                                              \
  do {                                                                          \
    FARPROC address = GetProcAddress(module, #n);                               \
    static_assert(sizeof(n) == sizeof(address));                                \
    std::memcpy(&n, &address, sizeof(n));                                       \
  } while (false);                                                              \
  if (!n) {                                                                     \
    error = L"A libmpv não contém a API necessária. Reinstale a biblioteca " \
            L"compatível pelo instalador.";                                     \
    FreeLibrary(module);                                                        \
    module = nullptr;                                                           \
    return false;                                                               \
  }
    PV_MPV_FUNCTIONS(PV_LOAD)
#undef PV_LOAD
    return true;
  }
  void unload() {
    if (module) {
      FreeLibrary(module);
      module = nullptr;
    }
#define PV_RESET(n) n = nullptr;
    PV_MPV_FUNCTIONS(PV_RESET)
#undef PV_RESET
  }
  ~MpvApi() { unload(); }
};
