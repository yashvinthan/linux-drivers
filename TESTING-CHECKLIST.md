# ACS6x Driver Testing and Troubleshooting Checklist

This checklist helps verify the Accusys ACS6x driver is working correctly on
TrueNAS SCALE with your ExaSAN chassis.

---

## Pre-Installation Checklist

- [ ] Hardware physically connected
  - [ ] Accusys ACS-62xxx PCIe card seated properly
  - [ ] ExaSAN A08S-PS chassis powered on
  - [ ] SAS/SATA cables connected between card and chassis
  - [ ] All drives inserted in chassis

- [ ] TrueNAS SCALE prerequisites
  - [ ] SSH enabled on TrueNAS
  - [ ] ZFS pool available for driver storage
  - [ ] Root access confirmed
  - [ ] Kernel version noted (`uname -r`)

- [ ] Build environment ready
  - [ ] Debian 12 VM or compatible build system
  - [ ] Matching pve-headers installed
  - [ ] Build completed successfully (ACS6x.ko exists)

---

## Installation Verification

### 1. PCI Device Detection

```bash
lspci -nn | grep -i accusys
```

**Expected output:**
```
01:00.0 RAID bus controller [0104]: Accusys, Inc. ACS-62xxx [14d6:6201]
```

**If not visible:**
- [ ] Check physical PCIe card installation
- [ ] Verify card LED indicators (if any)
- [ ] Try different PCIe slot
- [ ] Check BIOS/UEFI for PCIe slot enablement
- [ ] Test card in different system

### 2. Module Load Test

```bash
insmod /mnt/tank/system/drivers/ACS6x.ko
echo $?
```

**Expected:** Exit code `0` (success)

**If fails (exit code ≠ 0):**
```bash
dmesg | tail -30
```

**Common errors and solutions:**

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Invalid module format` | Kernel version mismatch | Rebuild with matching headers |
| `Unknown symbol` | Missing kernel config | Check kernel config, may need different kernel |
| `module verification failed` | Unsigned module | Sign module or disable sig check |
| `No such device` | Hardware not detected | Check PCI device with lspci |

### 3. Module Loaded Confirmation

```bash
lsmod | grep ACS6x
```

**Expected output:**
```
ACS6x                1167360  0
```

- [ ] Module appears in lsmod output
- [ ] No errors in `dmesg | grep -i acs`

### 4. Kernel Log Verification

```bash
dmesg | grep -i acs | tail -50
```

**Expected messages:**
```
[  X.XXXXXX] ACS_ame: loading out-of-tree module taints kernel.
[  X.XXXXXX] ACS MSG:  ACS_Driver_Initialization
[  X.XXXXXX] ACS MSG:  Detect device vendor ID: 14d6, Device ID: 6201
[  X.XXXXXX] ACS MSG:  Start ACS_Timer_Func for ACS index 1.
[  X.XXXXXX] scsi hostX: ACS6x
```

**Look for these indicators:**
- [ ] "ACS_Driver_Initialization" appears
- [ ] Correct Vendor/Device ID detected (14d6:6201 or similar)
- [ ] SCSI host registered
- [ ] Timer function started
- [ ] No error messages or warnings

**Red flags (investigate if present):**
- ❌ "probe fail" or "init fail"
- ❌ "DMA mask" errors
- ❌ "Unable to request IRQ"
- ❌ Kernel panic or oops messages

---

## Runtime Verification

### 5. SCSI Host Adapter Check

```bash
ls -l /sys/class/scsi_host/
cat /sys/class/scsi_host/host*/proc_name
```

**Expected:**
- [ ] At least one hostN directory exists
- [ ] One proc_name file contains "ACS6x"

```bash
# Get ACS6x host number
ACS_HOST=$(grep -l "ACS6x" /sys/class/scsi_host/host*/proc_name | grep -o 'host[0-9]*')
echo "ACS6x SCSI host: $ACS_HOST"
```

**Note the host number** (e.g., host2) for later use.

### 6. SCSI Device Discovery

```bash
lsscsi
```

**Expected output (example):**
```
[2:0:0:0]    disk    ATA      ST4000VN008-2DR1 SC60  /dev/sda
[2:0:1:0]    disk    ATA      ST4000VN008-2DR1 SC60  /dev/sdb
[2:0:2:0]    disk    ATA      WDC WD40EFRX-68N 0A82  /dev/sdc
...
```

**Verify:**
- [ ] Disks appear under correct host adapter number
- [ ] Disk count matches physical drives in chassis
- [ ] Device names assigned (/dev/sdX)
- [ ] Disk models recognized

**If no disks appear:**
```bash
# Manual SCSI bus scan (replace hostX with your ACS6x host)
echo "- - -" > /sys/class/scsi_host/hostX/scan

# Wait 5 seconds
sleep 5

# Check again
lsscsi
dmesg | tail -20
```

### 7. Block Device Verification

```bash
lsblk
```

**Expected:**
- [ ] All ExaSAN disks listed
- [ ] Correct sizes shown
- [ ] No I/O errors in dmesg

```bash
# Get disk serial numbers
for disk in /dev/sd?; do
  echo -n "$disk: "
  hdparm -I $disk 2>/dev/null | grep "Serial Number" || echo "(no serial)"
done
```

### 8. TrueNAS Disk Recognition

```bash
# Via CLI
midclt call disk.query | jq '.[] | {name, serial, size}'

# Or check via Web UI
# Navigate to: Storage → Disks
```

**Verify:**
- [ ] All ExaSAN disks appear in TrueNAS disk list
- [ ] Serial numbers match physical drives
- [ ] Sizes correct
- [ ] Status shows as "ONLINE" or available

### 9. Basic I/O Test

**WARNING:** Only run on unused/test disks!

```bash
# Pick a disk connected through ACS6x (e.g., /dev/sda)
TEST_DISK=/dev/sda

# Read test (safe, non-destructive)
dd if=$TEST_DISK of=/dev/null bs=1M count=100 status=progress

# Check for errors
dmesg | tail -20 | grep -i error
```

**Expected:**
- [ ] Read completes without errors
- [ ] Reasonable throughput (>50 MB/s for HDD, >200 MB/s for SSD)
- [ ] No kernel errors in dmesg

**If write test needed (DESTRUCTIVE - erases disk):**
```bash
# DANGER: This will destroy data on TEST_DISK!
# dd if=/dev/zero of=$TEST_DISK bs=1M count=100 oflag=direct status=progress
```

### 10. Performance Baseline

```bash
# Install fio if not present
apt install -y fio

# Sequential read test (safe)
fio --name=seq-read --rw=read --bs=1M --size=1G --numjobs=1 \
    --filename=/dev/sda --direct=1 --ioengine=libaio --iodepth=32 \
    --group_reporting
```

**Record baseline numbers:**
- [ ] Sequential read throughput: __________ MB/s
- [ ] IOPS: __________
- [ ] Latency: __________ ms

---

## Persistent Boot Testing

### 11. Service Configuration

```bash
systemctl status acs6x-loader.service
```

**Expected:**
```
● acs6x-loader.service - Accusys ACS6x RAID/HBA Driver Loader
     Loaded: loaded (/etc/systemd/system/acs6x-loader.service; enabled; preset: enabled)
     Active: active (exited) since ...
```

**Verify:**
- [ ] Service is `loaded`
- [ ] Service is `enabled`
- [ ] Service is `active (exited)`
- [ ] No error messages

### 12. Reboot Persistence Test

```bash
# Before reboot
lsmod | grep ACS6x > /tmp/pre-reboot-acs.txt
lsblk > /tmp/pre-reboot-disks.txt

# Reboot
reboot

# After reboot (wait for system to come back up)
lsmod | grep ACS6x > /tmp/post-reboot-acs.txt
lsblk > /tmp/post-reboot-disks.txt

# Compare
diff /tmp/pre-reboot-acs.txt /tmp/post-reboot-acs.txt
diff /tmp/pre-reboot-disks.txt /tmp/post-reboot-disks.txt
```

**Verify after reboot:**
- [ ] Module auto-loaded
- [ ] All disks reappeared
- [ ] Same /dev/sdX assignments (or expected changes)
- [ ] TrueNAS GUI shows all disks

---

## Advanced Troubleshooting

### Enable Dynamic Debug

For detailed driver debugging:

```bash
# Rebuild driver with debug enabled
# On build VM:
make clean
make DEBUG=y KERNELDIR=/lib/modules/6.12.15-1-pve/build

# Deploy to TrueNAS and load
rmmod ACS6x
insmod /mnt/tank/system/drivers/ACS6x.ko

# Watch debug output
dmesg -w
```

### Check IRQ Assignment

```bash
cat /proc/interrupts | grep -i acs
```

**Expected:** ACS6x driver should have IRQ assigned and interrupt count increasing.

**If IRQ count not increasing:**
- [ ] Hardware interrupt issue
- [ ] IRQ conflict
- [ ] Cable/connection problem

### DMA Addressing Check

```bash
dmesg | grep -i "dma.*64\|dma.*32"
```

**Expected:**
- Driver should prefer 64-bit DMA if supported
- Fallback to 32-bit if needed

### SCSI Error Logging

```bash
# Real-time SCSI error monitoring
tail -f /var/log/syslog | grep -i scsi

# Or
journalctl -f -u acs6x-loader.service
```

### Test Disk Hot-Plug (if supported)

```bash
# Before removing disk
ACS_HOST=host2  # Replace with your ACS6x host number
echo "Before removal:"
lsscsi -g | grep "$ACS_HOST"

# Physically remove a disk from chassis

# Trigger rescan
echo "- - -" > /sys/class/scsi_host/$ACS_HOST/scan
sleep 2

# Check
echo "After removal:"
lsscsi -g | grep "$ACS_HOST"
dmesg | tail -10

# Re-insert disk and rescan
echo "- - -" > /sys/class/scsi_host/$ACS_HOST/scan
sleep 2
lsscsi -g | grep "$ACS_HOST"
```

---

## Known Issues and Limitations

### Issue: "Module verification failed: signature and/or required key missing"

**Impact:** Module loads with warning, but works.

**Solution:**
- Sign the module (production)
- OR disable signature enforcement (testing only)
- See TRUENAS-DEPLOYMENT.md section on module signing

### Issue: Device names change after reboot (/dev/sda becomes /dev/sdb)

**Impact:** Confusion, potential data access issues if using device names directly.

**Solution:**
- Always use disk IDs or WWNs in TrueNAS pool configuration
- Check: `ls -l /dev/disk/by-id/`
- TrueNAS handles this automatically in pool config

### Issue: Driver loads but disks not detected

**Possible causes:**
1. SAS cable not fully seated
2. Chassis not fully powered on before TrueNAS boot
3. Firmware incompatibility
4. Wrong chassis model (A08S-PS required)

**Debug steps:**
- Check `dmesg` for probe errors
- Verify lspci shows Accusys card
- Try manual SCSI scan
- Check chassis power and LED indicators

### Issue: System hangs during disk I/O

**Possible causes:**
- DMA configuration mismatch
- IRQ conflicts
- Bad SAS cable or HBA

**Debug steps:**
- Check `dmesg` for DMA errors
- Try different PCIe slot
- Test with fewer disks
- Update chassis firmware (if available)

---

## Success Criteria

The driver is fully functional when ALL of the following are true:

- [x] lspci shows Accusys PCIe device
- [x] Module loads without errors
- [x] dmesg shows successful probe and initialization
- [x] SCSI host adapter registered
- [x] All expected disks appear in lsscsi
- [x] All disks appear in TrueNAS GUI → Storage → Disks
- [x] Basic I/O test completes without errors
- [x] Reasonable performance (within expected range for drive type)
- [x] Module auto-loads after reboot
- [x] Disks remain accessible after reboot
- [x] No kernel errors or warnings in logs

---

## Getting Help

If tests fail, collect this information:

```bash
# System info
uname -a > /tmp/acs6x-debug.txt
lspci -vvv >> /tmp/acs6x-debug.txt

# Module info
modinfo /mnt/tank/system/drivers/ACS6x.ko >> /tmp/acs6x-debug.txt

# Kernel log
dmesg >> /tmp/acs6x-debug.txt

# SCSI info
lsscsi -v >> /tmp/acs6x-debug.txt
cat /proc/scsi/scsi >> /tmp/acs6x-debug.txt

# Download and review
scp root@truenas-ip:/tmp/acs6x-debug.txt .
```

Review logs for errors and consult TRUENAS-DEPLOYMENT.md troubleshooting section.
