def phys_mem_read32(addr):
    return ipc.threads[0].mem("0x%08xp" % addr, 4).ToUInt32()

def phys_mem_read64(addr):
    return ipc.threads[0].mem("0x%08xp" % addr, 8).ToUInt64()

def pci_read(bus, dev, func, offset):
    config_addr = ((bus & 0xff) << 16) | ((dev & 0x1f) << 11) | ((func & 7) << 8) | (offset & 0xff)
    ipc.threads[0].dport(0xcf8, 0x80000000 | config_addr)
    return ipc.threads[0].dport(0xcfc).ToUInt32()

def pci_write(bus, dev, func, offset, val):
    config_addr = ((bus & 0xff) << 16) | ((dev & 0x1f) << 11) | ((func & 7) << 8) | (offset & 0xff)
    ipc.threads[0].dport(0xcf8, 0x80000000 | config_addr)
    ipc.threads[0].dport(0xcfc, val)

def get_npk_bdf(host_did):
    atom_dids = (0x5af0, 0x1990, 0x31f0) # BXTP, DNV, GLK
    if (host_did >> 16) in atom_dids:
        return (0, 0, 2)
    return (0, 0x1f, 7)

def print_visa_sec_access_policy():
    host_did = pci_read(0, 0, 0, 0)
    pch_did = pci_read(0, 0x1f, 0, 0)
    rev_id = pci_read(0, 0x1f, 0, 8)
    manuf_id = pci_read(0, 0, 0, 0xf8)
    platform_id = ipc.threads[0].msr(0x17).ToUInt64()
    cpuid = ipc.threads[0].cpuid(1, 0)['eax'].ToUInt32()
    
    print("-----------------------------------------------------------------");
    print("CPUID: 0x%05x: HOST DID: 0x%04x: PCH DID: 0x%04x:\n" \
          "REVISION ID: 0x%02x: MANUFACTURER ID: 0x%08x: PLATFORM ID: 0x%x" % \
        (cpuid, host_did >> 16, pch_did >> 16, rev_id & 0xff, manuf_id, (platform_id >> 50) & 7))
    
    npk_bus, npk_dev, npk_func = get_npk_bdf(host_did)
    print("NPK BDF: %x:%x:%x" % (npk_bus, npk_dev, npk_func))
    
    npk_csr_bar_lower = pci_read(npk_bus, npk_dev, npk_func, 0x10)
    if npk_csr_bar_lower == 0xffffffff:
        print("Error: NPK device is disabled in UEFI/BIOS")
        return
    npk_csr_bar_upper = pci_read(npk_bus, npk_dev, npk_func, 0x14)
    if npk_csr_bar_upper == 0xffffffff:
        printf("Error: Can't read NPK CSR BAR UPPER register: value read: 0xffffffff");
        return
    
    npk_csr_bar = (npk_csr_bar_upper << 32) | (npk_csr_bar_lower & 0xfff00000)
    if npk_csr_bar == 0:
        npk_csr_bar = 0xfef00000
        pci_write(npk_bus, npk_dev, npk_func, 0x10, npk_csr_bar)
    print("NPK CSR BAR: 0x%08x" % npk_csr_bar)
    
    npk_cmd = pci_read(npk_bus, npk_dev, npk_func, 0x4)
    if  (npk_cmd & 2) == 0:
        npk_cmd |= 2
        pci_write(npk_bus, npk_dev, npk_func, 0x4, npk_cmd)
    
    SCP = phys_mem_read64(npk_csr_bar | 0xffc00)
    SWP = phys_mem_read64(npk_csr_bar | 0xffc08)
    SRP = phys_mem_read64(npk_csr_bar | 0xffc10)
    print("NPK VISA Secure Policy: SCP: 0x%016x: SWP: 0x%016x: SRP: 0x%016x\n" % (SCP, SWP, SRP))
