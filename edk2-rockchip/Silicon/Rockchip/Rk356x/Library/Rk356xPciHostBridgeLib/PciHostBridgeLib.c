/** @file
  PCI Host Bridge Library instance for Rockchip Rk356x

  Copyright (c) 2023, Joel Winarske <joel.winarske@gmail.com>
  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#include <IndustryStandard/Rk356x.h>
#include "PciHostBridgeInit.h"


#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A08), // PCI Express
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

STATIC PCI_ROOT_BRIDGE mRootBridgeTemplate = {
  0,
  .Supports    = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
                  EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
                  EFI_PCI_ATTRIBUTE_ISA_IO_16 |
                  EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO |
                  EFI_PCI_ATTRIBUTE_VGA_MEMORY |
                  EFI_PCI_ATTRIBUTE_VGA_IO_16  |
                  EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16,
  .Attributes  = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
                  EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
                  EFI_PCI_ATTRIBUTE_ISA_IO_16 |
                  EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO |
                  EFI_PCI_ATTRIBUTE_VGA_MEMORY |
                  EFI_PCI_ATTRIBUTE_VGA_IO_16  |
                  EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16,
  .DmaAbove4G            = TRUE,
  .NoExtendedConfigSpace = FALSE,
  .ResourceAssigned      = FALSE,
  .AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                            EFI_PCI_HOST_BRIDGE_MEM64_DECODE,
  {
    FixedPcdGet32 (PcdPciBusMin),
    FixedPcdGet32 (PcdPciBusMax),
    0
  },
  {
    FixedPcdGet64 (PcdPciIoBase),
    (FixedPcdGet64 (PcdPciIoBase) + FixedPcdGet64 (PcdPciIoSize) - 1),
    (MAX_UINT64 - FixedPcdGet64 (PcdPciIoTranslation) + 1)
  },
  {
    FixedPcdGet32 (PcdPciMmio32Base),
    (FixedPcdGet32 (PcdPciMmio32Base) + FixedPcdGet32 (PcdPciMmio32Size) - 1),
    0
  },
  {
    FixedPcdGet64 (PcdPciMmio64Base),
    (FixedPcdGet64 (PcdPciMmio64Base) + FixedPcdGet64 (PcdPciMmio64Size) - 1),
    0,
  },
  //
  // No separate ranges for prefetchable and non-prefetchable BARs
  //
  { MAX_UINT64, 0, 0 },
  { MAX_UINT64, 0, 0 },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN *Count
  )
{
  PCI_ROOT_BRIDGE     *RootBridge;
  EFI_STATUS          Status;
  INTN Config[3] = { 0 };

  if (FixedPcdGet32 (PciHostBridge2x1Enable)) {
    Status = InitializePciHost (
      PCIE2X1_APB,
      PCIE2X1_DBI_BASE,
      PCIE2X1_SEGMENT,
      PCIE2X1_S_BASE,
      FixedPcdGet32 (PciHostBridge2x1NumLanes),
      FixedPcdGet32 (PciHostBridge2x1LinkSpeed),
      FixedPcdGet32 (PciHostBridge2x1PowerGpioBank),
      FixedPcdGet32 (PciHostBridge2x1PowerGpioPin),
      FixedPcdGet32 (PciHostBridge2x1ResetGpioBank),
      FixedPcdGet32 (PciHostBridge2x1ResetGpioPin)
    );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "Failed to initialize PciHost 2x1\n"));
      *Count = 0;
      return NULL;
    }
  }

  if (FixedPcdGet32 (PciHostBridge3x1Enable)) {
    Status = InitializePciHost (
      PCIE3X1_APB,
      PCIE3X1_DBI_BASE,
      PCIE3X1_SEGMENT,
      PCIE3X1_S_BASE,
      FixedPcdGet32 (PciHostBridge3x1NumLanes),
      FixedPcdGet32 (PciHostBridge3x1LinkSpeed),
      FixedPcdGet32 (PciHostBridge3x1PowerGpioBank),
      FixedPcdGet32 (PciHostBridge3x1PowerGpioPin),
      FixedPcdGet32 (PciHostBridge3x1ResetGpioBank),
      FixedPcdGet32 (PciHostBridge3x1ResetGpioPin)
    );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "Failed to initialize PciHost 3x1\n"));
      *Count = 0;
      return NULL;
    }
  }

  if (FixedPcdGet32 (PciHostBridge3x2Enable)) {
    Status = InitializePciHost (
      PCIE3X2_APB,
      PCIE3X2_DBI_BASE,
      PCIE3X2_SEGMENT,
      PCIE3X2_S_BASE,
      FixedPcdGet32 (PciHostBridge3x2NumLanes),
      FixedPcdGet32 (PciHostBridge3x2LinkSpeed),
      FixedPcdGet32 (PciHostBridge3x2PowerGpioBank),
      FixedPcdGet32 (PciHostBridge3x2PowerGpioPin),
      FixedPcdGet32 (PciHostBridge3x2ResetGpioBank),
      FixedPcdGet32 (PciHostBridge3x2ResetGpioPin)
    );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "Failed to initialize PciHost 3x2\n"));
      *Count = 0;
      return NULL;
    }
  }

  *Count = FixedPcdGet32 (PciHostBridgeCount);

  RootBridge = AllocateZeroPool (*Count * sizeof(PCI_ROOT_BRIDGE));

  for(UINTN i=0; i < *Count; i++) {

    PCI_ROOT_BRIDGE *bridge = &RootBridge[i];

    CopyMem (bridge, &mRootBridgeTemplate, sizeof(PCI_ROOT_BRIDGE));

    bridge->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)AllocateZeroPool
        (sizeof(EFI_PCI_ROOT_BRIDGE_DEVICE_PATH));
    CopyMem (bridge->DevicePath, &mDevicePathTemplate, sizeof(mDevicePathTemplate));
    ((EFI_PCI_ROOT_BRIDGE_DEVICE_PATH*)bridge->DevicePath)->AcpiDevicePath.UID = i;

    if (!Config[0] && FixedPcdGet32 (PciHostBridge2x1Enable)) {
      Config[0] = 1;
      bridge->Segment = PCIE2X1_SEGMENT;
      continue;
    }
    if (!Config[1] && FixedPcdGet32 (PciHostBridge3x1Enable)) {
      Config[1] = 1;
      bridge->Segment = PCIE3X1_SEGMENT;
      continue;
    }
    if (!Config[2] && FixedPcdGet32 (PciHostBridge3x2Enable)) {
      Config[2] = 1;
      bridge->Segment = PCIE3X2_SEGMENT;
    }
  }

  return RootBridge;
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  )
{
  for (UINTN i=0; i < Count; i++) {
    FreePool (Bridges[i].DevicePath);
  }
  FreePool (Bridges);
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((EFI_D_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((EFI_D_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              (sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr) /
               sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr[0])
               )
              );
      DEBUG ((EFI_D_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((EFI_D_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
