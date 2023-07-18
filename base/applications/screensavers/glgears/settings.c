
#include "glgears.h"

#include <winreg.h>
#include <commctrl.h>
#include <debug.h>

static HDC hDC;
static HGLRC hRC;

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;

extern
void
CreateContext(HWND hWnd, PSETTINGS Settings);

VOID LoadSettings(PSETTINGS Settings)
{
	HKEY hkey;
    DWORD dwBuffSize;

    RegOpenKeyEx(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\ScreenSavers\\glgears",
                0,
                KEY_QUERY_VALUE,
                &hkey);
    
    if (hkey)
    {
        dwBuffSize = sizeof(BOOL);
        DPRINT1("RegQueyValueEx 0x%x\n", RegQueryValueEx(hkey, L"sRGBMode", NULL, NULL, (LPBYTE)&Settings->sRGBMode, &dwBuffSize));
        DPRINT1("RegQueyValueEx 0x%x\n", RegQueryValueEx(hkey, L"Info", NULL, NULL, (LPBYTE)&Settings->Info, &dwBuffSize));
        dwBuffSize = sizeof(DWORD);
        DPRINT1("RegQueyValueEx 0x%x\n", RegQueryValueEx(hkey, L"Multisampling", NULL, NULL, (LPBYTE)&Settings->Multisampling, &dwBuffSize));
        RegCloseKey(hkey);
    }
    else
    {
        /* Defaults */
        Settings->sRGBMode = FALSE;
        Settings->Multisampling = 0;
        Settings->Info = TRUE;
    }
}

VOID SaveSettings(PSETTINGS Settings)
{
    HKEY hkey;

    RegCreateKeyEx(HKEY_CURRENT_USER, 
                L"Software\\Microsoft\\ScreenSavers\\glgears",
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,
                &hkey,
                NULL);

    if (hkey)
    {
        DPRINT1("RegSetValueEx 0x%x\n", RegSetValueEx(hkey, L"sRGBMode", 0, REG_DWORD, (LPBYTE)&Settings->sRGBMode, sizeof(BOOL)));
        DPRINT1("RegSetValueEx 0x%x\n", RegSetValueEx(hkey, L"Multisampling", 0, REG_DWORD, (LPBYTE)&Settings->Multisampling, sizeof(DWORD)));
        DPRINT1("RegSetValueEx 0x%x\n", RegSetValueEx(hkey, L"Info", 0, REG_DWORD, (LPBYTE)&Settings->Info, sizeof(BOOL)));
    }
}

VOID VerifyGLFeatures(HWND hDlg)
{
    SETTINGS Settings = {0,0,0};
    GLint maxSamples;
    
    CreateContext(hDlg, &Settings);
            
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    if(wglChoosePixelFormatARB == NULL)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SRGB), FALSE);
    }
    
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    wglMakeCurrent (NULL, NULL);
    wglDeleteContext (hRC);
    ReleaseDC (hDlg, hDC);
    if (maxSamples < 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_MSAA), FALSE);
    }
    else
    {
        SendDlgItemMessage(hDlg,
                        IDC_MSAA_CONTROL,
                        UDM_SETRANGE,
                        0,
                        MAKELONG(maxSamples, 2));
    }
}

VOID UpdateControl(HWND hDlg)
{
    if (IsDlgButtonChecked(hDlg, IDC_MSAA == BST_CHECKED))
        EnableWindow(GetDlgItem(hDlg, IDC_MSAALEVEL), TRUE);
}

//
// Dialogbox procedure for Configuration window
//
BOOL CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PSETTINGS Settings = NULL;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Settings = (PSETTINGS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SETTINGS));
            LoadSettings(Settings);
            VerifyGLFeatures(hDlg);

            CheckDlgButton(hDlg,
                    IDC_INFO,
                    (Settings->Info) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg,
                    IDC_SRGB,
                    (Settings->sRGBMode) ? BST_CHECKED : BST_UNCHECKED);
            
            if (Settings->Multisampling > 1)
                CheckDlgButton(hDlg,
                        IDC_MSAA,
                        BST_CHECKED);

            UpdateControl(hDlg);

            SendDlgItemMessage(hDlg,
                                IDC_MSAA_CONTROL,
                                UDM_SETPOS32,
                                0,
                                Settings->Multisampling);

            HeapFree(GetProcessHeap(), 0, Settings);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MSAA:
                    UpdateControl(hDlg);
                    break;
                    
                case IDOK:
                    Settings = (PSETTINGS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SETTINGS));
                    Settings->sRGBMode = IsDlgButtonChecked(hDlg, IDC_SRGB);
                    Settings->Info = IsDlgButtonChecked(hDlg, IDC_INFO);
                    Settings->Multisampling = IsDlgButtonChecked(hDlg, IDC_MSAA);
                    SaveSettings(Settings);
                    HeapFree(GetProcessHeap(), 0, Settings);

                    /* Fall through */

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            return FALSE;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;
            
        case WM_DESTROY:
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
