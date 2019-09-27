#include <iostream>
#include <Windows.h>
#include <WbemIdl.h>
#include <comdef.h>
#include <vector>
#include <comutil.h>

#include "StorageDevice.h"

#pragma comment(lib, "wbemuuid.lib")

using namespace::std;

// Basic COM stuff
void InitiailzeCOM()
{

	// 1. Initialize COM

	HRESULT hr;
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		cout << "Failed to initialize COM library. Error code = 0x" << hex << hr << endl;
	}

	// 2. Setting COM Security Levels

	hr = CoInitializeSecurity(
		NULL,                        // Security descriptor    
		-1,                          // COM negotiates authentication service
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication level for proxies
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation level for proxies
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities of the client or server
		NULL);                       // Reserved

	if (FAILED(hr))
	{
		cout << "Failed to initialize security. Error code = 0x" << hex << hr << endl;
		CoUninitialize();
	}
}

void SetupWBEM(IWbemLocator*& pLoc, IWbemServices*& pSvc)
{

	// 3. Obtain initial locator in WMI

	HRESULT hr;

	hr = CoCreateInstance(CLSID_WbemLocator, 0,
		CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hr))
	{
		cout << "Failed to create IWbemLocator object. Err code = 0x"
			<< hex << hr << endl;
		CoUninitialize();
	}

	// 4. Connect to WMI through the IWebmLocator::ConnectServer method

	// Connect to the root\default namespace with the current user.
	hr = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\microsoft\\windows\\storage"),  //namespace
		NULL,       // User name 
		NULL,       // User password
		0,         // Locale 
		NULL,     // Security flags
		0,         // Authority 
		0,        // Context object 
		&pSvc);   // IWbemServices proxy


	if (FAILED(hr))
	{
		cout << "Could not connect. Error code = 0x"
			<< hex << hr << endl;
		pLoc->Release();
		CoUninitialize();
	}

	// 5. Set the security levels on the proxy

	// Set the proxy so that impersonation of the client occurs.
	hr = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if (FAILED(hr))
	{
		cout << "Could not set proxy blanket. Error code = 0x"
			<< hex << hr << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}

	cout << "Connected to WMI" << endl;
}

int main()
{
	IWbemLocator *wbemLocator = NULL;
	IWbemServices *wbemServices = NULL;
	
	// Basic COM stuff
	InitiailzeCOM();

	// Setup WMI Services
	SetupWBEM(wbemLocator, wbemServices);

	// Determine Storage Information
	IEnumWbemClassObject* storageEnum = NULL;
	HRESULT hr = wbemServices->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM MSFT_PhysicalDisk"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&storageEnum);

	// Error checking
	if (FAILED(hr))
	{
		cout << "Query for MSFT_PhysicalDisk. Error code = 0x"
			<< hex << hr << endl;

		// Release memory
		wbemServices->Release();
		wbemLocator->Release();

		// Uninitialize COM
		CoUninitialize();
	}

	IWbemClassObject *storageWbemObj = NULL;
	ULONG uReturn = 0;

	// Used to store all drives
	vector<StorageDevice> storageDevices;

	while (storageEnum) {
		HRESULT hr2 = storageEnum->Next(WBEM_INFINITE, 1, &storageWbemObj, &uReturn);
		if (uReturn == 0)
		{
			break;
		}

		StorageDevice storageDevice;

		// Variant can be multiple different types
		VARIANT deviceId;
		VARIANT friendlyName;
		VARIANT healthStatus;
		VARIANT deviceSize;
		VARIANT mediaType;

		// Get the value from the storage obj
		storageWbemObj->Get(L"DeviceId", 0, &deviceId, 0, 0);
		storageWbemObj->Get(L"FriendlyName", 0, &friendlyName, 0, 0);
		storageWbemObj->Get(L"HealthStatus", 0, &healthStatus, 0, 0);
		storageWbemObj->Get(L"Size", 0, &deviceSize, 0, 0);
		storageWbemObj->Get(L"MediaType", 0, &mediaType, 0, 0);

		// Store in class to convert from variant to typed vars
		storageDevice.DeviceId = deviceId.bstrVal == NULL ? "" : _bstr_t(deviceId.bstrVal);
		storageDevice.FriendlyName = friendlyName.bstrVal == NULL ? "" : _bstr_t(friendlyName.bstrVal);
		storageDevice.HealthStatus = healthStatus.uintVal;
		storageDevice.DeviceSize = deviceSize.uintVal;
		storageDevice.MediaType = mediaType.uintVal;

		// Push to vector of storage objs
		storageDevices.push_back(storageDevice);

		// Release the storage object memory
		storageWbemObj->Release();
	}
}