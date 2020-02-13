#include "VR.h"

#include <string>
#include <openvr.h>
#include <vector>

std::string getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL) {
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

vr::HmdQuaternion_t getRotation(vr::HmdMatrix34_t matrix) {
	vr::HmdQuaternion_t q;
	//change for V1.1: casting all float values to double
	q.w = sqrt(fmax(0, 1 + (double) matrix.m[0][0] + (double) matrix.m[1][1] + (double) matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + (double)matrix.m[0][0] - (double) matrix.m[1][1] - (double) matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - (double)matrix.m[0][0] + (double) matrix.m[1][1] - (double) matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - (double)matrix.m[0][0] - (double) matrix.m[1][1] + (double) matrix.m[2][2])) / 2;
	q.x = copysign(q.x, (double) matrix.m[2][1] - (double) matrix.m[1][2]);
	q.y = copysign(q.y, (double) matrix.m[0][2] - (double) matrix.m[2][0]);
	q.z = copysign(q.z, (double) matrix.m[1][0] - (double) matrix.m[0][1]);
	return q;
}

vr::HmdVector3_t getPosition(vr::HmdMatrix34_t matrix) {
	vr::HmdVector3_t vector;

	vector.v[0] = matrix.m[0][3];
	vector.v[1] = matrix.m[1][3];
	vector.v[2] = matrix.m[2][3];

	return vector;
}

void VR::init() {
	vr::EVRInitError initError = vr::VRInitError_None;
	this->system = vr::VR_Init(&initError, vr::VRApplication_Scene, nullptr);
	if (initError != vr::VRInitError_None) {
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(initError));
		throw std::exception(buf);
	}
}

void VR::stop() {
	this->system = nullptr;
	vr::VR_Shutdown();
}
/*
	Listing all connected devices of those device classes:
		TrackedDeviceClass_HMD - The device at this index is an HMD
		TrackedDeviceClass_Controller - The device is a controller
		TrackedDeviceClass_GenericTracker - The device is a tracker
	Ignoring devices from those device classes:
		TrackedDeviceClass_Invalid - There is no device at this index
		TrackedDeviceClass_TrackingReference - The device is a camera, Lighthouse base station, or other device that supplies tracking ground truth.
		TrackedDeviceClass_DisplayRedirect - Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices
*/
std::map<int, VrDevice> VR::listDevices() {
	std::map<int, VrDevice> result;
	for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
		//Changed this to the recommended version
		vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(i);
		if ((trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD) || 
			(trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) ||
			(trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker)) {
				//then this is a device that we can and want to track
				VrDevice item;
				item.id = i;
				item.cls = trackedDeviceClass;
				item.name = getTrackedDeviceString(i, vr::Prop_ModelNumber_String);
				result.emplace(i, item);
		}
		
	}
	return result;
}

std::string VR::classToText(vr::ETrackedDeviceClass devClass) {
	switch (devClass) {
	case vr::TrackedDeviceClass_Invalid: return "Not valid";
	case vr::TrackedDeviceClass_HMD: return "HMD";
	case vr::TrackedDeviceClass_Controller: return "Tracked controller";
	case vr::TrackedDeviceClass_GenericTracker: return "Generic tracker";
	case vr::TrackedDeviceClass_TrackingReference: return "Tracking reference";
	case vr::TrackedDeviceClass_DisplayRedirect: return "Display redirect";
	default: return "";
	}
}