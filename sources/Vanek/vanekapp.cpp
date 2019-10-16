#include "vanekapp.h"
#include "overhang.h"

#include <params.h>
#include <stopwatch.h>

std::unique_ptr<VanekApp> VanekApp::instance;
std::once_flag VanekApp::flag;

VanekApp& VanekApp::GetInstance()
{
	std::call_once(flag, [] {instance.reset(new VanekApp); });
	return *instance;
}

void VanekApp::Run(std::string path)
{
	model3D = Model3D::Load(path);
	StopWatch::GetInstance().Start();
	BuildSupportStructure();
}

VanekApp::VanekApp()
{
	SetParameters();

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

	offscreen = glfwCreateWindow(100, 100, "Vanek App", 0, 0);
	if (!offscreen) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(offscreen);

	if (!gladLoadGL()) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void VanekApp::SetParameters()
{
	Params& params = Params::GetInstance();

	params.sliceThickness = 0.1f;
	params.dpi = 600;
	params.pixelWidth = 25.4 / params.dpi;
	params.maxFBOSize = 4000;

	params.samplingResolution = 5.0f;
	params.overhangAngle = 45 * 3.141592 / 180.0;
}

void VanekApp::BuildSupportStructure()
{
	OverhangDetector detector;
	detector.Run(model3D.get());
}