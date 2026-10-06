#include "Proxy/ProxyExports.h"
#include <windows.h>

import Core;

namespace
{
    // Thread entry. Runs Artemis (Start, then Run until shutdown) off the loader lock.
    // param: HMODULE of this DLL.
    // note: Exceptions must not escape the thread. Any failure just ends it.
    DWORD WINAPI Bootstrap(LPVOID param)
    {
        const auto handleModule = static_cast<HMODULE>(param);

        try
        {
            Core::Artemis artemis{ handleModule };

            if (artemis.Start())
            {
                artemis.Run();
            }
        }
        catch (...) {}

        return 0;
    }
}

// DLL entry point. On attach, spawns the Bootstrap thread.
// return: FALSE only if the thread could not be created (aborts the load).
// note: Loader lock is held here, so no real work is done in this function.
extern "C" BOOL APIENTRY DllMain(HMODULE handleModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(handleModule);

        HANDLE thread = CreateThread(nullptr, 0, Bootstrap, handleModule, 0, nullptr);
        if (thread == nullptr) return FALSE;
        CloseHandle(thread);
    }

    return TRUE;
}