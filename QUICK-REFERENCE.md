# ACS6x Driver - Quick Reference Card

**One-page reference for building and deploying Accusys ACS6x driver on TrueNAS SCALE**

---

## 🔧 Quick Build (Debian 12 VM)

```bash
# 1. Add Proxmox repo + headers
wget https://enterprise.proxmox.com/debian/proxmox-release-bookworm.gpg \
  -O /etc/apt/trusted.gpg.d/proxmox-release-bookworm.gpg
echo "deb http://download.proxmox.com/debian/pve bookworm pve-no-subscription" \
  | tee /etc/apt/sources.list.d/pve-no-subscription.list
apt update && apt install -y pve-headers-6.12.15-1-pve build-essential

# 2. Build module
cd /root/drivers/Linux_Drv_3.2.2
cp Makefile.new Makefile
make KERNELDIR=/lib/modules/6.12.15-1-pve/build

# 3. Verify
ls -lh ACS6x.ko
modinfo ./ACS6x.ko
```

**Output:** `ACS6x.ko` (~1.2 MB)

---

## 🚀 Quick Deploy (TrueNAS SCALE)

```bash
# 1. Create storage location
mkdir -p /mnt/tank/system/drivers

# 2. Transfer module (from build VM)
scp ACS6x.ko root@<truenas-ip>:/mnt/tank/system/drivers/

# 3. Test load (on TrueNAS)
insmod /mnt/tank/system/drivers/ACS6x.ko
dmesg | grep -i acs
lsmod | grep ACS6x
lsscsi

# 4. Setup auto-load
cat > /etc/systemd/system/acs6x-loader.service << 'EOF'
[Unit]
Description=Accusys ACS6x Driver Loader
After=local-fs.target

[Service]
Type=oneshot
ExecStart=/sbin/insmod /mnt/tank/system/drivers/ACS6x.ko
ExecStop=/sbin/rmmod ACS6x
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable acs6x-loader.service
systemctl start acs6x-loader.service

# 5. Verify
lsblk
midclt call disk.query
# Check TrueNAS GUI → Storage → Disks
```

---

## ✅ Quick Test Checklist

```bash
# Hardware detection
lspci -nn | grep -i accusys  # Should show: 14d6:6201 or similar

# Module loaded
lsmod | grep ACS6x            # Should show: ACS6x, size, dependencies

# Kernel log
dmesg | grep "ACS MSG"        # Should show: "ACS_Driver_Initialization"

# SCSI host
cat /sys/class/scsi_host/host*/proc_name | grep ACS6x  # Should show: ACS6x

# Disks detected
lsscsi                        # Should list all ExaSAN disks
lsblk                         # Should show /dev/sdX devices

# TrueNAS recognition
midclt call disk.query | jq '.[] | {name, serial}'

# Service enabled
systemctl status acs6x-loader.service  # Should show: active (exited)
```

**All checks pass?** ✅ Driver is working!

---

## 🐛 Quick Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `lspci` shows no Accusys device | Hardware not detected | Check PCIe card, reseat, try different slot |
| `insmod` fails: "Invalid module format" | Kernel mismatch | Rebuild with exact matching headers |
| Module loads, no disks | Cable/chassis issue | Check SAS cables, chassis power, `dmesg` errors |
| "module verification failed" | Unsigned module | Sign module or disable sig check (see docs) |
| Disks disappear after reboot | Service not enabled | Check `systemctl status acs6x-loader.service` |

### Get detailed debug info:
```bash
dmesg | tail -50                          # Recent kernel messages
modinfo /mnt/tank/system/drivers/ACS6x.ko # Module details
cat /proc/interrupts | grep -i acs        # IRQ assignment
```

---

## 📚 Full Documentation

- **BUILD-INSTRUCTIONS-DEBIAN12.md** - Complete build guide
- **TRUENAS-DEPLOYMENT.md** - Full deployment guide
- **TESTING-CHECKLIST.md** - Comprehensive test suite
- **README-MODERNIZATION.md** - Technical details
- **DELIVERABLES.md** - Complete project summary

---

## 📁 Key Files

```
/root/drivers/Linux_Drv_3.2.2/
├── compat.h                    ← Kernel compatibility layer (NEW)
├── acs_ame.c                   ← Main driver (minimally modified)
├── acs_ame.h                   ← Driver header (cleaned up)
├── Makefile.new                ← Modern Makefile (NEW)
├── dkms.conf                   ← DKMS config (NEW)
├── quick-build.sh              ← Build script (NEW)
├── acs6x-loader.service        ← Systemd unit (NEW)
└── ACS6x.ko                    ← Built module (after make)
```

---

## 🎯 Supported Kernels

- ✅ **6.12.x** (TrueNAS SCALE 24.10+)
- ✅ **6.8.x** (Debian backports)
- ✅ **6.6.x** (Debian Trixie)
- ✅ **6.1.x** (Debian Bookworm LTS)
- ✅ **5.15-5.19** (Ubuntu 22.04, older SCALE)

---

## 🔑 Key API Modernizations

| Old (removed in 5.0+) | New (6.x) | Handled by |
|-----------------------|-----------|------------|
| `pci_get_bus_and_slot()` | `pci_get_domain_bus_and_slot()` | compat.h |
| `cmd->scsi_done = done` | `scsi_done(cmd)` | compat.h |
| `QUEUE_FULL` | `SAM_STAT_TASK_SET_FULL` | compat.h |
| `class_create(owner, name)` | `class_create(name)` | compat.h |

**All changes isolated in `compat.h` - driver logic untouched!**

---

## 📞 Need Help?

1. Check `TESTING-CHECKLIST.md` troubleshooting section
2. Review `dmesg` output for errors
3. Verify hardware with `lspci -vvv`
4. Post in TrueNAS forums with:
   - Kernel version (`uname -r`)
   - Hardware model (ACS-62xxx variant)
   - `dmesg` output
   - `lspci -nn` output

---

**Version:** 3.2.2-modern | **Last Updated:** 2025-10-22 | **License:** Dual BSD/GPL
