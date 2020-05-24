#include <ndk/exfuncs.h>
#include <stdio.h>
#include <windows.h>
#include <conutils.h>
#include <strsafe.h>

#define NDEBUG

static BOOL UseKeyboard = FALSE;
static ULONG Multiplier = 4;

void MouseMove(int dx, int dy)
{
    mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
}

void MouseClick(int type)
{
    mouse_event(type == 0 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(type == 0 ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

int wmain(int argc, WCHAR *argv[])
{
    HANDLE hFile;
    DWORD dwBytesRead;
    CHAR c;
    CHAR str[4] = {0};

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    ConPuts(StdOut, L"COM control starting...\n");

    hFile = CreateFileW(L"COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ConPuts(StdOut, L"CreateFileW failed\n");
        return 1;
    }

    /* Disable echo */
    if (!WriteFile(hFile, "\xFF\xFB\x01\xFF\xFB\x03", 6, &dwBytesRead, NULL))
    {
        ConPuts(StdOut, L"WriteFile failed\n");
        CloseHandle(hFile);
        return 1;
    }
    /* Clear screen */
    if (!WriteFile(hFile, "\x1B[2J", 4, &dwBytesRead, NULL))
    {
        ConPuts(StdOut, L"WriteFile failed\n");
        CloseHandle(hFile);
        return 1;
    }

    while (TRUE)
    {
        if (!ReadFile(hFile, &c, sizeof(c), &dwBytesRead, NULL))
        {
            ConPuts(StdOut, L"ReadFile failed\n");
            break;
        }
        //ConPrintf(StdOut, L"%d ", c);
        str[0] = str[1];
        str[1] = str[2];
        str[2] = str[3];
        str[3] = c;

        if (str[1] == 0x1B)
        {
            if (str[2] == 0x5B)
            {
                switch (str[3])
                {
                    case 0x35: ConPuts(StdOut, L"PgUp\n");
                    if (Multiplier < 128)
                        Multiplier *= 2;
                    break;
                    case 0x36: ConPuts(StdOut, L"PgDn\n");
                    Multiplier /= 2;
                    if (!Multiplier)
                        Multiplier = 1;
                    break;
                    case 0x41: ConPuts(StdOut, L"Up\n");
                    MouseMove(0, -Multiplier);
                    break;
                    case 0x42: ConPuts(StdOut, L"Down\n");
                    MouseMove(0, Multiplier);
                    break;
                    case 0x43: ConPuts(StdOut, L"Right\n");
                    MouseMove(Multiplier, 0);
                    break;
                    case 0x44: ConPuts(StdOut, L"Left\n");
                    MouseMove(-Multiplier, 0);
                    break;
                    default: ConPrintf(StdOut, L"Unhandled: 0x%x\n", str[3]);
                    break;
                }
                str[1] = 0;
                str[2] = 0;
                str[3] = 0;
            }
        }
        if (str[2] == 0x0D && str[3] == 0x0A)
        {
            ConPuts(StdOut, L"Enter\n");
            MouseClick(1);
            str[2] = 0;
            str[3] = 0;
        }
        switch (str[3])
        {
            case 0x08: ConPuts(StdOut, L"Backspace\n");
            str[3] = 0;
            break;

            case 0x09: ConPuts(StdOut, L"Tab\n");
            UseKeyboard = !UseKeyboard;
            str[3] = 0;
            break;

            case 0x20: ConPuts(StdOut, L"Space\n");
            MouseClick(0);
            str[3] = 0;
            break;

            case 0x7F: ConPuts(StdOut, L"Delete\n");
            str[3] = 0;
            break;
        }
    }

    CloseHandle(hFile);

    return 0;
}

/* EOF */
