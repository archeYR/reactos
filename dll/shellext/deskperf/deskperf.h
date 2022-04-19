#pragma once

typedef struct _DESKTROUBLESHOOT
{
    const struct IShellPropSheetExtVtbl *lpIShellPropSheetExtVtbl;
    const struct IShellExtInitVtbl *lpIShellExtInitVtbl;
    const struct IClassFactoryVtbl *lpIClassFactoryVtbl;
    DWORD ref;

    HWND hwndDlg;
    PDESK_EXT_INTERFACE DeskExtInterface;
    IDataObject *pdtobj;
    DWORD dwAccelLevel;
    DWORD dwAccelLevelInReg;
    ULONG lCDSPref;
    BOOL bDisableUSWC;
    BOOL bReadOnly;
    LPWSTR szDeviceKey;

} DESKTROUBLESHOOT, *PDESKTROUBLESHOOT;

extern LONG dll_refs;

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)
#define interface_to_impl(instance,iface) ((PDESKTROUBLESHOOT)((ULONG_PTR)instance - FIELD_OFFSET(DESKTROUBLESHOOT,lp##iface##Vtbl)))

HRESULT
IDeskTroubleShoot_Constructor(REFIID riid,
                         LPVOID *ppv);

VOID
IDeskTroubleShoot_InitIface(PDESKTROUBLESHOOT This);

HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_QueryInterface(PDESKTROUBLESHOOT This,
                            REFIID iid,
                            PVOID *pvObject);

ULONG
IDeskTroubleShoot_AddRef(PDESKTROUBLESHOOT This);

ULONG
IDeskTroubleShoot_Release(PDESKTROUBLESHOOT This);

HRESULT
IDeskTroubleShoot_Initialize(PDESKTROUBLESHOOT This,
                        LPCITEMIDLIST pidlFolder,
                        IDataObject *pdtobj,
                        HKEY hkeyProgID);

HRESULT
IDeskTroubleShoot_AddPages(PDESKTROUBLESHOOT This,
                      LPFNADDPROPSHEETPAGE pfnAddPage,
                      LPARAM lParam);

HRESULT
IDeskTroubleShoot_ReplacePage(PDESKTROUBLESHOOT This,
                         EXPPS uPageID,
                         LPFNADDPROPSHEETPAGE pfnReplacePage,
                         LPARAM lParam);

static const GUID CLSID_IDeskTroubleShoot = {0xf92e8c40,0x3d33,0x11d2,{0xb1,0xaa,0x08,0x00,0x36,0xa7,0x5b,0x03}};

ULONG __cdecl DbgPrint(PCCH Format,...);
