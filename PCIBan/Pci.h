#pragma once
#include <fltKernel.h>

#define PCI_CONFIG_MAX_SIZE 0x40

#define PCI_MAX_BUS 0xff
#define PCI_MAX_DEVICE 32
#define PCI_MAX_FUNCTION 8

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_CLASS_CODE_UNCLASSIFIED 0x0
#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER 0x1
#define PCI_CLASS_CODE_NETWORK_CONTROLLER 0x2
#define PCI_CLASS_COD_DISPLAY_CONTROLLER 0x3

#define PCI_SUBCLASS_SERIAL_ATA 0x6

#define PCI_PROGIF_AHCI 0x1

#define HBA_PI_ACTIVE (1 << 0)
#define HBA_PI_PRESENT (3 << 8)

#define HBA_PxCMD_ST (1 << 0)
#define HBA_PxCMD_FRE (1 << 4)
#define HBA_PxCMD_FR (1 << 14)
#define HBA_PxCMD_CR (1 << 15)

#define ATA_DEV_BUSY (1 << 7)
#define ATA_DEV_DRQ (1 << 3)

#define HBA_PxIS_TFES (1 << 30)

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY 0xEC

typedef enum
{
	FIS_TYPE_REG_H2D = 0x27,
	FIS_TYPE_REG_D2H = 0x34,
	FIS_TYPE_DMA_ACT = 0x39,
	FIS_TYPE_DMA_SETUP = 0x41,
	FIS_TYPE_DATA = 0x46,
	FIS_TYPE_BIST = 0x58,
	FIS_TYPE_PIO_SETUP = 0x5F,
	FIS_TYPE_DEV_BITS = 0xA1,
} FIS_TYPE;

#pragma pack(push, 1)
typedef struct _PCI_HEADER_GENERIC
{
	UINT16 VendorId;
	UINT16 DeviceId;

	UINT16 Command;
	UINT16 Status;

	UINT8 RevisionId;
	UINT8 ProgIf;
	UINT8 SubClass;
	UINT8 ClassCode;

	UINT8 CacheLineSize;
	UINT8 LatencyTimer;
	UINT8 HeaderType;
	UINT8 Bist;
} PCI_HEADER_GENERIC;

typedef struct _PCI_HEADER_0
{
	PCI_HEADER_GENERIC Generic;

	UINT32 Bar[6];
	UINT32 CardBusCisPointer;

	UINT16 SubSystemVendorId;
	UINT16 SubSystemId;

	UINT32 ExpansionRomBaseAddress;
	UINT32 Reserved1; // FIXME capabilities bits
	UINT32 Reserved2;

	UINT8 InterruptLine;
	UINT8 InterruptPin;
	UINT8 MinGrant;
	UINT8 MaxLatency;
} PCI_HEADER_0;

typedef struct _PCI_CONFIG_REQUEST
{
	union
	{
		struct
		{

			UINT32 Offset : 8; // first 2 bytes must be 0 (i.e. 4 byte alignment)
			UINT32 Function : 3;
			UINT32 Device : 5;
			UINT32 Bus : 15;
			UINT32 MustBe1 : 1;
		};
		UINT32 Flags;
	};
} PCI_CONFIG_REQUEST;

typedef volatile struct _PCI_ABAR_PORT
{
	UINT64 Clb;
	UINT32 Fb;
	UINT32 Fbu;
	UINT32 Is;
	UINT32 Ie;
	UINT32 Cmd;
	UINT32 Rsv0;
	UINT32 Tfd;
	UINT32 Sig;
	UINT32 Ssts;
	UINT32 Sctl;
	UINT32 Serr;
	UINT32 Sact;
	UINT32 Ci;
	UINT32 Sntf;
	UINT32 Fbs;
	UINT32 Rsv1[11];
	UINT32 Vendor[4];
} PCI_ABAR_PORT;

typedef struct _PCI_ABAR_CMD_HEADER
{
	UINT8 Cfl : 5;
	UINT8 A : 1;
	UINT8 W : 1;
	UINT8 P : 1;
	UINT8 R : 1;
	UINT8 B : 1;
	UINT8 C : 1;
	UINT8 Rsv0 : 1;
	UINT8 Pmp : 4;
	UINT16 Prdtl;
	volatile UINT32 Prdbc;
	UINT64 Ctba;
	UINT32 Rsv1[4];
} PCI_ABAR_CMD_HEADER;

typedef struct _PCI_ABAR_FIS_REG_H2D
{
	UINT8 FisType;
	UINT8 Pmport : 4;
	UINT8 Rsv0 : 3;
	UINT8 C : 1;
	UINT8 Command;
	UINT8 Featurel;
	UINT8 Lba0;
	UINT8 Lba1;
	UINT8 Lba2;
	UINT8 Device;
	UINT8 Lba3;
	UINT8 Lba4;
	UINT8 Lba5;
	UINT8 Featureh;
	UINT8 Countl;
	UINT8 Counth;
	UINT8 Icc;
	UINT8 Control;
	UINT8 Rsv1[4];
} PCI_ABAR_FIS_REG_H2D;

typedef struct _PCI_ABAR_PRDT_ENTRY
{
	UINT32 Dba;
	UINT32 Dbau;
	UINT32 Rsv0;
	UINT32 Dbc : 22;
	UINT32 Rsv1 : 9;
	UINT32 I : 1;
} PCI_ABAR_PRDT_ENTRY;


typedef struct _PCI_ABAR_CMD_TBL
{
	CHAR Cfis[64];
	UINT8 Acmd[16];
	UINT8 Rsv[48];
	PCI_ABAR_PRDT_ENTRY PrdtEntry[1];
} PCI_ABAR_CMD_TBL;

typedef struct _PCI_ABAR
{
	UINT32 Cap;
	UINT32 Ghc;
	UINT32 Is;
	UINT32 Pi;
	UINT32 Vs;
	UINT32 CccCtl;
	UINT32 CccPts;
	UINT32 EmLoc;
	UINT32 EmCtl;
	UINT32 Cap2;
	UINT32 Bohc;
	UINT8 Rsv[0xA0 - 0x2C];
	UINT8 Vendor[0x100 - 0xA0];
	PCI_ABAR_PORT Ports[32];
} PCI_ABAR;
static_assert(FIELD_OFFSET(PCI_ABAR, Ports) == 0x100, "Invalid offset");


typedef struct _IDENTIFY_DEVICE_DATA
{
	struct
	{
		USHORT Reserved1 : 1;
		USHORT Retired3 : 1;
		USHORT ResponseIncomplete : 1;
		USHORT Retired2 : 3;
		USHORT FixedDevice : 1;
		USHORT RemovableMedia : 1;
		USHORT Retired1 : 7;
		USHORT DeviceType : 1;
	} GeneralConfiguration;
	USHORT NumCylinders;
	USHORT SpecificConfiguration;
	USHORT NumHeads;
	USHORT Retired1[2];
	USHORT NumSectorsPerTrack;
	USHORT VendorUnique1[3];
	UCHAR  SerialNumber[20];
	USHORT Retired2[2];
	USHORT Obsolete1;
	UCHAR  FirmwareRevision[8];
	UCHAR  ModelNumber[40];
	UCHAR  MaximumBlockTransfer;
	UCHAR  VendorUnique2;
	struct
	{
		USHORT FeatureSupported : 1;
		USHORT Reserved : 15;
	} TrustedComputing;
	struct
	{
		UCHAR  CurrentLongPhysicalSectorAlignment : 2;
		UCHAR  ReservedByte49 : 6;
		UCHAR  DmaSupported : 1;
		UCHAR  LbaSupported : 1;
		UCHAR  IordyDisable : 1;
		UCHAR  IordySupported : 1;
		UCHAR  Reserved1 : 1;
		UCHAR  StandybyTimerSupport : 1;
		UCHAR  Reserved2 : 2;
		USHORT ReservedWord50;
	} Capabilities;
	USHORT ObsoleteWords51[2];
	USHORT TranslationFieldsValid : 3;
	USHORT Reserved3 : 5;
	USHORT FreeFallControlSensitivity : 8;
	USHORT NumberOfCurrentCylinders;
	USHORT NumberOfCurrentHeads;
	USHORT CurrentSectorsPerTrack;
	ULONG  CurrentSectorCapacity;
	UCHAR  CurrentMultiSectorSetting;
	UCHAR  MultiSectorSettingValid : 1;
	UCHAR  ReservedByte59 : 3;
	UCHAR  SanitizeFeatureSupported : 1;
	UCHAR  CryptoScrambleExtCommandSupported : 1;
	UCHAR  OverwriteExtCommandSupported : 1;
	UCHAR  BlockEraseExtCommandSupported : 1;
	ULONG  UserAddressableSectors;
	USHORT ObsoleteWord62;
	USHORT MultiWordDMASupport : 8;
	USHORT MultiWordDMAActive : 8;
	USHORT AdvancedPIOModes : 8;
	USHORT ReservedByte64 : 8;
	USHORT MinimumMWXferCycleTime;
	USHORT RecommendedMWXferCycleTime;
	USHORT MinimumPIOCycleTime;
	USHORT MinimumPIOCycleTimeIORDY;
	struct
	{
		USHORT ZonedCapabilities : 2;
		USHORT NonVolatileWriteCache : 1;
		USHORT ExtendedUserAddressableSectorsSupported : 1;
		USHORT DeviceEncryptsAllUserData : 1;
		USHORT ReadZeroAfterTrimSupported : 1;
		USHORT Optional28BitCommandsSupported : 1;
		USHORT IEEE1667 : 1;
		USHORT DownloadMicrocodeDmaSupported : 1;
		USHORT SetMaxSetPasswordUnlockDmaSupported : 1;
		USHORT WriteBufferDmaSupported : 1;
		USHORT ReadBufferDmaSupported : 1;
		USHORT DeviceConfigIdentifySetDmaSupported : 1;
		USHORT LPSAERCSupported : 1;
		USHORT DeterministicReadAfterTrimSupported : 1;
		USHORT CFastSpecSupported : 1;
	} AdditionalSupported;
	USHORT ReservedWords70[5];
	USHORT QueueDepth : 5;
	USHORT ReservedWord75 : 11;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT SataGen1 : 1;
		USHORT SataGen2 : 1;
		USHORT SataGen3 : 1;
		USHORT Reserved1 : 4;
		USHORT NCQ : 1;
		USHORT HIPM : 1;
		USHORT PhyEvents : 1;
		USHORT NcqUnload : 1;
		USHORT NcqPriority : 1;
		USHORT HostAutoPS : 1;
		USHORT DeviceAutoPS : 1;
		USHORT ReadLogDMA : 1;
		USHORT Reserved2 : 1;
		USHORT CurrentSpeed : 3;
		USHORT NcqStreaming : 1;
		USHORT NcqQueueMgmt : 1;
		USHORT NcqReceiveSend : 1;
		USHORT DEVSLPtoReducedPwrState : 1;
		USHORT Reserved3 : 8;
	} SerialAtaCapabilities;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT NonZeroOffsets : 1;
		USHORT DmaSetupAutoActivate : 1;
		USHORT DIPM : 1;
		USHORT InOrderData : 1;
		USHORT HardwareFeatureControl : 1;
		USHORT SoftwareSettingsPreservation : 1;
		USHORT NCQAutosense : 1;
		USHORT DEVSLP : 1;
		USHORT HybridInformation : 1;
		USHORT Reserved1 : 6;
	} SerialAtaFeaturesSupported;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT NonZeroOffsets : 1;
		USHORT DmaSetupAutoActivate : 1;
		USHORT DIPM : 1;
		USHORT InOrderData : 1;
		USHORT HardwareFeatureControl : 1;
		USHORT SoftwareSettingsPreservation : 1;
		USHORT DeviceAutoPS : 1;
		USHORT DEVSLP : 1;
		USHORT HybridInformation : 1;
		USHORT Reserved1 : 6;
	} SerialAtaFeaturesEnabled;
	USHORT MajorRevision;
	USHORT MinorRevision;
	struct
	{
		USHORT SmartCommands : 1;
		USHORT SecurityMode : 1;
		USHORT RemovableMediaFeature : 1;
		USHORT PowerManagement : 1;
		USHORT Reserved1 : 1;
		USHORT WriteCache : 1;
		USHORT LookAhead : 1;
		USHORT ReleaseInterrupt : 1;
		USHORT ServiceInterrupt : 1;
		USHORT DeviceReset : 1;
		USHORT HostProtectedArea : 1;
		USHORT Obsolete1 : 1;
		USHORT WriteBuffer : 1;
		USHORT ReadBuffer : 1;
		USHORT Nop : 1;
		USHORT Obsolete2 : 1;
		USHORT DownloadMicrocode : 1;
		USHORT DmaQueued : 1;
		USHORT Cfa : 1;
		USHORT AdvancedPm : 1;
		USHORT Msn : 1;
		USHORT PowerUpInStandby : 1;
		USHORT ManualPowerUp : 1;
		USHORT Reserved2 : 1;
		USHORT SetMax : 1;
		USHORT Acoustics : 1;
		USHORT BigLba : 1;
		USHORT DeviceConfigOverlay : 1;
		USHORT FlushCache : 1;
		USHORT FlushCacheExt : 1;
		USHORT WordValid83 : 2;
		USHORT SmartErrorLog : 1;
		USHORT SmartSelfTest : 1;
		USHORT MediaSerialNumber : 1;
		USHORT MediaCardPassThrough : 1;
		USHORT StreamingFeature : 1;
		USHORT GpLogging : 1;
		USHORT WriteFua : 1;
		USHORT WriteQueuedFua : 1;
		USHORT WWN64Bit : 1;
		USHORT URGReadStream : 1;
		USHORT URGWriteStream : 1;
		USHORT ReservedForTechReport : 2;
		USHORT IdleWithUnloadFeature : 1;
		USHORT WordValid : 2;
	} CommandSetSupport;
	struct
	{
		USHORT SmartCommands : 1;
		USHORT SecurityMode : 1;
		USHORT RemovableMediaFeature : 1;
		USHORT PowerManagement : 1;
		USHORT Reserved1 : 1;
		USHORT WriteCache : 1;
		USHORT LookAhead : 1;
		USHORT ReleaseInterrupt : 1;
		USHORT ServiceInterrupt : 1;
		USHORT DeviceReset : 1;
		USHORT HostProtectedArea : 1;
		USHORT Obsolete1 : 1;
		USHORT WriteBuffer : 1;
		USHORT ReadBuffer : 1;
		USHORT Nop : 1;
		USHORT Obsolete2 : 1;
		USHORT DownloadMicrocode : 1;
		USHORT DmaQueued : 1;
		USHORT Cfa : 1;
		USHORT AdvancedPm : 1;
		USHORT Msn : 1;
		USHORT PowerUpInStandby : 1;
		USHORT ManualPowerUp : 1;
		USHORT Reserved2 : 1;
		USHORT SetMax : 1;
		USHORT Acoustics : 1;
		USHORT BigLba : 1;
		USHORT DeviceConfigOverlay : 1;
		USHORT FlushCache : 1;
		USHORT FlushCacheExt : 1;
		USHORT Resrved3 : 1;
		USHORT Words119_120Valid : 1;
		USHORT SmartErrorLog : 1;
		USHORT SmartSelfTest : 1;
		USHORT MediaSerialNumber : 1;
		USHORT MediaCardPassThrough : 1;
		USHORT StreamingFeature : 1;
		USHORT GpLogging : 1;
		USHORT WriteFua : 1;
		USHORT WriteQueuedFua : 1;
		USHORT WWN64Bit : 1;
		USHORT URGReadStream : 1;
		USHORT URGWriteStream : 1;
		USHORT ReservedForTechReport : 2;
		USHORT IdleWithUnloadFeature : 1;
		USHORT Reserved4 : 2;
	} CommandSetActive;
	USHORT UltraDMASupport : 8;
	USHORT UltraDMAActive : 8;
	struct
	{
		USHORT TimeRequired : 15;
		USHORT ExtendedTimeReported : 1;
	} NormalSecurityEraseUnit;
	struct
	{
		USHORT TimeRequired : 15;
		USHORT ExtendedTimeReported : 1;
	} EnhancedSecurityEraseUnit;
	USHORT CurrentAPMLevel : 8;
	USHORT ReservedWord91 : 8;
	USHORT MasterPasswordID;
	USHORT HardwareResetResult;
	USHORT CurrentAcousticValue : 8;
	USHORT RecommendedAcousticValue : 8;
	USHORT StreamMinRequestSize;
	USHORT StreamingTransferTimeDMA;
	USHORT StreamingAccessLatencyDMAPIO;
	ULONG  StreamingPerfGranularity;
	ULONG  Max48BitLBA[2];
	USHORT StreamingTransferTime;
	USHORT DsmCap;
	struct
	{
		USHORT LogicalSectorsPerPhysicalSector : 4;
		USHORT Reserved0 : 8;
		USHORT LogicalSectorLongerThan256Words : 1;
		USHORT MultipleLogicalSectorsPerPhysicalSector : 1;
		USHORT Reserved1 : 2;
	} PhysicalLogicalSectorSize;
	USHORT InterSeekDelay;
	USHORT WorldWideName[4];
	USHORT ReservedForWorldWideName128[4];
	USHORT ReservedForTlcTechnicalReport;
	USHORT WordsPerLogicalSector[2];
	struct
	{
		USHORT ReservedForDrqTechnicalReport : 1;
		USHORT WriteReadVerify : 1;
		USHORT WriteUncorrectableExt : 1;
		USHORT ReadWriteLogDmaExt : 1;
		USHORT DownloadMicrocodeMode3 : 1;
		USHORT FreefallControl : 1;
		USHORT SenseDataReporting : 1;
		USHORT ExtendedPowerConditions : 1;
		USHORT Reserved0 : 6;
		USHORT WordValid : 2;
	} CommandSetSupportExt;
	struct
	{
		USHORT ReservedForDrqTechnicalReport : 1;
		USHORT WriteReadVerify : 1;
		USHORT WriteUncorrectableExt : 1;
		USHORT ReadWriteLogDmaExt : 1;
		USHORT DownloadMicrocodeMode3 : 1;
		USHORT FreefallControl : 1;
		USHORT SenseDataReporting : 1;
		USHORT ExtendedPowerConditions : 1;
		USHORT Reserved0 : 6;
		USHORT Reserved1 : 2;
	} CommandSetActiveExt;
	USHORT ReservedForExpandedSupportandActive[6];
	USHORT MsnSupport : 2;
	USHORT ReservedWord127 : 14;
	struct
	{
		USHORT SecuritySupported : 1;
		USHORT SecurityEnabled : 1;
		USHORT SecurityLocked : 1;
		USHORT SecurityFrozen : 1;
		USHORT SecurityCountExpired : 1;
		USHORT EnhancedSecurityEraseSupported : 1;
		USHORT Reserved0 : 2;
		USHORT SecurityLevel : 1;
		USHORT Reserved1 : 7;
	} SecurityStatus;
	USHORT ReservedWord129[31];
	struct
	{
		USHORT MaximumCurrentInMA : 12;
		USHORT CfaPowerMode1Disabled : 1;
		USHORT CfaPowerMode1Required : 1;
		USHORT Reserved0 : 1;
		USHORT Word160Supported : 1;
	} CfaPowerMode1;
	USHORT ReservedForCfaWord161[7];
	USHORT NominalFormFactor : 4;
	USHORT ReservedWord168 : 12;
	struct
	{
		USHORT SupportsTrim : 1;
		USHORT Reserved0 : 15;
	} DataSetManagementFeature;
	USHORT AdditionalProductID[4];
	USHORT ReservedForCfaWord174[2];
	USHORT CurrentMediaSerialNumber[30];
	struct
	{
		USHORT Supported : 1;
		USHORT Reserved0 : 1;
		USHORT WriteSameSuported : 1;
		USHORT ErrorRecoveryControlSupported : 1;
		USHORT FeatureControlSuported : 1;
		USHORT DataTablesSuported : 1;
		USHORT Reserved1 : 6;
		USHORT VendorSpecific : 4;
	} SCTCommandTransport;
	USHORT ReservedWord207[2];
	struct
	{
		USHORT AlignmentOfLogicalWithinPhysical : 14;
		USHORT Word209Supported : 1;
		USHORT Reserved0 : 1;
	} BlockAlignment;
	USHORT WriteReadVerifySectorCountMode3Only[2];
	USHORT WriteReadVerifySectorCountMode2Only[2];
	struct
	{
		USHORT NVCachePowerModeEnabled : 1;
		USHORT Reserved0 : 3;
		USHORT NVCacheFeatureSetEnabled : 1;
		USHORT Reserved1 : 3;
		USHORT NVCachePowerModeVersion : 4;
		USHORT NVCacheFeatureSetVersion : 4;
	} NVCacheCapabilities;
	USHORT NVCacheSizeLSW;
	USHORT NVCacheSizeMSW;
	USHORT NominalMediaRotationRate;
	USHORT ReservedWord218;
	struct
	{
		UCHAR NVCacheEstimatedTimeToSpinUpInSeconds;
		UCHAR Reserved;
	} NVCacheOptions;
	USHORT WriteReadVerifySectorCountMode : 8;
	USHORT ReservedWord220 : 8;
	USHORT ReservedWord221;
	struct
	{
		USHORT MajorVersion : 12;
		USHORT TransportType : 4;
	} TransportMajorVersion;
	USHORT TransportMinorVersion;
	USHORT ReservedWord224[6];
	ULONG  ExtendedNumberOfUserAddressableSectors[2];
	USHORT MinBlocksPerDownloadMicrocodeMode03;
	USHORT MaxBlocksPerDownloadMicrocodeMode03;
	USHORT ReservedWord236[19];
	USHORT Signature : 8;
	USHORT CheckSum : 8;
} IDENTIFY_DEVICE_DATA, *PIDENTIFY_DEVICE_DATA;

#pragma pack(pop)