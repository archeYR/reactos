/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device enumeration functions
 * COPYRIGHT:   2018 Vadim Galyant
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

extern KSPIN_LOCK IopPnPSpinLock;
extern LIST_ENTRY IopPnpEnumerationRequestList;
extern KEVENT PiEnumerationFinished;

// Request types for PIP_ENUM_REQUEST
typedef enum _PIP_ENUM_TYPE
{
    PipEnumAddBootDevices,
    PipEnumAssignResources,
    PipEnumGetSetDeviceStatus,
    PipEnumClearProblem,
    PipEnumInvalidateRelationsInList,
    PipEnumHaltDevice,
    PipEnumBootDevices,
    PipEnumDeviceOnly,
    PipEnumDeviceTree,
    PipEnumRootDevices,
    PipEnumInvalidateDeviceState,
    PipEnumResetDevice,
    PipEnumIoResourceChanged,
    PipEnumSystemHiveLimitChange,
    PipEnumSetProblem,
    PipEnumShutdownPnpDevices,
    PipEnumStartDevice,
    PipEnumStartSystemDevices
} PIP_ENUM_TYPE;

typedef struct _PIP_ENUM_REQUEST
{
    LIST_ENTRY RequestLink;
    PDEVICE_OBJECT DeviceObject;
    PIP_ENUM_TYPE RequestType;
    UCHAR ReorderingBarrier;
    ULONG_PTR RequestArgument;
    PKEVENT CompletionEvent;
    NTSTATUS *CompletionStatus;
} PIP_ENUM_REQUEST, *PPIP_ENUM_REQUEST;

BOOLEAN PipEnumerationInProgress;
WORK_QUEUE_ITEM PipDeviceEnumerationWorkItem;


VOID
NTAPI
PipEnumerationWorker(_In_ PVOID Context)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PPIP_ENUM_REQUEST Request;
    BOOLEAN IsDereferenceObject;
    KIRQL OldIrql;
    NTSTATUS Status;

    PpDevNodeLockTree(1);

    while (TRUE)
    {
        Status = STATUS_SUCCESS;
        IsDereferenceObject = TRUE;

        KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

        if (IsListEmpty(&IopPnpEnumerationRequestList))
        {
            break;
        }

        Request = CONTAINING_RECORD(RemoveHeadList(&IopPnpEnumerationRequestList),
                                    PIP_ENUM_REQUEST,
                                    RequestLink);

        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

        InitializeListHead(&Request->RequestLink);

        //FIXME: Check ShuttingDown

        DeviceObject = Request->DeviceObject;
        ASSERT(DeviceObject);

        DeviceNode = IopGetDeviceNode(DeviceObject);
        ASSERT(DeviceNode);

        if (DeviceNode->State != DeviceNodeDeleted)
        {
            DPRINT("PipEnumerationWorker: DeviceObject - %p, Request->RequestType - %X\n",
                   DeviceObject,
                   Request->RequestType);

            switch (Request->RequestType)
            {
                case PipEnumDeviceOnly:
                case PipEnumDeviceTree:
                case PipEnumRootDevices:
                case PipEnumSystemHiveLimitChange:
                    DPRINT("PipEnumerationWorker: Reenumeration ...\n");
                    Status = IopEnumerateDevice(Request->DeviceObject);
                    IsDereferenceObject = FALSE;
                    break;

                default:
                    DPRINT1("PipEnumerationWorker: RequestType %u is NOT IMPLEMENTED\n", Request->RequestType);
                    Status = STATUS_NOT_IMPLEMENTED;
                    break;
            }
        }
        else
        {
            DPRINT("PipEnumerationWorker: DeviceNode->State == DeviceNodeDeleted\n");
            Status = STATUS_UNSUCCESSFUL;
        }

        if (Request->CompletionStatus)
        {
            *Request->CompletionStatus = Status;
        }

        if (Request->CompletionEvent)
        {
            KeSetEvent(Request->CompletionEvent, IO_NO_INCREMENT, FALSE);
        }

        if (IsDereferenceObject)
        {
            ObDereferenceObject(Request->DeviceObject);
        }

        ExFreePoolWithTag(Request, TAG_IO);
    }

    PipEnumerationInProgress = FALSE;
    KeSetEvent(&PiEnumerationFinished, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
    PpDevNodeUnlockTree(1);
}

static
NTSTATUS
PipRequestDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIP_ENUM_TYPE RequestType,
    _In_ UCHAR ReorderingBarrier,
    _In_ ULONG_PTR RequestArgument,
    _In_ PKEVENT CompletionEvent,
    _Out_ NTSTATUS *CompletionStatus)
{
    PPIP_ENUM_REQUEST Request;
    PDEVICE_OBJECT RequestDeviceObject;
    KIRQL OldIrql;

    DPRINT("PipRequestDeviceAction: DeviceObject - %p, RequestType - %X\n",
           DeviceObject,
           RequestType);

    //FIXME: check ShuttingDown

    Request = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(PIP_ENUM_REQUEST),
                                    TAG_IO);
    if (!Request)
    {
        DPRINT1("PipRequestDeviceAction: cannot allocate a PIP_ENUM_REQUEST structure\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!DeviceObject)
    {
        RequestDeviceObject = IopRootDeviceNode->PhysicalDeviceObject;
    }
    else
    {
        RequestDeviceObject = DeviceObject;
    }

    ObReferenceObject(RequestDeviceObject);

    Request->DeviceObject = RequestDeviceObject;
    Request->RequestType = RequestType;
    Request->ReorderingBarrier = ReorderingBarrier;
    Request->RequestArgument = RequestArgument;
    Request->CompletionEvent = CompletionEvent;
    Request->CompletionStatus = CompletionStatus;

    InitializeListHead(&Request->RequestLink);

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

    InsertTailList(&IopPnpEnumerationRequestList, &Request->RequestLink);
    DPRINT("PipRequestDeviceAction: Inserted Request - %p\n", Request);

    if (RequestType == PipEnumAddBootDevices ||
        RequestType == PipEnumBootDevices ||
        RequestType == PipEnumRootDevices)
    {
        ASSERT(!PipEnumerationInProgress);

        PipEnumerationInProgress = TRUE;
        KeClearEvent(&PiEnumerationFinished);
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

        PipEnumerationWorker(Request);

        return STATUS_SUCCESS;
    }

    // Start the worker if it's not started already
    if (PipEnumerationInProgress)
    {
        DPRINT("PipRequestDeviceAction: PipEnumerationInProgress\n");
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
        return STATUS_SUCCESS;
    }

    PipEnumerationInProgress = TRUE;
    KeClearEvent(&PiEnumerationFinished);
    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

    ExInitializeWorkItem(&PipDeviceEnumerationWorkItem,
                         PipEnumerationWorker,
                         Request);

    ExQueueWorkItem(&PipDeviceEnumerationWorkItem, DelayedWorkQueue);
    DPRINT("PipRequestDeviceAction: Queue &PipDeviceEnumerationWorkItem - %p\n",
           &PipDeviceEnumerationWorkItem);

    return STATUS_SUCCESS;
}

VOID
NTAPI
IoInvalidateDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE Type)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);

    if (!DeviceObject || !DeviceNode ||
        DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)
    {
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                     0x2,
                     (ULONG_PTR)DeviceObject,
                     0x0,
                     0x0);
    }

    switch (Type)
    {
        case BusRelations:
            DPRINT("IoInvalidateDeviceRelations: PipEnumDeviceTree\n");
            PipRequestDeviceAction(DeviceObject,
                                   PipEnumDeviceTree,
                                   0,
                                   0,
                                   NULL,
                                   NULL);
            break;

        case PowerRelations:
            DPRINT1("IoInvalidateDeviceRelations: PowerRelations NOT IMPLEMENTED\n");
            break;

        case SingleBusRelations:
            DPRINT("IoInvalidateDeviceRelations: PipEnumDeviceOnly\n");
            PipRequestDeviceAction(DeviceObject,
                                   PipEnumDeviceOnly,
                                   0,
                                   0,
                                   NULL,
                                   NULL);
            break;

        default:
            DPRINT1("IoInvalidateDeviceRelations: relation type \"%u\" is NOT IMPLEMENTED\n", Type);
            break;
    }
}

NTSTATUS
NTAPI
IoSynchronousInvalidateDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE Type)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
