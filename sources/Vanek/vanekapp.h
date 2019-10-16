#pragma once

#include <model3d.h>
#include <GLFW/glfw3.h>

class VanekApp
{
public:
	~VanekApp() {}

	static VanekApp& GetInstance();

	void Run(std::string);

private:
	VanekApp();

	void SetParameters();
	void BuildSupportStructure();

private:
	static std::unique_ptr<VanekApp> instance;
	static std::once_flag flag;

	GLFWwindow* offscreen;

	std::unique_ptr<Model3D> model3D;
};
