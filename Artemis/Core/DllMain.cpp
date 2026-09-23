#include "Proxy/ProxyExports.h"
#include <windows.h>

import Core;

namespace
{
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