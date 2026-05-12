#include "uefi.h"

// GUIDs to recognise
static EFI_GUID gEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2,
    {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
};

static EFI_GUID gEfiLoadedImageDevicePathProtocolGuid = {
    0xBC62157E, 0x3E33, 0x4FEC,
    {0x99, 0x20, 0x2D, 0x3B, 0x36, 0xD7, 0x50, 0xDF}
};

static int guid_eq(EFI_GUID* a, EFI_GUID* b) {
    return memcmp(a, b, sizeof(EFI_GUID)) == 0;
}

// The loaded image instance — set this before calling entry()
static EFI_LOADED_IMAGE_PROTOCOL* gLoadedImage = NULL;

static EFI_STATUS EFIAPI efi_HandleProtocol(
    EFI_HANDLE handle,
    EFI_GUID*  protocol,
    VOID**     interface
) {
    if (guid_eq(protocol, &gEfiLoadedImageProtocolGuid)) {
        *interface = gLoadedImage;
        return EFI_SUCCESS;
    }
    serial_puts("[STUB] HandleProtocol: unknown GUID\n");
    *interface = NULL;
    return EFI_NOT_FOUND;
}

static EFI_STATUS EFIAPI efi_OpenProtocol(
    EFI_HANDLE handle,
    EFI_GUID*  protocol,
    VOID**     interface,
    EFI_HANDLE agent,
    EFI_HANDLE controller,
    UINT32     attributes
) {
    if (guid_eq(protocol, &gEfiLoadedImageProtocolGuid)) {
        *interface = gLoadedImage;
        return EFI_SUCCESS;
    }
    if (guid_eq(protocol, &gEfiLoadedImageDevicePathProtocolGuid)) {
        *interface = NULL;   // acceptable — Limine handles null here
        return EFI_NOT_FOUND;
    }
    serial_puts("[STUB] OpenProtocol: unknown GUID\n");
    *interface = NULL;
    return EFI_NOT_FOUND;
}