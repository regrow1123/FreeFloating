#include "huangapp.h"
#include "anchormap.h"

#include <params.h>
#include <stopwatch.h>

std::unique_ptr<HuangApp> HuangApp::instance;
std::once_flag HuangApp::flag;

HuangApp& HuangApp::GetInstance()
{
	std::call_once(flag, [] {instance.reset(new HuangApp); });
	return *instance;
}

void HuangApp::Run(std::string path)
{
	model3D = Model3D::Load(path);
	StopWatch::GetInstance().Start();
	BuildSupportStructure();
}

HuangApp::HuangApp()
{
	SetParameters();

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

	offscreen = glfwCreateWindow(100, 100, "Huang App", 0, 0);
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

void HuangApp::SetParameters()
{
	Params& params = Params::GetInstance();

	params.sliceThickness = 0.1f;
	params.dpi = 600;
	params.pixelWidth = 25.4 / params.dpi;
	params.maxFBOSize = 4000;

	params.selfSupportThres = 0.1f;
	params.effectiveRadius = 5.0f;
}

void HuangApp::BuildSupportStructure()
{
	AnchorMapGenerator generator;
	generator.Run(model3D.get());
}