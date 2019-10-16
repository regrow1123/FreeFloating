#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include <model3d.h>

class FreeFloatingApp
{
public:
	static FreeFloatingApp& GetInstance();

	void Run(std::string);

private:
	FreeFloatingApp();

	void SetParameters();

	void BuildSupportStructure();

private:
	static std::unique_ptr<FreeFloatingApp> instance;
	static std::once_flag flag;

	GLFWwindow* offscreen;

	std::unique_ptr<Model3D> model3D;
};
