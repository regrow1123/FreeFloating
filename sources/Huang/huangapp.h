#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include <model3d.h>

class HuangApp
{
public:
	~HuangApp() {}

	static HuangApp& GetInstance();

	void Run(std::string);

private:
	HuangApp();

	void SetParameters();
	void BuildSupportStructure();

private:
	static std::unique_ptr<HuangApp> instance;
	static std::once_flag flag;

	GLFWwindow* offscreen;

	std::unique_ptr<Model3D> model3D;
};
