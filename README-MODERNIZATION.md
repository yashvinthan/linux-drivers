# Accusys ACS6x Driver - Kernel 6.x Modernization

**Version:** 3.2.2 (Modernized)
**Target Kernels:** 6.1 - 6.12
**Target Platform:** TrueNAS SCALE (Debian 12 base, Proxmox-style kernels)
**Hardware:** ExaSAN A08S-PS chassis with Accusys ACS-62xxx PCIe RAID/HBA controller

---

## Overview

This is a **minimally invasive** modernization of the legacy Accusys ACS6x driver
(version 3.2.2 from 2014) to support modern Linux kernels 6.1 through 6.12.

The goal is to preserve all original I/O logic and hardware interaction code while
updating only the kernel API calls that have changed in newer kernels.

---

## What Was Changed

### 1. **New Compatibility Header (`compat.h`)**

A comprehensive compatibility layer that handles kernel API differences:

- **PCI API:** `pci_get_bus_and_slot()` → `pci_get_domain_bus_and_slot()`
- **DMA API:** `pci_set_dma_mask()` → `dma_set_mask_and_coherent()`
- **SCSI midlayer:**
  - Status codes: `QUEUE_FULL` → `SAM_STAT_TASK_SET_FULL`
  - Command completion: `cmd->scsi_done` → `scsi_done(cmd)` wrapper
  - Resid handling via `scsi_get_resid()` / `scsi_set_resid()`
- **Timer API:** Already handled correctly in original code (4.15+ timer_setup)
- **Device class:** `class_create(THIS_MODULE, name)` → `class_create(name)` for 6.4+
- **IOCTL:** Proper unlocked_ioctl prototype handling

All changes are wrapped in `LINUX_VERSION_CODE` checks for backward compatibility.

### 2. **Source Code Modifications**

**Minimal changes to `acs_ame.c`:**
- Changed `#include "modern_kernel_compat.h"` → `#include "compat.h"`
- Updated `class_create()` call to use `acs_class_create()` macro

**No changes needed for:**
- Timer API (already modernized in original)
- SCSI queuecommand (already has 5.16+ support)
- DMA direction constants (handled by compat.h)

### 3. **Build System Updates**

**New `Makefile.new`:**
- Standard Kbuild format for modern kernels
- Proper `make -C /lib/modules/$(uname -r)/build M=$(PWD) modules`
- Clean target removes all build artifacts
- Help target for documentation

**DKMS Support:**
- `dkms.conf` for automatic rebuilding across kernel updates
- Installation instructions in `dkms-postinst.txt`

### 4. **Documentation**

- **BUILD-INSTRUCTIONS-DEBIAN12.md** - Complete build guide
- **TRUENAS-DEPLOYMENT.md** - TrueNAS SCALE deployment steps
- **TESTING-CHECKLIST.md** - Comprehensive testing and troubleshooting
- **acs6x-loader.service** - Systemd unit for auto-loading
- **quick-build.sh** - Automated build script

---

## File Inventory

### Core Driver Files (Original, Unmodified)
- `AME_module.c` / `AME_module.h` - AME module logic
- `AME_Queue.c` / `AME_Queue.h` - Queue management
- `AME_Raid.c` / `AME_Raid.h` - RAID logic
- `MPIO.c` / `MPIO.h` - Multi-path I/O
- `ACS_MSG.c` / `ACS_MSG.h` - Messaging subsystem
- `ame.h` - AME definitions
- `AME_import.h` - Import definitions
- `OS_Define.h` - OS definitions

### Modified Driver Files
- `acs_ame.c` - Main driver file (**minor changes**)
- `acs_ame.h` - Driver header (**macro consolidation**)

### New/Updated Files
- **`compat.h`** - ✨ NEW: Kernel compatibility layer
- **`Makefile.new`** - ✨ NEW: Modernized build system
- `Makefile` - Original (replaced by Makefile.new for builds)
- `modern_kernel_compat.h` - Old compat layer (superseded by compat.h)

### Build and Deployment Files
- **`quick-build.sh`** - ✨ NEW: Automated build script
- **`dkms.conf`** - ✨ NEW: DKMS configuration
- **`dkms-postinst.txt`** - ✨ NEW: DKMS installation guide
- **`acs6x-loader.service`** - ✨ NEW: Systemd unit file

### Documentation Files
- **`README-MODERNIZATION.md`** - ✨ This file
- **`BUILD-INSTRUCTIONS-DEBIAN12.md`** - ✨ NEW: Build guide
- **`TRUENAS-DEPLOYMENT.md`** - ✨ NEW: Deployment guide
- **`TESTING-CHECKLIST.md`** - ✨ NEW: Testing guide
- **`acs6x-kernel-6.x-modernization.patch`** - ✨ NEW: Unified diff

---

## Quick Start

### Build on Debian 12 VM

```bash
# Install prerequisites
sudo apt update
sudo apt install -y build-essential

# Add Proxmox repo for matching headers
wget https://enterprise.proxmox.com/debian/proxmox-release-bookworm.gpg \
  -O /etc/apt/trusted.gpg.d/proxmox-release-bookworm.gpg
echo "deb http://download.proxmox.com/debian/pve bookworm pve-no-subscription" | \
  sudo tee /etc/apt/sources.list.d/pve-no-subscription.list
sudo apt update

# Install kernel headers (match your TrueNAS kernel)
sudo apt install -y pve-headers-6.12.15-1-pve

# Build
cd /root/drivers/Linux_Drv_3.2.2
./quick-build.sh 6.12.15-1-pve

# Result: ACS6x.ko
```

### Deploy to TrueNAS SCALE

```bash
# On TrueNAS
mkdir -p /mnt/tank/system/drivers

# From build VM
scp ACS6x.ko root@<truenas-ip>:/mnt/tank/system/drivers/

# On TrueNAS - Test load
insmod /mnt/tank/system/drivers/ACS6x.ko
dmesg | tail -30
lsmod | grep ACS6x

# Setup persistent loading
cp acs6x-loader.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable acs6x-loader.service
systemctl start acs6x-loader.service

# Verify
lsscsi
lsblk
```

---

## API Changes Summary

### PCI Subsystem

| Old API (Removed) | New API (6.x) | Compatibility |
|-------------------|---------------|---------------|
| `pci_get_bus_and_slot(bus, devfn)` | `pci_get_domain_bus_and_slot(0, bus, devfn)` | compat.h:67 |
| `pci_set_dma_mask(pdev, mask)` | `dma_set_mask(&pdev->dev, mask)` | compat.h:73 |
| `pci_set_consistent_dma_mask(pdev, mask)` | `dma_set_coherent_mask(&pdev->dev, mask)` | compat.h:75 |

### SCSI Subsystem

| Old API (Changed) | New API (6.x) | Compatibility |
|-------------------|---------------|---------------|
| `cmd->scsi_done(cmd)` | `scsi_done(cmd)` | compat.h:102 |
| `cmd->scsi_done = done` | Not needed (callback passed in) | compat.h:107 |
| `QUEUE_FULL` | `SAM_STAT_TASK_SET_FULL` | compat.h:92 |
| `COMMAND_TERMINATED` | `SAM_STAT_TASK_ABORTED` | compat.h:96 |

### Device Class

| Old API (Changed) | New API (6.4+) | Compatibility |
|-------------------|----------------|---------------|
| `class_create(THIS_MODULE, name)` | `class_create(name)` | compat.h:133 |

### Timer API (Already handled in original)

| Old API (Removed) | New API (4.15+) | Status |
|-------------------|-----------------|--------|
| `init_timer()` + `.data` + `.function` | `timer_setup()` + `from_timer()` | ✓ Already updated |

---

## Kernel Version Support Matrix

| Kernel | Status | Notes |
|--------|--------|-------|
| 6.12.x | ✅ Tested | Primary target (TrueNAS SCALE 24.10+) |
| 6.8.x  | ✅ Expected to work | Debian 12 backports |
| 6.6.x  | ✅ Expected to work | Debian 13 (Trixie) |
| 6.1.x  | ✅ Expected to work | Debian 12 LTS kernel |
| 5.18-5.19 | ✅ Should work | PCI_DMA_* removal handled |
| 5.16-5.17 | ✅ Should work | scsi_done() change handled |
| 5.15 LTS | ✅ Should work | Ubuntu 22.04 LTS |
| 5.10 LTS | ⚠️ May work | Some compat paths may need testing |
| < 5.0 | ❌ Unsupported | Would need additional compat code |

---

## Testing Status

### Compile Testing
- ✅ Compiles cleanly against headers: `pve-headers-6.12.15-1-pve`
- ✅ No warnings with `-Wall`
- ✅ Module info shows correct version and license

### Runtime Testing
⚠️ **Requires actual hardware** - Cannot fully test without:
- Accusys ACS-62xxx PCIe card
- ExaSAN A08S-PS chassis
- Physical disk drives

### Expected Runtime Behavior
Based on code analysis, the driver should:
1. Detect Accusys PCI device (vendor 14d6, device 6201)
2. Initialize DMA (prefer 64-bit, fallback 32-bit)
3. Map PCI BAR regions
4. Register SCSI host adapter
5. Enumerate connected disks
6. Register character device for management ioctl
7. Start timer for LED/status updates

---

## Differences from Original

### Intentionally Preserved
- **All hardware I/O logic** - Unchanged
- **RAID/MPIO algorithms** - Unchanged
- **DMA descriptor formats** - Unchanged
- **Interrupt handling** - Unchanged
- **SCSI command translation** - Unchanged

### Intentionally Updated
- **Kernel API calls** - Modernized via compat.h
- **Build system** - Modernized Makefile
- **Documentation** - Extensive new docs
- **Deployment method** - Added systemd support

### Removed/Deprecated
- `modern_kernel_compat.h` - Superseded by `compat.h`
- Old Makefile variable names - Standardized to EXTRA_CFLAGS

---

## Known Limitations

1. **Module Signing:**
   - Built module is unsigned
   - Will trigger "module verification failed" warning on signed kernels
   - Solutions: Sign the module OR disable signature enforcement

2. **Kernel ABI:**
   - Requires exact major.minor.patch match (6.12.15 → 6.12.15)
   - Symbols may differ between `+debug` vs normal builds
   - Usually ABI-compatible despite version string differences

3. **Hardware Support:**
   - Only tested with Accusys ACS-62xxx series
   - May not work with older ACS-61xxx (untested)
   - Requires specific PCIe device IDs (see acs_ame.h:130-155)

4. **TrueNAS Persistence:**
   - System updates may remove systemd unit
   - Recommended: Use TrueNAS Init/Shutdown Scripts (UI-managed)
   - Alternative: Reinstall service after updates

---

## Troubleshooting

### Build Failures

**"No rule to make target 'modules'"**
→ Kernel headers not installed or wrong KERNELDIR path

**"implicit declaration of function 'class_create'"**
→ compat.h not included properly, check `#include "compat.h"`

**"unknown field 'scsi_done' specified in initializer"**
→ Expected for 5.16+, compat.h handles this via wrapper

### Load Failures

**"Invalid module format"**
→ Kernel version mismatch, rebuild with exact matching headers

**"Unknown symbol in module"**
→ Kernel config difference, check `CONFIG_SCSI_SAS_ATTRS` etc.

**"module verification failed"**
→ Module unsigned, see TRUENAS-DEPLOYMENT.md for signing instructions

### Runtime Failures

**Module loads but no disks**
→ Hardware not detected, check `lspci`, cables, power

**Kernel panic on load**
→ Serious compatibility issue, check DMA mask in dmesg

**Disks appear then disappear**
→ Unstable connection, check SAS cables and chassis firmware

---

## Development Notes

### Code Style
Original vendor code style preserved:
- Mix of camelCase and under_score
- 4-space indents (mostly)
- Minimal comments
- Extensive `#ifdef` version checks

### Debugging
Enable debug output:
```bash
make DEBUG=y
# Adds -DACS_DEBUG flag
# Enables DPRINTK() messages in driver
```

View debug output:
```bash
dmesg -w | grep -i acs
# or
journalctl -k -f | grep -i acs
```

### Adding Support for New Kernels

If a future kernel breaks compatibility:

1. Check changed APIs: https://lwn.net/Kernel/Index/#Kernel_development-API_changes
2. Add version check to `compat.h`
3. Create wrapper macro or inline function
4. Test compilation and runtime
5. Update kernel support matrix in this README

Example:
```c
// In compat.h
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,20,0)
#define new_api_wrapper(args) new_api_call(args)
#else
#define new_api_wrapper(args) old_api_call(args)
#endif
```

---

## Credits

- **Original Driver:** Accusys Technology Inc. (2006-2014)
- **Modernization:** Community effort for TrueNAS SCALE compatibility (2025)
- **Testing:** Requires community members with actual hardware

---

## License

Original driver license: **Dual BSD/GPL**
(As specified in MODULE_LICENSE() macro)

This modernization respects the original license and adds no additional restrictions.

---

## Support

This is a community modernization effort. For hardware support:
- **Accusys:** https://www.accusys.com/
- **TrueNAS Forums:** https://forums.truenas.com/

For kernel API questions:
- **Linux Kernel Mailing List:** https://lkml.org/
- **TrueNAS SCALE Documentation:** https://www.truenas.com/docs/scale/

---

## Version History

- **3.2.2 (2014)** - Original vendor release
- **3.2.2-modern (2025)** - Kernel 6.x modernization

---

**Last Updated:** 2025-10-22
**Maintainer:** Community
**Status:** Testing phase - awaiting hardware validation
