#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdlib>

const std::filesystem::path SHADER_DIRECTORY = std::filesystem::current_path() / "shaders";

namespace ShaderCompile {
	void compileShader(const std::string& filename);
	std::vector<char> readShader(const std::string& filename);
	VkShaderModule createShaderModule(VkDevice& device, const std::vector<char>& code);
}