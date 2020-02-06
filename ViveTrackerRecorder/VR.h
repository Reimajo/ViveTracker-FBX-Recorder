#pragma once

#include <string>
#include <vector>
#include <map>
#include <openvr.h>

struct VrDevice {
public:
	int id;
	vr::ETrackedDeviceClass cls;
	std::string name;
};

class VR {
private:
	vr::IVRSystem* system;
public:
	void init();
	void stop();
	std::map<int, VrDevice> listDevices();

	vr::IVRSystem* getSystem() {
		return system;
	}

	static std::string classToText(vr::ETrackedDeviceClass devClass);
};

vr::HmdQuaternion_t getRotation(vr::HmdMatrix34_t matrix);
vr::HmdVector3_t getPosition(vr::HmdMatrix34_t matrix);