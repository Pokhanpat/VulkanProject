#include "ShaderCompile.h"

void ShaderCompile::compileShader(const std::string& filename) {
	std::string input_command{ "glslc " };
	input_command.append((SHADER_DIRECTORY / filename).generic_string());
	input_command.append(" -o ");
	input_command.append((SHADER_DIRECTORY / "compiled").generic_string());
	input_command.append("/");
	input_command.append((SHADER_DIRECTORY / filename).stem().generic_string());
	input_command.append(".spv");

	system(input_command.c_str());
}

std::vector<char> ShaderCompile::readShader(const std::string& filename) {
	std::ifstream file((SHADER_DIRECTORY/"compiled"/filename).generic_string(), std::ios::ate | std::ios::binary);
	if (!file.is_open()) { throw std::runtime_error("Failed to open file."); }

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule ShaderCompile::createShaderModule(VkDevice& device, const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to build Vulkan Shader.");
	}
	return shaderModule;
}