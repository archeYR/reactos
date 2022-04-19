#include "precomp.h"

#define NDEBUG
#include <debug.h>

LONG dll_refs = 0;

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IShellPropSheetExt_QueryInterface(IShellPropSheetExt *iface,
                                               REFIID iid,
                                               PVOID *pvObject)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskTroubleShoot_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IShellPropSheetExt_AddRef(IShellPropSheetExt* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskTroubleShoot_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IShellPropSheetExt_Release(IShellPropSheetExt* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskTroubleShoot_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IShellPropSheetExt_AddPages(IShellPropSheetExt* iface,
                                         LPFNADDPROPSHEETPAGE pfnAddPage,
                                         LPARAM lParam)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskTroubleShoot_AddPages(This,
                                 pfnAddPage,
                                 lParam);
}

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IShellPropSheetExt_ReplacePage(IShellPropSheetExt* iface,
                                            EXPPS uPageID,
                                            LPFNADDPROPSHEETPAGE pfnReplacePage,
                                            LPARAM lParam)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskTroubleShoot_ReplacePage(This,
                                    uPageID,
                                    pfnReplacePage,
                                    lParam);
}

static IShellPropSheetExtVtbl efvtIShellPropSheetExt =
{
    IDeskTroubleShoot_IShellPropSheetExt_QueryInterface,
    IDeskTroubleShoot_IShellPropSheetExt_AddRef,
    IDeskTroubleShoot_IShellPropSheetExt_Release,
    IDeskTroubleShoot_IShellPropSheetExt_AddPages,
    IDeskTroubleShoot_IShellPropSheetExt_ReplacePage
};

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IShellExtInit_QueryInterface(IShellExtInit *iface,
                                          REFIID iid,
                                          PVOID *pvObject)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellExtInit);
    return IDeskTroubleShoot_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IShellExtInit_AddRef(IShellExtInit* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellExtInit);
    return IDeskTroubleShoot_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IShellExtInit_Release(IShellExtInit* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellExtInit);
    return IDeskTroubleShoot_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IShellExtInit_Initialize(IShellExtInit* iface,
                                      LPCITEMIDLIST pidlFolder,
                                      IDataObject *pdtobj,
                                      HKEY hkeyProgID)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IShellExtInit);
    return IDeskTroubleShoot_Initialize(This,
                                   pidlFolder,
                                   pdtobj,
                                   hkeyProgID);
}

static IShellExtInitVtbl efvtIShellExtInit =
{
    IDeskTroubleShoot_IShellExtInit_QueryInterface,
    IDeskTroubleShoot_IShellExtInit_AddRef,
    IDeskTroubleShoot_IShellExtInit_Release,
    IDeskTroubleShoot_IShellExtInit_Initialize
};

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IClassFactory_QueryInterface(IClassFactory *iface,
                                          REFIID iid,
                                          PVOID *pvObject)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IClassFactory);
    return IDeskTroubleShoot_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IClassFactory_AddRef(IClassFactory* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IClassFactory);
    return IDeskTroubleShoot_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskTroubleShoot_IClassFactory_Release(IClassFactory* iface)
{
    PDESKTROUBLESHOOT This = interface_to_impl(iface, IClassFactory);
    return IDeskTroubleShoot_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IClassFactory_CreateInstance(IClassFactory *iface,
                                          IUnknown * pUnkOuter,
                                          REFIID riid,
                                          PVOID *ppvObject)
{
    if (pUnkOuter != NULL &&
        !IsEqualIID(riid,
                    &IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    return IDeskTroubleShoot_Constructor(riid,
                                    ppvObject);
}

static HRESULT STDMETHODCALLTYPE
IDeskTroubleShoot_IClassFactory_LockServer(IClassFactory *iface,
                                      BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&dll_refs);
    else
        InterlockedDecrement(&dll_refs);

    return S_OK;
}

static IClassFactoryVtbl efvtIClassFactory =
{
    IDeskTroubleShoot_IClassFactory_QueryInterface,
    IDeskTroubleShoot_IClassFactory_AddRef,
    IDeskTroubleShoot_IClassFactory_Release,
    IDeskTroubleShoot_IClassFactory_CreateInstance,
    IDeskTroubleShoot_IClassFactory_LockServer,
};

VOID
IDeskTroubleShoot_InitIface(PDESKTROUBLESHOOT This)
{
    This->lpIShellPropSheetExtVtbl = &efvtIShellPropSheetExt;
    This->lpIShellExtInitVtbl = &efvtIShellExtInit;
    This->lpIClassFactoryVtbl = &efvtIClassFactory;

    IDeskTroubleShoot_AddRef(This);
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;
    if (IsEqualCLSID(rclsid,
                     &CLSID_IDeskTroubleShoot))
    {
        return IDeskTroubleShoot_Constructor(riid,
                                        ppv);
    }

    DPRINT1("DllGetClassObject: CLASS_E_CLASSNOTAVAILABLE\n");
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    return dll_refs == 0 ? S_OK : S_FALSE;
}
