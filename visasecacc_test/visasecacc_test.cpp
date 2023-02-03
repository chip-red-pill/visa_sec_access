#include "rwdrv.h"

#include <cstdio>
#include <intrin.h>


int read_pci(unsigned bus, unsigned dev, unsigned func, unsigned offset, DWORD* out_val)
{
	const unsigned ecam_base = 0xe0000000;
	return rwdrv_read_phys_mem(ecam_base | (bus << 20) | ((dev & 0x1f) << 15) | ((func & 7) << 12) | (offset & 0xfff), out_val);
}

int write_pci(unsigned bus, unsigned dev, unsigned func, unsigned offset, DWORD val)
{
	const unsigned ecam_base = 0xe0000000;
	return rwdrv_write_phys_mem(ecam_base | (bus << 20) | ((dev & 0x1f) << 15) | ((func & 7) << 12) | (offset & 0xfff), val);
}

void get_npk_bdf(unsigned pch_did, unsigned& bus, unsigned& dev, unsigned& func)
{
	switch (pch_did >> 16)
	{
	// old SoCs
	case 0x5af0: // DNV
	case 0x1990: // GLK
	case 0x31f0: // BXTP
		bus = 0;
		dev = 0;
		func = 2;
		break;
	// Mainline + new SoCs
	default:
		bus = 0;
		dev = 31;
		func = 7;
		break;
	}
}

int main()
{
	wprintf(L"-----------------------------------------------------------------\n");

	DWORD pch_did;
	if (read_pci(0, 0, 0, 0, &pch_did))
	{
		wprintf(L"Error: Can't read PCH DID\n");
		return -1;
	}
	int cpu_info[4];
	__cpuid(cpu_info, 1);
	wprintf(L"CPUID: 0x%08x: PCH DID: 0x%04x\n", cpu_info[0], pch_did >> 16);

	unsigned npk_bus, npk_dev, npk_func;
	get_npk_bdf(pch_did, npk_bus, npk_dev, npk_func);
	wprintf(L"NPK BDF: %x:%x:%x\n", npk_bus, npk_dev, npk_func);

	DWORD npk_csr_bar_lower;
	if (read_pci(npk_bus, npk_dev, npk_func, 0x10, &npk_csr_bar_lower))
	{
		wprintf(L"Error: Can't read NPK CSR BAR register\n");
		return -1;
	}
	if (npk_csr_bar_lower == 0xffffffff)
	{
		wprintf(L"Error: NPK device is disabled in UEFI/BIOS\n");
		return -1;
	}
	DWORD npk_csr_bar_upper;
	if (read_pci(npk_bus, npk_dev, npk_func, 0x14, &npk_csr_bar_upper))
	{
		wprintf(L"Error: Can't read NPK CSR BAR UPPER register\n");
		return -1;
	}
	if (npk_csr_bar_upper == 0xffffffff)
	{
		wprintf(L"Error: Can't read NPK CSR BAR UPPER register: value read: 0xffffffff \n");
		return - 1;
	}

	// Check NPK memory space enabled
	DWORD npk_cmd;
	if (read_pci(npk_bus, npk_dev, npk_func, 0x4, &npk_cmd))
	{
		wprintf(L"Error: Can't read NPK CMD register\n");
		return -1;
	}
	if ((npk_cmd & 2) == 0)
	{
		npk_cmd |= 2;
		if (write_pci(npk_bus, npk_dev, npk_func, 0x4, npk_cmd))
		{
			wprintf(L"Error: Can't write NPK CMD register: value: 0x%08x\n", npk_cmd);
			return -1;
		}
	}

	unsigned long long npk_csr_bar  = ((unsigned long long)npk_csr_bar_upper << 32) | (npk_csr_bar_lower & 0xfff00000);
	wprintf(L"NPK CSR BAR: 0x%08llx\n", npk_csr_bar);

	DWORD SCP, SWP, SRP;
	// Secure Control Policy
	if (rwdrv_read_phys_mem(npk_csr_bar | 0xffc00, &SCP))
	{
		wprintf(L"Error: Can't read NPK SCP register");
		return -1;
	}
	// Secure Write Policy
	if (rwdrv_read_phys_mem(npk_csr_bar | 0xffc08, &SWP))
	{
		wprintf(L"Error: Can't read NPK SWP register");
		return -1;
	}
	// Secure Read Policy
	if (rwdrv_read_phys_mem(npk_csr_bar | 0xffc10, &SRP))
	{
		wprintf(L"Error: Can't read NPK SRP register");
		return -1;
	}

	wprintf(L"NPK VISA Secure Policy: SCP: 0x%08x: SWP: 0x%08x: SRP: 0x%08x\n", SCP, SWP, SRP);
	return 0;
}
