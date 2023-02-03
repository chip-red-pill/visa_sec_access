#include "rwdrv.h"

#include <cstdio>
#include <cassert>


static bool g_rwdrv_started;
static bool g_srvc_created;
static bool g_srvc_started;
static HANDLE g_rwdrv_hnd;

int start_rwdrv_driver()
{
	SC_HANDLE sc_mng = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (sc_mng == NULL)
	{
		DWORD err = GetLastError();
		wprintf(L"Error: Can't open SC Manager database: 0x%08x\n", err);
		return -1;
	}

	SC_HANDLE srvc = CreateService(sc_mng, L"RwDrv", L"RwDrv",
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		L"\\SystemRoot\\System32\\drivers\\RwDrv.sys", NULL, NULL, NULL, NULL, NULL);
	if (srvc == NULL && GetLastError() != ERROR_SERVICE_EXISTS)
	{
		DWORD err = GetLastError();
		CloseServiceHandle(sc_mng);

		wprintf(L"Error: Can't create RwDrv service: 0x%08x\n", err);
		return -1;
	}
	g_srvc_created = srvc != NULL;
	if (!g_srvc_created)
	{
		srvc = OpenService(sc_mng, L"RwDrv", SERVICE_ALL_ACCESS);
		if (srvc == NULL)
		{
			DWORD err = GetLastError();
			CloseServiceHandle(sc_mng);

			wprintf(L"Error: Can't open RwDrv service: 0x%08x\n", err);
			return -1;
		}
	}
	CloseServiceHandle(sc_mng);
	
	BOOL res = StartService(srvc, NULL, NULL);
	if (!res && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
	{
		DWORD err = GetLastError();
		CloseServiceHandle(srvc);

		wprintf(L"Error: Can't start RwDrv: 0x%08x\n", err);
		return -1;
	}
	g_srvc_started = res;
	CloseServiceHandle(srvc);
	
	g_rwdrv_started = true;
	return 0;
}

void stop_rwdrv_driver()
{
	SC_HANDLE sc_mng = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (sc_mng == NULL)
	{
		DWORD err = GetLastError();
		wprintf(L"Error: Can't open SC Manager database: 0x%08x\n", err);
		return;
	}

	SC_HANDLE srvc = OpenService(sc_mng, L"RwDrv", SERVICE_ALL_ACCESS);
	if (srvc == NULL)
	{
		DWORD err = GetLastError();
		CloseServiceHandle(sc_mng);

		wprintf(L"Error: Can't open RwDrv service: 0x%08x\n", err);
		return;
	}
	CloseServiceHandle(sc_mng);

	assert(g_srvc_started || !g_srvc_created);
	if (g_srvc_started)
	{
		SERVICE_STATUS srv_sts;
		BOOL res = ControlService(srvc, SERVICE_CONTROL_STOP, &srv_sts);
		if (!res && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
		{
			DWORD err = GetLastError();
			wprintf(L"Error: Can't stop RwDrv service: 0x%08x\n", err);
		}
	}
	if (g_srvc_created)
	{
		BOOL res = DeleteService(srvc);
		if (!res)
		{
			DWORD err = GetLastError();
			wprintf(L"Error: Can't delete RwDrv service: 0x%08x\n", err);
		}
	}

	CloseServiceHandle(srvc);
	g_rwdrv_started = false;
}

int open_rwdrv()
{
	assert(g_rwdrv_started);

	g_rwdrv_hnd = CreateFile(L"\\\\.\\RwDrv", FILE_GENERIC_READ | FILE_GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (!g_rwdrv_hnd)
	{
		DWORD err = GetLastError();
		wprintf(L"Error: Can't open RwDrv device: 0x%08x\n", err);
		return -1;
	}

	return 0;
}

void close_rwdrv()
{
	if (g_rwdrv_hnd)
	{
		CloseHandle(g_rwdrv_hnd);
		g_rwdrv_hnd = NULL;
	}
}

void deinit_rwdrv()
{
	if (g_rwdrv_hnd)
		close_rwdrv();
	if (g_rwdrv_started)
		stop_rwdrv_driver();
}

int init_rwdrv()
{
	if (!g_rwdrv_started)
	{
		if (start_rwdrv_driver())
		{
			stop_rwdrv_driver();
			return -1;
		}

		atexit(deinit_rwdrv);
	}
	
	if (!g_rwdrv_hnd)
		if (open_rwdrv())
			return -1;

	assert(g_rwdrv_started);
	assert(g_rwdrv_hnd);
	return 0;
}


struct RWDRV_PHYSMEM_REQ
{
	unsigned long long addr;
	DWORD buf_size;
	DWORD val_size;
	DWORD* val_ptr;
	DWORD val;
};

int rwdrv_read_phys_mem(unsigned long long addr, DWORD* out_val)
{
	assert(out_val);

	if (init_rwdrv())
		return -1;
	assert(g_rwdrv_hnd);

	RWDRV_PHYSMEM_REQ req_buf;
	DWORD bytes_ret;

	req_buf.addr = addr;
	req_buf.buf_size = 4;
	req_buf.val_size = 2;
	req_buf.val_ptr = &req_buf.val;

	BOOL res = DeviceIoControl(g_rwdrv_hnd, 0x222808, &req_buf, sizeof req_buf, NULL, 0, &bytes_ret, NULL);
	if (!res)
	{
		DWORD err = GetLastError();
		wprintf(L"Error: Can't control RwDrv device (read phys mem): ioctl code: 0x%08x: addr: 0x%08llx: error: 0x%08x\n",
			0x222808, addr, err);
		return -1;
	}
	*out_val = req_buf.val;
	return 0;
}

int rwdrv_write_phys_mem(unsigned long long addr, DWORD val)
{
	if (init_rwdrv())
		return -1;
	assert(g_rwdrv_hnd);

	RWDRV_PHYSMEM_REQ req_buf;
	DWORD bytes_ret;

	req_buf.addr = addr;
	req_buf.buf_size = 4;
	req_buf.val_size = 2;
	req_buf.val_ptr = &req_buf.val;
	req_buf.val = val;

	BOOL res = DeviceIoControl(g_rwdrv_hnd, 0x22280c, &req_buf, sizeof req_buf, NULL, 0, &bytes_ret, NULL);
	if (!res)
	{
		DWORD err = GetLastError();
		wprintf(L"Error: Can't control RwDrv device (write phys mem): ioctl code: 0x%08x: addr: 0x%08llx: error: 0x%08x\n",
			0x22280c, addr, err);
		return -1;
	}
	return 0;
}
