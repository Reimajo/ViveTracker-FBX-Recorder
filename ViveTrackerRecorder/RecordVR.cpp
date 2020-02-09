#include "RecordVR.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <conio.h>

#include "FbxExport.h"
#include "VR.h"
#include "StopWatch.h"
#include "Console.h"

//for sleeping
#include <chrono>
#include <thread>

/*
Listing all devices in the console
*/
void listDevices(VR& vr) {
	auto devices = vr.listDevices();
	std::cout << "VR tracked devices:\n";
	for (auto dev : devices) {
		std::cout << "Device " << dev.second.id << " (" << dev.second.name << ") ";
		std::cout << (vr.getSystem()->IsTrackedDeviceConnected(dev.second.id) ? "connected" : "not connected");
		std::cout << " - " << VR::classToText(dev.second.cls) << "\n";
	}
}

struct Args {
public:
	std::string filename;
	std::vector<int> deviceList;
};

/*
Reading all arguments that have been specified in Visual Studio (Project/Properties/Debugging/CommandArguments) 
or when calling the .exe manually in CMD
*/
bool parseArgs(int argc, char* argv[], Args& args) {
	bool help = false;

	for (int i = 1; i < argc; i++) {
		//std::cout << i << "  " << argv[i] << "\n";
		auto strArg = std::string(argv[i]);
		if (strArg == "/?" || strArg == "-h" || strArg == "--help") {
			help = true;
			break;
		} else if (strArg == "-list") {
			VR vr;
			vr.init();
			listDevices(vr);
			vr.stop();
			return false;
		} else if (strArg == "-o") {
			i++;
			if (i >= argc) {
				std::cout << "Missing filename after -o";
				return false;
			}
			args.filename = argv[i];
		} else if (strArg == "-d") {
			i++;
			while (i < argc && argv[i][0] != '-') {
				try {
					args.deviceList.push_back(std::stoi(argv[i]));
					i++;
				} catch (std::invalid_argument) {
					std::cout << "Invalid device number: " << argv[i];
					return false;
				}
			}
			i--; // Undo the last move-to-next, we'll add this again at the end of the loop.
		} else {
			std::cout << "Unknown option: " << argv[i];
			return false;
		}
	}
	if (argc < 2) help = true;

	if (help) {
		auto appName = std::string(argv[0]);
		int lastBackslash = appName.rfind('\\');
		int lastSlash = appName.rfind('/');
		int x = std::max(lastBackslash, lastSlash);
		auto nameOnly = appName.substr(x + 1);
		std::cout << nameOnly << " -h\n";
		std::cout << nameOnly << " -list\n";
		std::cout << nameOnly << " -o filename -d devicelist\n\n";
		std::cout << "-list              List all tracked VR devices and their IDs.\n";
		std::cout << "-o filename        Gives the file name to write the animation data to.\n";
		std::cout << "-d devid devid...  Gives all the device ids to record.\n";
	}
	return true;
}
/*
Reading pose and rotation from a single tracked device and storing it into a frame
*/
void trackDevice(VR &vr,  int devId, int time, std::vector<KeyFrame>& frames) {
	vr::VRControllerState_t state;
	vr::TrackedDevicePose_t pose;
	// read device class
	vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(devId);
	// read all generic trackers and controllers
	if ((trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) || (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) || (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference)) {
		if (vr.getSystem()->GetControllerStateWithPose(vr::TrackingUniverseStanding, devId, &state, sizeof(state), &pose)) {
			if (pose.bPoseIsValid) {
				auto pos = getPosition(pose.mDeviceToAbsoluteTracking);
				auto rot = getRotation(pose.mDeviceToAbsoluteTracking);

				frames.push_back(KeyFrame(time, pos, rot));

				std::cout << pos.v[0] << " " << pos.v[1] << " " << pos.v[2] << "\n";
				std::cout << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << "\n";
			}
			else {
				std::cout << "Pose is invalid\n\n";
			}
		}
		else {
			std::cout << "Could not get pose\n\n";
		}
	// for the HMD, functions are different
	} else if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD) {
		vr.getSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &pose, 1);
		if (pose.bPoseIsValid) {
			auto pos = getPosition(pose.mDeviceToAbsoluteTracking);
			auto rot = getRotation(pose.mDeviceToAbsoluteTracking);

			frames.push_back(KeyFrame(time, pos, rot));

			std::cout << pos.v[0] << " " << pos.v[1] << " " << pos.v[2] << "\n";
			std::cout << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << "\n";
		} else {
				std::cout << "Pose is invalid\n\n";
		}
	}
}
/*
Main function that is running this program
*/
int main(int argc, char* argv[]) {
	Args args;
	if (!parseArgs(argc, argv, args)) {
		return 0;
	}

	VR vr;
	vr.init();
	//adding a small delay to allow steam to wake up
	static bool justStarted = true;
	if (justStarted) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		justStarted = false;
	}
	//printing all connected devices
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		if (!vr.getSystem()->IsTrackedDeviceConnected(unDevice))
			continue;
		vr::VRControllerState_t state;
		if (vr.getSystem()->GetControllerState(unDevice, &state, sizeof(state)))
		{
			vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);
			switch (trackedDeviceClass) {
			case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD:
				std::cout << "HMD connected on " + std::to_string(unDevice) + "\n";
				break;
			case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker:
				std::cout << "Tracker connected on " + std::to_string(unDevice) + "\n";
			case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
				std::cout << "Controller connected on " + std::to_string(unDevice) + "\n";
			}
		}
	}
	//Listing all devices that are currently trackable, including reference points
	std::map<int, VrDevice> devices = vr.listDevices();

	//When user has no index specified when calling the .exe, we will record all devices
	if (args.deviceList.empty() == true) {
		std::cout << "No device list specified, recording all devices\n\n";
		//TIL: You can't copy a map in c++ with a simple loop-through without modifying it, crazy things will happen. So we search for all possible IDs instead.
		for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++) {
			if (devices.find(unDevice) != devices.end()) {
				args.deviceList.push_back(unDevice);
			}
		}
	}
	//Checking if user has specified an ID that is not a trackable device
	for (auto devId : args.deviceList) {
		bool found = false;
		if (devices.find(devId) == devices.end()) {
			std::cout << "Unrecognized device id: " << devId << "\n";
		}
	}

	// Set up the exporter early, to avoid having "file unavailable" errors *after* the recording
	auto fbx = setupFbx(args.filename.c_str());  
	//List of all frames
	std::map<int, std::vector<KeyFrame>> frames;

	//Printing all devices that will be recorded
	for (auto devId : args.deviceList) {
		auto dev = devices[devId];
		std::cout << "Recording " << dev.id << ": " << dev.name << "\n";
		frames.emplace(devId, std::vector<KeyFrame>());
	}

	StopWatch watch;
	std::cout << "\n\n\n";

	Console console;
	watch.start();
	std::cout << "Recording... press any key to stop.\n";
	std::cout << std::fixed;

	int consoleLines = args.deviceList.size() * 2;
	for (int i = 0; i < consoleLines; i++) {
		std::cout << "\n";
	}

	do {
		console.moveCursor(-consoleLines);

		int time = watch.time();
		//our loop for the final recording, runs as long as no key is pressed
		for (int devId : args.deviceList) {
			trackDevice(vr, devId, time, frames[devId]);
		}
	} while (!_kbhit());
	vr.stop();

	// Export to FBX
	for (int devId : args.deviceList) {
		setTransforms(fbx.scene, devices[devId].name + " - " + std::to_string(devId), frames[devId]);
	}
	cleanupFbx(fbx);

}


/*
Unused function to test the FBX API
void fbxTest2() {
	auto fbx = setupFbx("test2.fbx");

	auto empty = FbxNode::Create(fbx.scene, "Empty");
	fbx.scene->GetRootNode()->AddChild(empty);


	std::vector<KeyFrame> frames;
	frames.push_back({ 0, vr::HmdVector3_t {1, 0, 0}, vr::HmdQuaternion_t{0, 90, 0} });
	frames.push_back({ 20, vr::HmdVector3_t {0, 0, 1}, vr::HmdQuaternion_t{0, 0, 90} });
	setTransforms(fbx.scene, "Test object", frames);
	cleanupFbx(fbx);
}
*/