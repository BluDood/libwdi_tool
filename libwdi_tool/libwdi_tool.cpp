#include <iostream>
#include <string>
#include <vector>
#include <libwdi.h>

bool RunningAsAdmin()
{
	BOOL isAdmin = FALSE;
	PSID adminGroup = nullptr;

	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(
					&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0, &adminGroup))
	{
		CheckTokenMembership(nullptr, adminGroup, &isAdmin);
		FreeSid(adminGroup);
	}

	return isAdmin;
}

bool FindDevice(const std::string &deviceName)
{
	static struct wdi_options_create_list ocl = {0};
	ocl.list_all = true;

	struct wdi_device_info *device, *list;

	if (wdi_create_list(&list, &ocl) != WDI_SUCCESS)
	{
		return false;
	}

	for (device = list; device; device = device->next)
	{
		if (device->desc == deviceName)
		{
			wdi_destroy_list(list);
			return true;
		}
	}

	wdi_destroy_list(list);
	return false;
}

bool CheckDriver(const std::string &deviceName, const std::string &driverName)
{
	static struct wdi_options_create_list ocl = {0};
	ocl.list_all = true;

	struct wdi_device_info *device, *list;

	if (wdi_create_list(&list, &ocl) != WDI_SUCCESS)
	{
		return false;
	}

	for (device = list; device; device = device->next)
	{
		if (device->desc == deviceName)
		{
			bool found = device->driver && device->driver == driverName;
			wdi_destroy_list(list);
			return found;
		}
	}

	wdi_destroy_list(list);
	return false;
}

bool InstallDriver(const std::string &deviceName, const std::string &driverName)
{
	static struct wdi_options_prepare_driver opd = {0};

	if (driverName == "libusbK")
	{
		opd.driver_type = WDI_LIBUSBK;
	}
	else
	{
		printf("Unknown driver type\n");
		return false;
	}

	static struct wdi_options_create_list ocl = {0};
	ocl.list_all = true;
	ocl.list_hubs = TRUE;
	ocl.trim_whitespaces = TRUE;

	static struct wdi_options_install_driver oid = {0};

	struct wdi_device_info *device, *list;

	if (wdi_create_list(&list, &ocl) != WDI_SUCCESS)
	{
		return false;
	}

	for (device = list; device != NULL; device = device->next)
	{
		if (device->desc == deviceName)
		{
			printf("Driver: %d\n", opd.driver_type);
			int wpd = wdi_prepare_driver(device, NULL, "usb_device.inf", &opd);
			printf("wpd: %s\n", wdi_strerror(wpd));
			if (wpd != WDI_SUCCESS)
			{
				wdi_destroy_list(list);
				return false;
			}
			int wid = wdi_install_driver(device, NULL, "usb_device.inf", &oid);
			printf("wid: %s\n", wdi_strerror(wid));
			if (wid != WDI_SUCCESS)
			{
				wdi_destroy_list(list);
				return false;
			}
			wdi_destroy_list(list);
			return true;
		}
	}
}

void usage()
{
	printf("Usage: libwdi_tool [option]\n");
	printf("Options:\n");
	printf("  --find_device <device> Find device by name\n");
	printf("  --check_driver <device> <driver> Check driver for device\n");
	printf("  --install_driver <device> <driver> Install driver for device\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		usage();
		return 1;
	}

	std::string command = argv[1];

	if (command == "--find_device")
	{
		if (argc < 3)
		{
			usage();
			return 1;
		}

		std::string deviceName = argv[2];
		if (FindDevice(deviceName))
		{
			printf("Device found\n");
			return 0;
		}
		else
		{
			printf("Device not found\n");
			return 1;
		}
	}
	else if (command == "--check_driver")
	{
		if (argc < 4)
		{
			usage();
			return 1;
		}

		std::string deviceName = argv[2];
		std::string driverName = argv[3];
		if (CheckDriver(deviceName, driverName))
		{
			printf("Driver found\n");
			return 0;
		}
		else
		{
			printf("Driver not found\n");
			return 1;
		}
	}
	else if (command == "--install_driver")
	{
		if (argc < 4)
		{
			usage();
			return 1;
		}

		bool isAdmin = RunningAsAdmin();
		if (!isAdmin)
		{
			printf("Needs to be run as administrator\n");
			return 1;
		}

		std::string deviceName = argv[2];
		std::string driverName = argv[3];
		if (InstallDriver(deviceName, driverName))
		{
			printf("Driver installed\n");
			return 0;
		}
		else
		{
			printf("Driver not installed\n");
			return 1;
		}
	}
	else
	{
		usage();
		return 1;
	}

	return 0;
}
