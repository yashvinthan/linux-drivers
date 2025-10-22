# Build Instructions for Debian 12 VM (Targeting TrueNAS SCALE)

This guide walks you through building the Accusys ACS6x driver on a Debian 12 VM
with Proxmox kernel headers that match the TrueNAS SCALE kernel version.

## Prerequisites

- Debian 12 (Bookworm) VM
- Internet connection
- Root or sudo access
- At least 2GB free disk space

## Step 1: Update Base System

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y build-essential dkms git wget curl
```

## Step 2: Add Proxmox Repository

TrueNAS SCALE uses Proxmox-style kernels. We'll add the Proxmox repo to get matching headers.

```bash
# Add Proxmox GPG key
wget https://enterprise.proxmox.com/debian/proxmox-release-bookworm.gpg \
  -O /etc/apt/trusted.gpg.d/proxmox-release-bookworm.gpg

# Add Proxmox repository (no-subscription, free version)
echo "deb http://download.proxmox.com/debian/pve bookworm pve-no-subscription" | \
  sudo tee /etc/apt/sources.list.d/pve-no-subscription.list

# Update package index
sudo apt update
```

## Step 3: Install Kernel Headers

Find the kernel version closest to your TrueNAS SCALE kernel. For example, if TrueNAS
is running 6.12.15-1-pve or 6.12.15-debug+truenas, install:

```bash
# List available Proxmox kernel headers
apt-cache search pve-headers | grep 6.

# Install headers matching your TrueNAS kernel (adjust version as needed)
# Example for 6.12.x:
sudo apt install -y pve-headers-6.12.15-1-pve

# OR for 6.1.x LTS:
# sudo apt install -y pve-headers-6.1.0-28-pve

# OR install current running kernel headers (if already on Proxmox kernel):
# sudo apt install -y pve-headers-$(uname -r)
```

**Note:** TrueNAS SCALE kernel (e.g., `6.12.15-debug+truenas`) is ABI-compatible
with Proxmox `pve-headers-6.12.x`. The module will load despite version string
differences due to ABI compatibility.

## Step 4: Verify Headers Installation

```bash
# Check that headers are installed
ls -la /lib/modules/

# You should see a directory like: 6.12.15-1-pve
# Verify build symlink exists
ls -la /lib/modules/6.12.15-1-pve/build
```

## Step 5: Prepare Driver Source

```bash
# Navigate to driver directory
cd /root/drivers/Linux_Drv_3.2.2

# Verify all required files are present
ls -la compat.h Makefile.new dkms.conf

# Use the modernized Makefile
cp Makefile.new Makefile

# Clean any previous build artifacts
make clean
```

## Step 6: Build the Driver

### Option A: Build for Specific Kernel Version

```bash
# Build against Proxmox 6.12 headers
make KERNELDIR=/lib/modules/6.12.15-1-pve/build

# OR for 6.1 LTS:
# make KERNELDIR=/lib/modules/6.1.0-28-pve/build
```

### Option B: Build with Debug Output (recommended for first build)

```bash
make DEBUG=y KERNELDIR=/lib/modules/6.12.15-1-pve/build
```

## Step 7: Verify Build Success

```bash
# Check that ACS6x.ko was created
ls -lh ACS6x.ko

# Verify module info
modinfo ./ACS6x.ko

# Expected output should show:
#   filename:       /root/drivers/Linux_Drv_3.2.2/ACS6x.ko
#   version:        3.2.2
#   description:    ACS SAS/SATA RAID HBA
#   license:        Dual BSD/GPL
#   ...
```

## Step 8: Test Load (Optional - on Debian VM)

**WARNING:** Only test if you have actual Accusys hardware, or be prepared for
a harmless probe failure.

```bash
# Try to load the module
sudo insmod ./ACS6x.ko

# Check kernel messages
dmesg | tail -30

# Unload if loaded
sudo rmmod ACS6x
```

## Step 9: Package for TrueNAS SCALE

```bash
# Create deployment directory
mkdir -p ~/truenas-acs6x-deploy

# Copy the built module
cp ACS6x.ko ~/truenas-acs6x-deploy/

# Copy deployment scripts (see TRUENAS-DEPLOYMENT.md)
cp TRUENAS-DEPLOYMENT.md ~/truenas-acs6x-deploy/
cp acs6x-loader.service ~/truenas-acs6x-deploy/

# Create a tarball for easy transfer
cd ~
tar -czf acs6x-truenas-module.tar.gz truenas-acs6x-deploy/

# Transfer to TrueNAS SCALE
# Option 1: SCP
# scp acs6x-truenas-module.tar.gz root@truenas-ip:/root/

# Option 2: Use TrueNAS web UI to upload to a dataset
```

## Troubleshooting

### Error: "No rule to make target 'modules'"

**Cause:** Kernel headers not properly installed or KERNELDIR path wrong.

**Fix:**
```bash
# Verify KERNELDIR path exists
ls /lib/modules/6.12.15-1-pve/build

# If missing, reinstall headers:
sudo apt install --reinstall pve-headers-6.12.15-1-pve
```

### Error: "fatal error: linux/module.h: No such file or directory"

**Cause:** Kernel headers incomplete or build-essential missing.

**Fix:**
```bash
sudo apt install -y build-essential linux-headers-$(uname -r)
# or
sudo apt install -y pve-headers-6.12.15-1-pve
```

### Error: "implicit declaration of function 'class_create'"

**Cause:** compat.h not being included properly.

**Fix:**
```bash
# Verify compat.h exists
cat compat.h | grep class_create

# Ensure acs_ame.c includes it
grep 'include.*compat' acs_ame.c

# Should show: #include "compat.h"
```

### Warning: "module verification failed: signature and/or required key missing"

**This is normal** on systems with Secure Boot or module signing. The module
will still load with `insmod` or by disabling signature verification:

```bash
# Temporary disable for testing
sudo modprobe --allow-unsupported-modules ACS6x
```

For TrueNAS SCALE deployment, see `TRUENAS-DEPLOYMENT.md` for signing or
signature enforcement bypass methods.

## Building for Multiple Kernel Versions

If you need to support multiple TrueNAS SCALE versions:

```bash
# Install multiple header versions
sudo apt install -y pve-headers-6.1.0-28-pve pve-headers-6.12.15-1-pve

# Build for each
make clean
make KERNELDIR=/lib/modules/6.1.0-28-pve/build
cp ACS6x.ko ACS6x-6.1.ko

make clean
make KERNELDIR=/lib/modules/6.12.15-1-pve/build
cp ACS6x.ko ACS6x-6.12.ko
```

## Next Steps

After successful build, proceed to `TRUENAS-DEPLOYMENT.md` for deployment
instructions on TrueNAS SCALE systems.
