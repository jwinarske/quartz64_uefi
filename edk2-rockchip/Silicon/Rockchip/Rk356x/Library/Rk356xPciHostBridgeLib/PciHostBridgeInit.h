/** @file

  Copyright 2021-2023, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIHOSTBRIDGEINIT_H__
#define PCIHOSTBRIDGEINIT_H__

#define PCIE_SEGMENT_PCIE20             0
#define PCIE_SEGMENT_PCIE30X1           1
#define PCIE_SEGMENT_PCIE30X2           2


EFI_STATUS
InitializePciHost (
  EFI_PHYSICAL_ADDRESS     ApbBase,
  EFI_PHYSICAL_ADDRESS     DbiBase,
  EFI_PHYSICAL_ADDRESS     PcieSegment,
  EFI_PHYSICAL_ADDRESS     BaseAddress,
  UINT32                   NumLanes,
  UINT32                   LinkSpeed,
  UINT32                   PowerGpioBank,
  UINT32                   PowerGpioPin,
  UINT32                   ResetGpioBank,
  UINT32                   ResetGpioPin
  );

#endif /* PCIHOSTBRIDGEINIT_H__ */