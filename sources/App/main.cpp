#include "freefloatingapp.h"
#include "huangapp.h"
#include "vanekapp.h"

std::vector<std::string> recipes = {
	"FreeFloating",
	"Huang",
	"Vanek"
};

std::string ParseCLArgs(int argc, char** argv, std::vector<std::string>& recipes)
{
	if (argc < 2)
		exit(EXIT_FAILURE);

	std::string recipeName = argv[1];
	auto it = std::find(recipes.begin(), recipes.end(), recipeName);
	if (it == recipes.end())
		exit(EXIT_FAILURE);

	return recipeName;
}

int main(int argc, char* argv[])
{
	std::string recipe = ParseCLArgs(argc, argv, recipes);

	if (recipe == "FreeFloating")
		FreeFloatingApp::GetInstance().Run(argv[2]);
	else if (recipe == "Huang")
		HuangApp::GetInstance().Run(argv[2]);
	else if (recipe == "Vanek")
		VanekApp::GetInstance().Run(argv[2]);
}