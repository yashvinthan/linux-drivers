# TrueNAS SCALE Deployment Guide for Accusys ACS6x Driver

This guide covers deploying the compiled ACS6x.ko module on TrueNAS SCALE systems.

## Important Notes

- TrueNAS SCALE root filesystem is **read-only** by default
- Drivers must be loaded from writable datasets (ZFS pools)
- Boot-persistent loading requires systemd units or Init/Shutdown scripts
- Module must match or be ABI-compatible with running kernel

## Prerequisites

- TrueNAS SCALE installed and accessible (SSH enabled)
- Compiled ACS6x.ko module (from BUILD-INSTRUCTIONS-DEBIAN12.md)
- ZFS pool available for storing driver
- Root SSH access

## Step 1: Check TrueNAS Kernel Version

SSH into your TrueNAS SCALE system:

```bash
ssh root@<truenas-ip>

# Check kernel version
uname -r
# Example output: 6.12.15-debug+truenas
```

**Verify ABI compatibility:** The module compiled against `6.12.15-1-pve` headers
is ABI-compatible with `6.12.15-debug+truenas`. Major.minor.patch must match.

## Step 2: Create Persistent Storage Location

```bash
# Create directory on a ZFS dataset (replace 'tank' with your pool name)
mkdir -p /mnt/tank/system/drivers

# Alternative: Create a dedicated dataset
zfs create tank/system
zfs create tank/system/drivers
```

## Step 3: Transfer and Install Module

### Option A: SCP Transfer

From your build machine:
```bash
scp ACS6x.ko root@<truenas-ip>:/mnt/tank/system/drivers/
```

### Option B: Upload via TrueNAS UI

1. Navigate to Datasets → Create dataset named "system"
2. Use Shell or web file manager to upload ACS6x.ko
3. Move to /mnt/tank/system/drivers/

### Verify Transfer

```bash
ls -lh /mnt/tank/system/drivers/ACS6x.ko
# Should show file with size ~1.2MB

# Check module info
modinfo /mnt/tank/system/drivers/ACS6x.ko
```

## Step 4: Manual Test Load

Before setting up automatic loading, test manually:

```bash
# Attempt to load the module
insmod /mnt/tank/system/drivers/ACS6x.ko

# Check if loaded
lsmod | grep ACS6x

# Check kernel log for probe messages
dmesg | grep -i acs | tail -50

# If you see your Accusys PCI device detected, SUCCESS!
# Look for messages like:
#   "ACS MSG: ACS_Driver_Initialization"
#   "ACS MSG: Start ACS_Timer_Func"

# Check PCI devices
lspci -nn | grep -i accusys
# Should show something like:
# 01:00.0 RAID bus controller [0104]: Accusys, Inc. Device [14d6:6201]
```

### If Load Fails

```bash
# Check detailed error
dmesg | tail -50

# Common issues:
# 1. "version magic" mismatch → rebuild with exact kernel headers
# 2. "Unknown symbol" → missing kernel config options
# 3. "Operation not permitted" → disable signature verification (see below)
```

## Step 5: Create Persistent Loader Service

Create a systemd unit to load the driver at boot.

```bash
# Create systemd service file
cat > /mnt/tank/system/drivers/acs6x-loader.service << 'EOF'
[Unit]
Description=Load Accusys ACS6x RAID Driver
DefaultDependencies=no
After=local-fs.target
Before=zfs-import.target

[Service]
Type=oneshot
ExecStart=/sbin/insmod /mnt/tank/system/drivers/ACS6x.ko
ExecStop=/sbin/rmmod ACS6x
RemainAfterExit=yes
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Copy to systemd directory
cp /mnt/tank/system/drivers/acs6x-loader.service /etc/systemd/system/

# Enable the service
systemctl daemon-reload
systemctl enable acs6x-loader.service

# Test the service
systemctl start acs6x-loader.service
systemctl status acs6x-loader.service

# Verify module loaded
lsmod | grep ACS6x
```

## Step 6: Verify Disk Detection

```bash
# Check block devices
lsblk

# You should now see your ExaSAN disks, typically as /dev/sdX

# Check SCSI devices
ls -l /sys/class/scsi_host/
cat /sys/class/scsi_host/host*/proc_name
# Should include "ACS6x"

# List all SCSI devices
lsscsi
# Should show devices attached through ACS6x host adapter

# Check TrueNAS disk detection
midclt call disk.query
# Should list your ExaSAN disks
```

## Step 7: TrueNAS Web UI Verification

1. Log into TrueNAS web interface
2. Navigate to **Storage** → **Disks**
3. Verify your ExaSAN disks appear in the list
4. Check disk serial numbers match your hardware

## Step 8: Make Service Survive TrueNAS Updates

TrueNAS SCALE may overwrite /etc/systemd/system during updates. Use a Init/Shutdown Script instead:

### Method A: Using TrueNAS Init/Shutdown Scripts (Recommended)

1. Log into TrueNAS Web UI
2. Go to **System** → **Advanced** → **Init/Shutdown Scripts**
3. Click **Add**
4. Configure:
   - **Type:** Command
   - **Command:** `/sbin/insmod /mnt/tank/system/drivers/ACS6x.ko`
   - **When:** Post Init
   - **Enabled:** ✓ Yes
   - **Timeout:** 10
5. Save

This is persistent across TrueNAS updates.

### Method B: Recreate Systemd Unit After Updates

After TrueNAS updates, re-run:

```bash
cp /mnt/tank/system/drivers/acs6x-loader.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable acs6x-loader.service
systemctl start acs6x-loader.service
```

## Step 9: Reboot Test

```bash
# Reboot TrueNAS
reboot

# After reboot, SSH back in and verify
lsmod | grep ACS6x
dmesg | grep -i acs
lsblk
midclt call disk.query
```

## Handling Module Signature Verification

If you get "module verification failed" errors:

### Option 1: Sign the Module (Production)

```bash
# On TrueNAS, check if signing keys exist
ls /var/lib/dkms/mok.*

# If they exist, sign the module
/lib/modules/$(uname -r)/build/scripts/sign-file \
  sha256 \
  /var/lib/dkms/mok.priv \
  /var/lib/dkms/mok.der \
  /mnt/tank/system/drivers/ACS6x.ko
```

### Option 2: Disable Signature Verification (Testing Only)

```bash
# Temporary (until reboot)
echo "module.sig_enforce=0" > /etc/modprobe.d/disable-sig-check.conf
update-initramfs -u

# Then reboot
```

**Warning:** Disabling signature checks reduces system security. Only use for testing.

## Troubleshooting

### Module Loads but No Disks Appear

```bash
# Check if Accusys hardware is detected
lspci -nn | grep -i accusys

# If no PCI device, hardware issue or unsupported controller

# Check SCSI host adapter
cat /sys/class/scsi_host/host*/proc_name | grep ACS

# Manually trigger SCSI scan
echo "- - -" > /sys/class/scsi_host/host0/scan
# (Replace host0 with actual ACS host number)

# Check dmesg for errors
dmesg | grep -A 10 "ACS6x"
```

### Module Won't Load: "Invalid module format"

**Cause:** Kernel version mismatch between build and runtime.

**Fix:** Rebuild module with exact matching kernel headers:
```bash
# On build VM
uname -r  # on TrueNAS: 6.12.15-debug+truenas
# Install pve-headers-6.12.15-1-pve (closest match)
# Rebuild as per BUILD-INSTRUCTIONS-DEBIAN12.md
```

### Module Loads but System Unstable

**Symptoms:** Kernel panics, I/O errors, system hangs.

**Possible causes:**
1. Incompatible hardware/firmware
2. DMA addressing issues (64-bit vs 32-bit)
3. IRQ conflicts

**Debug steps:**
```bash
# Enable driver debug output
# Before loading, rebuild with DEBUG=y

# Check for DMA errors
dmesg | grep -i dma

# Check IRQ assignments
cat /proc/interrupts | grep ACS

# Try loading with different parameters (if module supports them)
insmod /mnt/tank/system/drivers/ACS6x.ko debug=1
```

### TrueNAS Update Broke Driver

After TrueNAS updates kernel:

```bash
# Check new kernel version
uname -r

# Rebuild module on Debian VM with matching headers
# Re-deploy as per this guide

# If kernel minor version changed (6.12 → 6.13):
# Expect rebuild required
# If only patch changed (6.12.15 → 6.12.16):
# Often ABI-compatible, may work without rebuild
```

## Performance Tuning

After successful deployment, optimize for performance:

```bash
# Check current I/O scheduler
cat /sys/block/sdX/queue/scheduler

# Set to 'none' or 'mq-deadline' for SAS/SSD
echo none > /sys/block/sdX/queue/scheduler

# Increase queue depth (if your array supports high queue depth)
echo 128 > /sys/block/sdX/queue/nr_requests

# Add to Init/Shutdown script to persist:
# echo none > /sys/block/sd*/queue/scheduler
```

## Monitoring

```bash
# Watch kernel messages for driver errors
dmesg -w | grep -i acs

# Monitor SCSI errors
grep -i scsi /var/log/syslog | tail -50

# Check module is still loaded
watch -n 5 'lsmod | grep ACS6x'
```

## Removal / Uninstall

```bash
# Stop and disable service
systemctl stop acs6x-loader.service
systemctl disable acs6x-loader.service
rm /etc/systemd/system/acs6x-loader.service
systemctl daemon-reload

# OR remove Init/Shutdown script via TrueNAS UI

# Unload module
rmmod ACS6x

# Remove files
rm /mnt/tank/system/drivers/ACS6x.ko

# Reboot to ensure clean state
reboot
```

## Support and Updates

- Driver version: 3.2.2 (modernized for kernel 6.x)
- Original vendor: Accusys Technology Inc.
- Modernization: Community effort for TrueNAS SCALE compatibility

Check `/root/drivers/Linux_Drv_3.2.2/` for source and rebuild instructions.
