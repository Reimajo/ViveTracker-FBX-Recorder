#pragma once

#include <openvr.h>
#include <vector>
#include <fbxsdk.h>

struct Fbx {
public:
	FbxManager* manager;
	FbxExporter* exporter;
	FbxScene* scene;
};

struct KeyFrame {
public:
	int time;
	vr::HmdVector3_t position;
	vr::HmdQuaternion_t rotation;

	KeyFrame(int time, vr::HmdVector3_t pos, vr::HmdQuaternion_t rot): time(time), position(pos), rotation(rot) {}
};


Fbx setupFbx(const char* filename);
void cleanupFbx(Fbx fbx);
void setTransforms(fbxsdk::FbxScene* scene, const std::string& objName, const std::vector<KeyFrame>& frames);