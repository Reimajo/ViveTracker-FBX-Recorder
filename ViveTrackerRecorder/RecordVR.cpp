#include "RecordVR.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <conio.h>

#include "FbxExport.h"
#include "VR.h"
#include "StopWatch.h"
#include "Console.h"

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

void trackDevice(VR &vr,  int devId, int time, std::vector<KeyFrame>& frames) {
	vr::VRControllerState_t state;
	vr::TrackedDevicePose_t pose;
	if (vr.getSystem()->GetControllerStateWithPose(vr::TrackingUniverseStanding, devId, &state, sizeof(state), &pose)) {
		if (pose.bPoseIsValid) {
			auto pos = getPosition(pose.mDeviceToAbsoluteTracking);
			auto rot = getRotation(pose.mDeviceToAbsoluteTracking);

			frames.push_back(KeyFrame(time, pos, rot));

			std::cout << pos.v[0] << " " << pos.v[1] << " " << pos.v[2] << "\n";
			std::cout << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << "\n";
		} else {
			std::cout << "Pose is invalid\n\n";
		}
	} else {
		std::cout << "Could not get pose\n\n";
	}
}

int main(int argc, char* argv[]) {
	Args args;
	if (!parseArgs(argc, argv, args)) {
		return 0;
	}

	VR vr;
	vr.init();
	auto devices = vr.listDevices();
	for (auto devId : args.deviceList) {
		bool found = false;
		if (devices.find(devId) == devices.end()) {
			std::cout << "Unrecognized device id: " << devId << "\n";
		}
	}

	auto fbx = setupFbx(args.filename.c_str());  // Set up the exporter early, to avoid having "file unavailable" errors *after* the recording

	std::map<int, std::vector<KeyFrame>> frames;

	for (auto devId : args.deviceList) {
		auto dev = devices[devId];
		std::cout << "Recording " << dev.id << ": " << dev.name << "\n";
		frames.emplace(devId, std::vector<KeyFrame>());
	}

	StopWatch watch;
	std::cout << "\n\n\n\n";

	Console console;
	watch.start();
	std::cout << "Recording... press any key to stop.\n";
	std::cout << std::fixed;
	do {
		console.moveCursor(-2);

		int time = watch.time();

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