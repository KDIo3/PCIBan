#include "Driver.h"
#include "Pci.h"

#include <intrin.h>

#define AHCI_SEARCH_COUNT 64


//
// A found PCI device.
//
typedef struct _PCI_DEVICE
{
	PCI_HEADER_GENERIC Header;
	UINT8 Function;
	UINT8 Device;
	UINT8 Bus;
} PCI_DEVICE;

//
// Reads a block of data from a PCI config.
//
static
BOOLEAN
ReadPciConfig(
	_Out_ PVOID Out,
	_In_ SIZE_T Size,
	_In_ UINT8 Bus,
	_In_ UINT8 Device,
	_In_ UINT8 Function
)
{
	if (Size > 0xff)
	{
		return FALSE;
	}

	PCI_CONFIG_REQUEST request;
	request.Function = Function;
	request.Device = Device;
	request.Bus = Bus;
	request.MustBe1 = 1;

	PCHAR cout = (PCHAR)Out;
	SIZE_T offset = 0;
	while (Size)
	{
		SIZE_T bsize = min(Size, 4);
		request.Offset = (UINT32)offset;

		__outdword(PCI_CONFIG_ADDRESS, request.Flags);

		UINT32 read = __indword(PCI_CONFIG_DATA);
		memcpy(cout, &read, bsize);

		cout += bsize;
		offset += bsize;
		Size -= bsize;
	}

	return TRUE;
}

//
// Analyzes a PCI config.
//
static
BOOLEAN
AnalyzePciConfig(
	_Out_ PCI_DEVICE* Out,
	_In_ UINT8 Bus,
	_In_ UINT8 Device,
	_In_ UINT8 Function
)
{
	if (!ReadPciConfig(&Out->Header, sizeof(PCI_HEADER_GENERIC), Bus, Device, Function))
	{
		return FALSE;
	}

	return Out->Header.VendorId != 0xffff;
}

//
// Finds all PCI devices at a specific bus + device.
//
static
VOID
FindPciDevicesAt(
	_Out_ PCI_DEVICE* Out,
	_Inout_ SIZE_T* OutLen,
	_In_ SIZE_T Max,
	_In_ UINT8 Bus,
	_In_ UINT8 Device
)
{
	PCI_DEVICE generic;
	generic.Bus = Bus;
	generic.Device = Device;
	if (AnalyzePciConfig(&generic, Bus, Device, 0))
	{
		for (UINT8 function = 0; function < PCI_MAX_FUNCTION; function++)
		{
			generic.Function = function;
			if (AnalyzePciConfig(&generic, Bus, Device, function) && *OutLen < Max)
			{
				Out[(*OutLen)++] = generic;
			}
		}
	}
}

//
// Finds all PCI devices using bruteforce.
//
static
VOID
FindPciDevices(
	_Out_ PCI_DEVICE* Out,
	_Inout_ SIZE_T* OutLen
)
{
	SIZE_T max = *OutLen;
	*OutLen = 0;

	for (UINT8 bus = 0; bus < PCI_MAX_BUS; bus++)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Reading PCI bus %d\n", (INT32)bus);

		for (UINT8 device = 0; device < PCI_MAX_DEVICE; device++)
		{
			FindPciDevicesAt(Out, OutLen, max, bus, device);
		}
	}
}

//
// Requests an AHCI serial from an ABAR.
//
static
VOID
RequestAhciSerialAbar(
	_Inout_ PCI_ABAR* Abar,
	_In_ UINT8 Port,
	_In_ PHYSICAL_ADDRESS IdentifyOutput
)
{
	#define AHCI_PORT_START() p->Cmd &= ~HBA_PxCMD_ST; \
		while (p->Cmd & HBA_PxCMD_CR) \
		{ \
			_mm_pause(); \
		} \
		p->Cmd |= HBA_PxCMD_FRE; \
		p->Cmd |= HBA_PxCMD_ST;

	#define AHCI_PORT_STOP() p->Cmd &= ~HBA_PxCMD_ST; \
		while (p->Cmd & HBA_PxCMD_CR) \
		{ \
			_mm_pause(); \
		} \
		p->Cmd &= ~HBA_PxCMD_FRE;


	PCI_ABAR_PORT* p = NULL;
	PHYSICAL_ADDRESS tmpAddr;
	PCI_ABAR_CMD_HEADER* cmdh = NULL;
	PCI_ABAR_CMD_TBL* cmdt = NULL;
	PCI_ABAR_FIS_REG_H2D* cmdfis = NULL;

	p = &Abar->Ports[Port];

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Finding slot\n");
	UINT32 slotMask = (p->Sact | p->Ci);
	INT32 slot = -1;
	for (INT32 i = 0; i < 8 && slot == -1; i++)
	{
		if (!(slotMask & (1 << i)))
		{
			slot = i;
		}
	}

	if (slot == -1)
	{
		goto _cleanup;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Mapping CMD header\n");
	tmpAddr.QuadPart = (UINT64)p->Clb + (slot * sizeof(PCI_ABAR_CMD_HEADER));
	cmdh = (PCI_ABAR_CMD_HEADER*)MmMapIoSpace(tmpAddr, sizeof(PCI_ABAR_CMD_HEADER), MmNonCached);
	if (!cmdh)
	{
		goto _cleanup;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Mapping CMD table\n");
	tmpAddr.QuadPart = cmdh->Ctba;
	cmdt = (PCI_ABAR_CMD_TBL*)MmMapIoSpace(tmpAddr, sizeof(PCI_ABAR_CMD_TBL), MmNonCached);
	if (!cmdt)
	{
		goto _cleanup;
	}

	cmdfis = (PCI_ABAR_FIS_REG_H2D*)cmdt->Cfis;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Setting\n");

	p->Ie = 0xffffffff;
	p->Is = 0x0;
	p->Tfd = 0x0;

	cmdh->Cfl = sizeof(PCI_ABAR_FIS_REG_H2D) / sizeof(UINT32);
	cmdh->W = 0; 
	cmdh->Cfl = 5;
	cmdh->Prdtl = 1;

	cmdt->PrdtEntry[0].Dba = IdentifyOutput.LowPart;
	cmdt->PrdtEntry[0].Dbau = IdentifyOutput.HighPart;
	cmdt->PrdtEntry[0].Dbc = 512 - 1;
	cmdt->PrdtEntry[0].I = 1;

	memset(cmdfis, 0, sizeof(PCI_ABAR_FIS_REG_H2D));
	cmdfis->FisType = FIS_TYPE_REG_H2D;
	cmdfis->C = 1;
	cmdfis->Command = ATA_CMD_IDENTIFY;

	p->Ci |= (1 << slot);
	while (p->Ci & (1 << slot) && !(p->Is & HBA_PxIS_TFES))
	{
		_mm_pause();
	}

	if (p->Is & HBA_PxIS_TFES)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Disk error\n");
		goto _cleanup;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Retrieved!\n");

_cleanup:
	if (cmdh)
	{
		MmUnmapIoSpace(cmdh, sizeof(PCI_ABAR_CMD_HEADER));
	}

	if (cmdt)
	{
		MmUnmapIoSpace(cmdt, sizeof(PCI_ABAR_CMD_TBL));
	}
}

//
// Requests an AHCI serial from a PCI device.
//
static
VOID
RequestAhciSerialDevice(
	_In_ PCI_HEADER_0* Header
)
{
	UINT64 abarAddrRaw = Header->Bar[5];
	if (abarAddrRaw)
	{
		PHYSICAL_ADDRESS abarAddr;
		abarAddr.QuadPart = (UINT64)abarAddrRaw;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Mapping ABAR\n");
		PCI_ABAR* abar = (PCI_ABAR*)MmMapIoSpace(abarAddr, sizeof(PCI_ABAR), MmNonCached);
		if (abar)
		{
			// TODO FIXME calculate num of ports
			IDENTIFY_DEVICE_DATA* buf = (IDENTIFY_DEVICE_DATA*)ExAllocatePool(NonPagedPool, PAGE_SIZE);
			memset(buf, 0, PAGE_SIZE);

			RequestAhciSerialAbar(abar, 0, MmGetPhysicalAddress(buf));

			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Serial= %p\n", buf);
			MmUnmapIoSpace(abar, sizeof(PCI_ABAR));
		}
	}
}

//
// Collects all AHCI serials.
//
static
VOID
CollectAhciSerials(
)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Finding devices\n");
	PCI_DEVICE devices[AHCI_SEARCH_COUNT];
	SIZE_T len = sizeof(PCI_HEADER_GENERIC) * AHCI_SEARCH_COUNT;
	FindPciDevices(devices, &len);

	for (SIZE_T i = 0; i < len; i++)
	{
		PCI_DEVICE device = devices[i];
		if (device.Header.ClassCode == PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER &&
			device.Header.SubClass == PCI_SUBCLASS_SERIAL_ATA &&
			device.Header.ProgIf == PCI_PROGIF_AHCI)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Reading AHCI PCI config\n");

			PCI_HEADER_0 header;
			if (ReadPciConfig(&header, sizeof(header), device.Bus, device.Device, device.Function))
			{
				RequestAhciSerialDevice(&header);
			}
		}
	}
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) 
{
	CollectAhciSerials();
	return STATUS_SUCCESS;
}
