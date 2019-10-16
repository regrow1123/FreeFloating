#include "freefloatingapp.h"
#include "supportpoint.h"

#include <params.h>
#include <stopwatch.h>

std::unique_ptr<FreeFloatingApp> FreeFloatingApp::instance;
std::once_flag FreeFloatingApp::flag;

FreeFloatingApp& FreeFloatingApp::GetInstance()
{
	std::call_once(flag, [] {instance.reset(new FreeFloatingApp); });
	return *instance;
}

void FreeFloatingApp::Run(std::string path)
{
	model3D = Model3D::Load(path);
	StopWatch::GetInstance().Start();
	BuildSupportStructure();
}

FreeFloatingApp::FreeFloatingApp()
{
	SetParameters();

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	Params& params = Params::GetInstance();
	offscreen = glfwCreateWindow(100, 100, "FreeFloating App", 0, 0);
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

void FreeFloatingApp::SetParameters()
{
	Params& params = Params::GetInstance();

	params.sliceThickness = 0.1f;
	params.dpi = 600;
	params.pixelWidth = 25.4 / params.dpi;
	params.maxFBOSize = 4000;

	params.effectiveRadius = 5.0f;
	params.overhangAngle = 45 * 3.141592 / 180.0;
}

void FreeFloatingApp::BuildSupportStructure()
{
	SupportPointFinder supportPointFinder;
	supportPointFinder.Run(model3D.get());
}