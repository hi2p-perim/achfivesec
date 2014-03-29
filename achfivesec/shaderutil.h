#pragma once
#ifndef ACHFIVESEC_SHADER_UTIL_H
#define ACHFIVESEC_SHADER_UTIL_H

#include "common.h"
#include <string>
#include <unordered_map>

class ShaderUtil
{
private:

	ShaderUtil() {}
	FW_DISABLE_COPY_AND_MOVE(ShaderUtil);

public:

	typedef std::unordered_map<std::string, std::string> ShaderTemplateDict;

public:

	static std::string GenerateShaderString(const std::string& input, const ShaderTemplateDict& dict);

};

#endif // ACHFIVESEC_SHADER_UTIL_H