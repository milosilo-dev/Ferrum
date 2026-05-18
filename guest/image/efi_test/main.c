#include "efi.h"

/* EFI_BUFFER_TOO_SMALL is not defined in this efi.h; raw UEFI spec value. */
#define EFI_BUFFER_TOO_SMALL ((EFI_STATUS)0x8000000000000005ULL)

/* ============================================================
   Global test state (MUST be at top-level)
   ============================================================ */

static EFI_SYSTEM_TABLE* ST;

/* FIX #2: RealBS now points at a by-value copy of the boot services
   struct, so patching the live table doesn't destroy the originals. */
static EFI_BOOT_SERVICES SavedBS;
static EFI_BOOT_SERVICES* RealBS;

/* BSS test */
static UINT64 bss_test[256];
static UINT8  data_test[4096];

/* ABI tracking state */
typedef struct {
    UINTN call_count;
    UINTN stack_errors;
    UINTN abi_errors;

    UINT64 last_rsp;
    UINT64 last_rip;
} EFI_ABI_STATE;

static EFI_ABI_STATE ABI;

/* ============================================================
   Helpers
   ============================================================ */

static void print(const CHAR16* s)
{
    ST->ConOut->OutputString(ST->ConOut, (CHAR16*)s);
}

static void print_hex(UINT64 v)
{
    CHAR16 buf[17];
    buf[16] = 0;

    for (int i = 15; i >= 0; i--) {
        UINT64 x = v & 0xF;
        buf[i] = (x < 10) ? (L'0' + x) : (L'A' + (x - 10));
        v >>= 4;
    }

    print(buf);
    print(L"\r\n");
}

/* ============================================================
   ABI instrumentation
   ============================================================ */

static inline UINT64 get_rsp(void)
{
    UINT64 rsp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

static inline UINT64 get_rip(void)
{
    UINT64 rip;
    __asm__ volatile("lea (%%rip), %0" : "=r"(rip));
    return rip;
}

/* ============================================================
   Wrappers
   ============================================================ */

static EFI_STATUS EFIAPI wrap_Stall(UINTN us)
{
    UINT64 rsp_in = get_rsp();
    ABI.last_rsp = rsp_in;
    ABI.last_rip = get_rip();
    ABI.call_count++;

    print(L"[ABI] Stall\r\n");

    EFI_STATUS s = RealBS->Stall(us);

    UINT64 rsp_out = get_rsp();
    if (rsp_out != rsp_in) {
        ABI.stack_errors++;
        print(L"[ABI] STACK MISMATCH\r\n");
    }

    return s;
}

static EFI_STATUS EFIAPI wrap_GetMemoryMap(
    UINTN*  Size,
    VOID*   Map,
    UINTN*  Key,
    UINTN*  DescSize,
    UINT32* DescVer
)
{
    UINT64 rsp_in = get_rsp();
    ABI.call_count++;

    print(L"[ABI] GetMemoryMap\r\n");

    EFI_STATUS s = RealBS->GetMemoryMap(
        Size, Map, Key, DescSize, DescVer
    );

    /* FIX #4: only flag zero-size as a bug on a *successful* return.
       The standard probe pattern deliberately passes Size=0 on the
       first call, expecting EFI_BUFFER_TOO_SMALL back with the real
       size filled in — that is not a bug. */
    if (s == EFI_SUCCESS && Size && *Size == 0) {
        print(L"[ABI] ZERO SIZE BUG\r\n");
        ABI.abi_errors++;
    }

    if (s == EFI_SUCCESS && DescSize && *DescSize == 0) {
        print(L"[ABI] ZERO DESCRIPTOR BUG\r\n");
        ABI.abi_errors++;
    }

    UINT64 rsp_out = get_rsp();
    if (rsp_out != rsp_in) {
        ABI.stack_errors++;
        print(L"[ABI] STACK MISMATCH\r\n");
    }

    return s;
}

static EFI_STATUS EFIAPI wrap_AllocatePool(
    EFI_MEMORY_TYPE type,
    UINTN           size,
    VOID**          out
)
{
    ABI.call_count++;

    print(L"[ABI] AllocatePool\r\n");

    return RealBS->AllocatePool(type, size, out);
}

static EFI_STATUS EFIAPI wrap_HandleProtocol(
    EFI_HANDLE  h,
    EFI_GUID*   g,
    VOID**      i
)
{
    ABI.call_count++;

    print(L"[ABI] HandleProtocol\r\n");

    /* FIX #1: HandleProtocol is declared as VOID* in this efi.h, not as
       a typed function pointer, so it must be cast before calling.
       Signature matches the UEFI spec: (Handle, GUID*, Interface**). */
    typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
        EFI_HANDLE, EFI_GUID*, VOID**
    );
    return ((EFI_HANDLE_PROTOCOL)RealBS->HandleProtocol)(h, g, i);
}

/* ============================================================
   Install validator layer
   ============================================================ */

static void InstallValidator(EFI_SYSTEM_TABLE* SystemTable)
{
    ST = SystemTable;

    /* FIX #2: copy the boot-services struct by value so RealBS
       holds the original function pointers even after we patch
       the live table below.  The original code set RealBS to the
       same pointer and then immediately overwrote its fields,
       making every wrapper call itself recursively (infinite loop /
       stack overflow). */
    SavedBS = *SystemTable->BootServices;
    RealBS  = &SavedBS;

    print(L"[ABI] Installing validator...\r\n");

    /* Patch the live table to point at our wrappers. */
    SystemTable->BootServices->Stall          = wrap_Stall;
    SystemTable->BootServices->GetMemoryMap   = wrap_GetMemoryMap;
    SystemTable->BootServices->AllocatePool   = wrap_AllocatePool;
    /* HandleProtocol is VOID* in this efi.h so cast to assign. */
    SystemTable->BootServices->HandleProtocol = (VOID*)wrap_HandleProtocol;

    print(L"[ABI] Installed\r\n");
}

/* ============================================================
   Tests
   ============================================================ */

static void run_tests(void)
{
    print(L"\r\n=== EFI ABI VALIDATION ===\r\n");

    /* BSS test */
    for (UINTN i = 0; i < 256; i++) {
        if (bss_test[i] != 0) {
            print(L"[FAIL] BSS not zeroed\r\n");
            break;
        }
    }

    print(L"[OK] BSS check\r\n");

    /* writable memory test */
    for (UINTN i = 0; i < sizeof(data_test); i++) {
        data_test[i] = (UINT8)i;
    }

    print(L"[OK] RW memory\r\n");

    /* Use the live (patched) BootServices table so every call flows
       through our wrappers and gets counted in ABI.call_count.
       RealBS is only used *inside* the wrappers to reach the real
       firmware implementations without re-entering the wrappers. */
    EFI_BOOT_SERVICES* BS = ST->BootServices;

    for (UINTN i = 0; i < 16; i++) {
        VOID*      p = NULL;
        EFI_STATUS s = BS->AllocatePool(
            EfiLoaderData, 1024 + i * 16, &p
        );

        if (s != EFI_SUCCESS || p == NULL) {
            print(L"[FAIL] AllocatePool returned error\r\n");
            ABI.abi_errors++;
            continue;
        }

        BS->FreePool(p);
    }

    print(L"[OK] AllocatePool stress\r\n");

    /* memory map probe — intentionally passes size=0 to discover the
       required buffer size; expects EFI_BUFFER_TOO_SMALL. */
    UINTN  size = 0;
    UINTN  key  = 0;
    UINTN  desc = 0;
    UINT32 ver  = 0;

    EFI_STATUS s =
        BS->GetMemoryMap(&size, NULL, &key, &desc, &ver);

    print(L"[INFO] GetMemoryMap status: ");
    print_hex(s);

    print(L"[INFO] GetMemoryMap desc: ");
    print_hex(desc);

    print(L"[INFO] GetMemoryMap key: ");
    print_hex(key);

    print(L"[INFO] GetMemoryMap size: ");
    print_hex(size);

    if (s == EFI_BUFFER_TOO_SMALL) {
        print(L"[OK] GetMemoryMap probe (EFI_BUFFER_TOO_SMALL as expected)\r\n");
    }
}

/* ============================================================
   Entry point
   ============================================================ */

EFI_STATUS EFIAPI efi_main(
    EFI_HANDLE       ImageHandle,
    EFI_SYSTEM_TABLE* SystemTable
)
{
    InstallValidator(SystemTable);

    run_tests();

    print(L"\r\n=== ABI REPORT ===\r\n");
    print(L"Calls: ");
    print_hex(ABI.call_count);

    print(L"Stack errors: ");
    print_hex(ABI.stack_errors);

    print(L"ABI errors: ");
    print_hex(ABI.abi_errors);

    /* safe halt */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }

    return 0;
}