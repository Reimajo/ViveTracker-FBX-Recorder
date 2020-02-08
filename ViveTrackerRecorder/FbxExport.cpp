#include "FbxExport.h"
#include <vector>

vr::HmdVector3_t toEulerAngles(vr::HmdQuaternion_t q) {
	vr::HmdVector3_t angles;

	// roll (x-axis rotation)
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.v[0] = std::atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	double sinp = 2 * (q.w * q.y - q.z * q.x);
	if (std::abs(sinp) >= 1)
		angles.v[1] = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		angles.v[1] = std::asin(sinp);

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.v[2] = std::atan2(siny_cosp, cosy_cosp);

	for (int i = 0; i <= 2; i++) {
		angles.v[i] *= 180 / M_PI;
	}

	return angles;
}

Fbx setupFbx(const char* filename) {
	auto manager = FbxManager::Create();
	auto settings = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(settings);

	auto exporter = FbxExporter::Create(manager, "");
	if (!exporter->Initialize(filename, IOSBinary, manager->GetIOSettings())) {
		throw std::exception((std::string("Could not init fbx exporter: ") + exporter->GetStatus().GetErrorString()).c_str());
	}

	auto scene = fbxsdk::FbxScene::Create(manager, "Recorded VR");
	scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	scene->GetGlobalSettings().SetTimeMode(FbxTime::eFrames1000);

	return {
		manager, exporter, scene
	};
}

void cleanupFbx(Fbx fbx) {
	fbx.exporter->Export(fbx.scene);

	fbx.exporter->Destroy();
	fbx.scene->Destroy();
	fbx.manager->Destroy();
}

void setTransforms(fbxsdk::FbxScene* scene, const std::string& objName, const std::vector<KeyFrame>& frames) {
	auto node = fbxsdk::FbxNode::Create(scene, objName.c_str());
	node->LclTranslation.Set(FbxDouble3(0, 0, 0));
	node->LclRotation.Set(FbxDouble3(0, 0, 0));
	scene->GetRootNode()->AddChild(node);

	auto animStack = FbxAnimStack::Create(scene, (objName + " stack").c_str());
	auto animLayer = FbxAnimLayer::Create(scene, (objName + " layer").c_str());
	animStack->AddMember(animLayer);

	auto curveTx = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	auto curveTy = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	auto curveTz = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	auto curveRx = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	auto curveRy = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	auto curveRz = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	curveTx->KeyModifyBegin();
	curveTy->KeyModifyBegin();
	curveTz->KeyModifyBegin();

	curveRx->KeyModifyBegin();
	curveRy->KeyModifyBegin();
	curveRz->KeyModifyBegin();

	int last[6] = { 0,0,0,0,0,0 };

	FbxTime time;
	FbxAnimCurveKey key;
	for (auto frame : frames) {
		time.SetMilliSeconds(frame.time);

		key.Set(time, frame.position.v[0]); curveTx->KeyAdd(time, key, &last[0]);
		key.Set(time, frame.position.v[1]); curveTy->KeyAdd(time, key, &last[1]);
		key.Set(time, frame.position.v[2]); curveTz->KeyAdd(time, key, &last[2]);

		auto euler = toEulerAngles(frame.rotation);

		key.Set(time, euler.v[0]); curveRx->KeyAdd(time, key, &last[3]);
		key.Set(time, euler.v[1]); curveRy->KeyAdd(time, key, &last[4]);
		key.Set(time, euler.v[2]); curveRz->KeyAdd(time, key, &last[5]);
	}

	curveTx->KeyModifyEnd();
	curveTy->KeyModifyEnd();
	curveTz->KeyModifyEnd();

	curveRx->KeyModifyEnd();
	curveRy->KeyModifyEnd();
	curveRz->KeyModifyEnd();
}