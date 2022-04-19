#include "precomp.h"

#include <winreg.h>
#include <wchar.h>
#include <strsafe.h>

//#define NDEBUG
#include <debug.h>

#include "resource.h"

static HINSTANCE hInstance;

static VOID
SetDynaCDSPreference(PDESKTROUBLESHOOT This)
{
    HKEY hKey = NULL;
    LONG lResult;
    WCHAR szCDSPref[40];
    DWORD dwDataBuff = sizeof(szCDSPref);
    
    lResult = RegCreateKeyExW(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display",
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
    
    if (lResult == ERROR_SUCCESS)
    {
        StringCchPrintfW(szCDSPref, sizeof(szCDSPref)/sizeof(WCHAR), L"%d", This->lCDSPref);
        DPRINT1("Saving reg value: %ws\n", szCDSPref);
        RegSetValueExW(hKey, L"DynaSettingsChange", 0, REG_SZ, (LPBYTE)&szCDSPref, dwDataBuff);
        RegCloseKey(hKey);
    }
}

static VOID
GetDynaCDSPreference(PDESKTROUBLESHOOT This)
{
    HKEY hKey = NULL;
    LONG i = 0;
    LONG lResult;
    ULONG lCDSPref = 0;
    WCHAR szCDSData[50];
    DWORD dwDataBuff = sizeof(szCDSData);
    
    /* Get the dynamic change display settings preference */
    RegOpenKeyExW(HKEY_CURRENT_USER,
                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display",
                  0,
                  KEY_READ,
                  &hKey);
    
    lResult = RegQueryValueExW(hKey,
                               L"DynaSettingsChange",
                               0,
                               0,
                               (LPBYTE)&szCDSData,
                               &dwDataBuff);
    
    /* Convert the numerical string value to integer */
    if (lResult == ERROR_SUCCESS)
    {
        while(TRUE)
        {
            if (szCDSData[i] < 48 || szCDSData[i] > 57)
                break;
            lCDSPref = lCDSPref * 10 + szCDSData[i] - '0';
            i++;
        }
            
            
        DPRINT1("DynaCDSPreference is %d, i is %d\n", lCDSPref, i);
    }

    RegCloseKey(hKey);

    if (i == 0)
    {  

        dwDataBuff = sizeof(szCDSData);
        
        /* We failed to get anything from HKCU. Fallback to HKLM */
        RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display",
                      0,
                      KEY_READ,
                      &hKey);
             
        lResult = RegQueryValueExW(hKey,
                                   L"DynaSettingsChange",
                                   0,
                                   0,
                                   (LPBYTE)&szCDSData,
                                   &dwDataBuff);

        if (lResult == ERROR_SUCCESS)
        {
            while(TRUE)
            {
                if (szCDSData[i] < 48 || szCDSData[i] > 57)
                    break;
                lCDSPref = lCDSPref * 10 + szCDSData[i] - '0';
                i++;
            }
                
                
            DPRINT1("DynaCDSPreference is %d\n", lCDSPref);
        }
        
        RegCloseKey(hKey);
    }
    
    if (i != 0)
    {
        This->lCDSPref = lCDSPref;
    }
    else
    {
        /* Fallback to default preference (apply new settings dynamically) */
        This->lCDSPref = 1;
    }
              
}

/* deskperf implements its own AskDynamicApply dialog because it can't use */
/* DisplaySaveSettings which would invoke it from desk.cpl/themeui.dll     */
static INT_PTR CALLBACK
AskDynaCDSProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PDESKTROUBLESHOOT This;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
    {
        This = (PDESKTROUBLESHOOT)GetWindowLongPtrW(hwndDlg, DWLP_USER);
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            This = (PDESKTROUBLESHOOT)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)This);
            CheckDlgButton(hwndDlg, ((This->lCDSPref & 1) != 0) + IDC_REBOOT_REQUIRED, BST_CHECKED);
            Ret = TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    This->lCDSPref = IsDlgButtonChecked(hwndDlg, IDC_WITHOUT_REBOOT);
                    if (IsDlgButtonChecked(hwndDlg, IDC_SAVE_CDSPREF))
                        SetDynaCDSPreference(This);  
                    EndDialog(hwndDlg,
                              IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,
                              IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg,
                      IDCANCEL);
            break;
    }

    return Ret;
}

static VOID
USWCMessage(PDESKTROUBLESHOOT This)
{
    WCHAR szMsgBoxTitle[50];
    WCHAR szMsgBoxDesc[100];
    
    if (!LoadStringW(hInstance,
                    IDS_USWC_TITLE,
                    szMsgBoxTitle,
                    sizeof(szMsgBoxTitle) / sizeof(WCHAR)))
        return;
    
    if (!LoadStringW(hInstance,
                    IDS_USWC_DESC,
                    szMsgBoxDesc,
                    sizeof(szMsgBoxDesc) / sizeof(WCHAR)))
        return;

    MessageBoxW(This->hwndDlg,
                szMsgBoxDesc,
                szMsgBoxTitle,
                MB_ICONINFORMATION);
}

static VOID
RegErrorMessage(PDESKTROUBLESHOOT This)
{
    WCHAR szMsgBoxTitle[50];
    WCHAR szMsgBoxDesc[100];
    
    if (!LoadStringW(hInstance,
                    IDS_REG_ERROR_TITLE,
                    szMsgBoxTitle,
                    sizeof(szMsgBoxTitle) / sizeof(WCHAR)))
        return;

    
    if (!LoadStringW(hInstance,
                    IDS_REG_ERROR_DESC,
                    szMsgBoxDesc,
                    sizeof(szMsgBoxDesc) / sizeof(WCHAR)))
        return;
    
    MessageBoxW(This->hwndDlg,
                szMsgBoxDesc,
                szMsgBoxTitle,
                MB_ICONSTOP);
}

static VOID
SetUSWCState(PDESKTROUBLESHOOT This)
{
    HKEY hKey = NULL;
    HKEY hKeyUSWC = NULL;
    LONG lResult;

    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                  L"System\\CurrentControlSet\\Control\\GraphicsDrivers",
                  0,
                  KEY_WRITE,
                  &hKey);

    /* Depending on current state, either enable or disable USWC */
    if (This->bDisableUSWC == FALSE)
    {
        lResult = RegCreateKeyExW(hKey,
                                  L"DisableUSWC",
                                  0,
                                  NULL,
                                  0,
                                  KEY_READ,
                                  NULL,
                                  &hKeyUSWC,
                                  NULL);
        RegCloseKey(hKeyUSWC);
    }
    else
	{
        lResult = RegDeleteKeyExW(hKey,
                                  L"DisableUSWC",
                                  0,
                                  0);
	}
	
    RegCloseKey(hKey);

    if (lResult == ERROR_SUCCESS)
    {
        USWCMessage(This);
        This->bDisableUSWC = !This->bDisableUSWC;
    }
    else
    {
        DPRINT1("Failed! error: %d\n", lResult);
        RegErrorMessage(This);
    }

}

static VOID
GetUSWCState(PDESKTROUBLESHOOT This)
{
    LONG lResult;
    HKEY hKey = NULL;
    HKEY hKeyUSWC = NULL;
    This->bReadOnly = FALSE;
    
    /* Default is FALSE (USWC enabled) */
    This->bDisableUSWC = FALSE;
    
    /* Try to open the Graphics Drivers key with r/w access */
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Control\\GraphicsDrivers",
                            0,
                            KEY_READ | KEY_WRITE,
                            &hKey);
    
    
    if (lResult != ERROR_SUCCESS)
    {
        /* We failed. Try read-only access */
        lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"System\\CurrentControlSet\\Control\\GraphicsDrivers",
                                0,
                                KEY_READ,
                                &hKey);
        if (lResult == ERROR_SUCCESS)
        {
            /* We have restricted permissions. Disable this control from user modification */
            This->bReadOnly = TRUE;
        }
    }
        
    lResult = RegOpenKeyExW(hKey,
                            L"DisableUSWC",
                            0,
                            KEY_READ,
                            &hKeyUSWC);

    if (lResult == ERROR_SUCCESS)
    {
        /* Key exists. USWC is disabled */
        This->bDisableUSWC = TRUE;
    }
    
    RegCloseKey(hKeyUSWC);
    RegCloseKey(hKey);

}

static VOID
GetDeviceKey(PDESKTROUBLESHOOT This)
{
    DWORD dwDevNum = 0;
    BOOL bDevFound = FALSE;
    LONG i = 0;
    LPCWSTR szDeskDisplayDevice = NULL;
    LPWSTR szDeviceKeyPos = NULL;
    SIZE_T lenDeviceKey;
    DISPLAY_DEVICE dd;
    
    dd.cb = sizeof(DISPLAY_DEVICE);
    szDeskDisplayDevice = (LPCWSTR)QueryDeskCplString(This->pdtobj,
                                                      RegisterClipboardFormatW(
                                                      DESK_EXT_DISPLAYDEVICE));
   
    while(EnumDisplayDevicesW(NULL, dwDevNum, &dd, 0))
    {
        if (!lstrcmpW(dd.DeviceName, szDeskDisplayDevice))
        {
            bDevFound = TRUE;
            break;
        }
        else
        {
            dwDevNum++;
        }
    }
    
    if (bDevFound == FALSE)
    {
        /* No device of such index. Something went wrong */
        This->szDeviceKey = NULL;
        return;
    }

    lenDeviceKey = lstrlenW(dd.DeviceKey);
    This->szDeviceKey = (LPWSTR)malloc(sizeof(WCHAR)*lenDeviceKey+sizeof(UNICODE_NULL));
    
    if (This->szDeviceKey == NULL)
        return;
    
    while(i <= lenDeviceKey)
    {
        This->szDeviceKey[i] = towupper(dd.DeviceKey[i]);
        i++;
    }
    
    szDeviceKeyPos = wcsstr(This->szDeviceKey, L"\\SYSTEM");
    
    lstrcpynW(This->szDeviceKey, szDeviceKeyPos+1, lstrlenW(szDeviceKeyPos));
    DPRINT1("Copying reg key: %ws\n", This->szDeviceKey);	
    
}

static VOID
SetAccelerationLevel(PDESKTROUBLESHOOT This)
{
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwAccelData = 0;
    DWORD dwDataBuff = 0;
    
    if (This->szDeviceKey == NULL)
    {
        RegErrorMessage(This);
        return;
    }
    
    dwAccelData = This->dwAccelLevel;
    dwDataBuff = sizeof(dwAccelData);

    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                  This->szDeviceKey,
                  0,
                  KEY_WRITE,
                  &hKey);

    lResult = RegSetValueExW(hKey,
                             L"Acceleration.Level",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwAccelData,
                             dwDataBuff);
    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS)
    {
        DPRINT1("Failed! error: %d\n", lResult);
        RegErrorMessage(This);
    }
    else
    {
        This->dwAccelLevelInReg = This->dwAccelLevel;
    }
}

static VOID
GetAccelerationLevel(PDESKTROUBLESHOOT This)
{
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwDataBuff = sizeof(DWORD);
    
    /* Default is 0 (all accelerations enabled) */
    DWORD dwAccelData = 0;
    
    This->bReadOnly = FALSE;

    GetDeviceKey(This);
    DPRINT1("Opening: %ws\n", This->szDeviceKey);
    
    if (This->szDeviceKey == NULL)
        return;
    
    /* We got a device key. Try to open it with r/w accesss */
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            This->szDeviceKey,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hKey);
    
    if (lResult != ERROR_SUCCESS)
    {
        /* We failed. Try read-only access */
        lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                This->szDeviceKey,
                                0,
                                KEY_READ,
                                &hKey);
        
        if (lResult == ERROR_SUCCESS)
        {
            /* We have restricted permissions. Disable this control from user modification */
            This->bReadOnly = TRUE;
        }
    }
    
    
    RegQueryValueExW(hKey,
                     L"Acceleration.Level",
                     0,
                     0,
                     (LPBYTE)&dwAccelData,
                     &dwDataBuff);
                     
    RegCloseKey(hKey);

    DPRINT1("PerfLevel is %d\n", dwAccelData);
    This->dwAccelLevelInReg = dwAccelData; 
}

static VOID
UpdateGraphicsText(PDESKTROUBLESHOOT This)
{
    DWORD dwStrID;
    WCHAR szBuffer[200];
    dwStrID = IDS_LEVEL0 + This->dwAccelLevel;
    
    if (!LoadStringW(hInstance,
                    dwStrID,
                    szBuffer,
                    sizeof(szBuffer) / sizeof(WCHAR)))
        return;

    SetDlgItemTextW(This->hwndDlg,
                    IDC_ACCELERATION_LEVEL_TEXT,
                    szBuffer);
}

static VOID
UpdateAccelerationLevel(PDESKTROUBLESHOOT This)
{

    /* This is a reversed trackbar */                                               
    This->dwAccelLevel = 5 - SendDlgItemMessageW(This->hwndDlg, 
                                                 IDC_ACCELERATION_LEVEL,
                                                 TBM_GETPOS,
                                                 0, 0);
    UpdateGraphicsText(This);
}

static LONG
ApplyTroubleShootSettings(PDESKTROUBLESHOOT This)
{
    LONG lChangeRet = DISP_CHANGE_SUCCESSFUL;
    LONG lRet = PSNRET_NOERROR;
    BOOL bUSWCSet = FALSE;
    
    DPRINT1("bDisableUSWC is %d, ButtonState is %d\n", This->bDisableUSWC, IsDlgButtonChecked(This->hwndDlg, IDC_WRITE_COMBINING));
    /* Set the flag if user modified USWC state */
    if (IsDlgButtonChecked(This->hwndDlg, IDC_WRITE_COMBINING) == This->bDisableUSWC)
        bUSWCSet = TRUE;

    DPRINT1("dwAccelLevel is %d, dwAccelLevelInReg is %d\n", This->dwAccelLevel, This->dwAccelLevelInReg);
    
    /* Was the acceleration modified? */
    if (This->dwAccelLevel != This->dwAccelLevelInReg)
    {
        GetDynaCDSPreference(This);
        
        /* Invoke the AskDynamicApply dialog if there's such preference                   */
        /* (Windows accepts any DynaSettingsChange value that meets the bitwse AND tests) */
        if ((This->lCDSPref & 2) != 0 && bUSWCSet == FALSE)
        {   
            /* Only proceed further if dialog returned ID_OK value */
            if (DialogBoxParamW(hInstance, MAKEINTRESOURCE(IDD_COMPATIBILITY),
                This->hwndDlg, AskDynaCDSProc, (LPARAM)This) != 1)
                return PSNRET_INVALID_NOCHANGEPAGE;
        }
        
        SetAccelerationLevel(This);
        if (This->dwAccelLevel == This->dwAccelLevelInReg)
        {
          /* Dynamically apply new acceleration level if there's such preference */
          /* Otherwise notify user about required reboot */
          if ((This->lCDSPref & 1) != 0 && (This->lCDSPref & 2) == 0)
          {
              lChangeRet = ChangeDisplaySettingsW(NULL, CDS_RESET);
              DPRINT1("lChangeRet is %d\n", lChangeRet);
          }
          else
          {
              lChangeRet = DISP_CHANGE_RESTART;
          }
        }
        else
        { 
          return PSNRET_INVALID_NOCHANGEPAGE;
        }
    }
    
    if (lChangeRet == DISP_CHANGE_RESTART)
        PropSheet_RestartWindows(GetParent(This->hwndDlg));
    
    if (lChangeRet != DISP_CHANGE_SUCCESSFUL && lChangeRet != DISP_CHANGE_RESTART)
        lRet = PSNRET_INVALID_NOCHANGEPAGE;
    
    if (bUSWCSet == TRUE)
    {
        SetUSWCState(This);
        if (IsDlgButtonChecked(This->hwndDlg, IDC_WRITE_COMBINING) != This->bDisableUSWC)
            PropSheet_RestartWindows(GetParent(This->hwndDlg));
        else
            lRet = PSNRET_INVALID_NOCHANGEPAGE;
    }

    return lRet;
}

static VOID
InitTroubleShootDialog(PDESKTROUBLESHOOT This)
{
    GetAccelerationLevel(This);
    
    if (This->bReadOnly == TRUE)
        EnableWindow(GetDlgItem(This->hwndDlg, IDC_ACCELERATION_LEVEL), FALSE);
    
    GetUSWCState(This);
    
    if (This->bReadOnly == TRUE)
        EnableWindow(GetDlgItem(This->hwndDlg, IDC_WRITE_COMBINING), FALSE);
        
    
    SendDlgItemMessageW(This->hwndDlg,
                        IDC_ACCELERATION_LEVEL,
                        TBM_SETRANGE,
                        FALSE,
                        MAKELONG(0,5)); 
    
    SendDlgItemMessageW(This->hwndDlg, 
                        IDC_ACCELERATION_LEVEL,
                        TBM_SETPOS,
                        TRUE, 5 - This->dwAccelLevelInReg);
    
    UpdateAccelerationLevel(This);
    
    CheckDlgButton(This->hwndDlg,
                   IDC_WRITE_COMBINING,
                   (This->bDisableUSWC) ? BST_UNCHECKED : BST_CHECKED);
}

static INT_PTR CALLBACK
TroubleShootDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PDESKTROUBLESHOOT This;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
        This = (PDESKTROUBLESHOOT)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            This = (PDESKTROUBLESHOOT)((LPCPROPSHEETPAGE)lParam)->lParam;
            This->hwndDlg = hwndDlg;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)This);

            InitTroubleShootDialog(This);
            Ret = TRUE;
            break;

        case WM_HSCROLL:
            switch (LOWORD(wParam))
            {
                case TB_LINEUP:
                case TB_LINEDOWN:
                case TB_PAGEUP:
                case TB_PAGEDOWN:
                case TB_TOP:
                case TB_BOTTOM:
                case TB_ENDTRACK:
                case TB_THUMBTRACK:
                    UpdateAccelerationLevel(This);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_WRITE_COMBINING:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;
        
        case WM_NOTIFY:
        {
            NMHDR *nmh = (NMHDR *)lParam;

            switch (nmh->code)
            {
                case PSN_APPLY:
                SetWindowLongPtrW(hwndDlg,
                                 DWLP_MSGRESULT,
                                 ApplyTroubleShootSettings(This));
                Ret = TRUE;
                break;

                case PSN_RESET:
                    break;
            }
            break;
        }

    }

    return Ret;
}

static VOID
IDeskTroubleShoot_Destroy(PDESKTROUBLESHOOT This)
{
    if (This->pdtobj != NULL)
    {
        IDataObject_Release(This->pdtobj);
        This->pdtobj = NULL;
    }

    if (This->DeskExtInterface != NULL)
    {
        LocalFree((HLOCAL)This->DeskExtInterface);
        This->DeskExtInterface = NULL;
    }

    if (This->szDeviceKey != NULL)
    {
        free(This->szDeviceKey);
        This->szDeviceKey = NULL;
    }
}

ULONG
IDeskTroubleShoot_AddRef(PDESKTROUBLESHOOT This)
{
    ULONG ret;

    ret = InterlockedIncrement((PLONG)&This->ref);
    if (ret == 1)
        InterlockedIncrement(&dll_refs);

    return ret;
}

ULONG
IDeskTroubleShoot_Release(PDESKTROUBLESHOOT This)
{
    ULONG ret;

    ret = InterlockedDecrement((PLONG)&This->ref);
    if (ret == 0)
    {
        IDeskTroubleShoot_Destroy(This);
        InterlockedDecrement(&dll_refs);

        HeapFree(GetProcessHeap(),
                 0,
                 This);
    }

    return ret;
}

HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_QueryInterface(PDESKTROUBLESHOOT This,
                            REFIID iid,
                            PVOID *pvObject)
{
    *pvObject = NULL;

    if (IsEqualIID(iid,
                   &IID_IShellPropSheetExt) ||
        IsEqualIID(iid,
                   &IID_IUnknown))
    {
        *pvObject = impl_to_interface(This, IShellPropSheetExt);
    }
    else if (IsEqualIID(iid,
                        &IID_IShellExtInit))
    {
        *pvObject = impl_to_interface(This, IShellExtInit);
    }
    else if (IsEqualIID(iid,
                        &IID_IClassFactory))
    {
        *pvObject = impl_to_interface(This, IClassFactory);
    }
    else
    {
        DPRINT1("IDeskTroubleShoot::QueryInterface(%p,%p): E_NOINTERFACE\n", iid, pvObject);
        return E_NOINTERFACE;
    }

    IDeskTroubleShoot_AddRef(This);
    return S_OK;
}

HRESULT
IDeskTroubleShoot_Initialize(PDESKTROUBLESHOOT This,
                        LPCITEMIDLIST pidlFolder,
                        IDataObject *pdtobj,
                        HKEY hkeyProgID)
{
    DPRINT1("IDeskTroubleShoot::Initialize(%p,%p,%p)\n", pidlFolder, pdtobj, hkeyProgID);

    if (pdtobj != NULL)
    {
        IDataObject_AddRef(pdtobj);
        This->pdtobj = pdtobj;

        /* Get a copy of the desk.cpl extension interface */
        This->DeskExtInterface = QueryDeskCplExtInterface(This->pdtobj);
        if (This->DeskExtInterface != NULL)
            return S_OK;
    }

    return S_FALSE;
}

HRESULT
IDeskTroubleShoot_AddPages(PDESKTROUBLESHOOT This,
                      LPFNADDPROPSHEETPAGE pfnAddPage,
                      LPARAM lParam)
{
    HPROPSHEETPAGE hpsp;
    PROPSHEETPAGE psp;

    DPRINT1("IDeskTroubleShoot::AddPages(%p,%p)\n", pfnAddPage, lParam);

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_TROUBLESHOOT);
    psp.pfnDlgProc = TroubleShootDlgProc;
    psp.lParam = (LPARAM)This;

    hpsp = CreatePropertySheetPage(&psp);
    if (hpsp != NULL && pfnAddPage(hpsp, lParam))
        return S_OK;
    return S_FALSE;
}

HRESULT
IDeskTroubleShoot_ReplacePage(PDESKTROUBLESHOOT This,
                         EXPPS uPageID,
                         LPFNADDPROPSHEETPAGE pfnReplacePage,
                         LPARAM lParam)
{
    DPRINT1("IDeskTroubleShoot::ReplacePage(%u,%p,%p)\n", uPageID, pfnReplacePage, lParam);
    return E_NOTIMPL;
}

HRESULT
IDeskTroubleShoot_Constructor(REFIID riid,
                         LPVOID *ppv)
{
    PDESKTROUBLESHOOT This;
    HRESULT hRet = E_OUTOFMEMORY;

    DPRINT1("IDeskTroubleShoot::Constructor(%p,%p)\n", riid, ppv);

    This = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(*This));
    if (This != NULL)
    {
        ZeroMemory(This,
                   sizeof(*This));

        IDeskTroubleShoot_InitIface(This);

        hRet = IDeskTroubleShoot_QueryInterface(This,
                                           riid,
                                           ppv);
        if (!SUCCEEDED(hRet))
            IDeskTroubleShoot_Release(This);
    }

    return hRet;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}
