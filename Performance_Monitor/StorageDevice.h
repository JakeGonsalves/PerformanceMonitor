#pragma once
#include <iostream>

using namespace::std;

class StorageDevice
{
public:
	StorageDevice();
	~StorageDevice();

	string DeviceId;
	string FriendlyName;
	int HealthStatus;
	int DeviceSize;
	int MediaType;
};

