#include "pch.h"
#include "shaderutil.h"
#include "logger.h"
#include "gl.h"
#include <ctemplate/template.h>

namespace ct = ctemplate;

std::string ShaderUtil::GenerateShaderString( const std::string& input, const ShaderTemplateDict& dict )
{
	// Create ctemplate's dictionary
	ct::TemplateDictionary tempDict("shaderStringDict");

	// Predefined values
	tempDict["GLShaderVersion"] = FW_GL_SHADER_VERSION;
	tempDict["GLVertexAttributes"] = FW_GL_VERTEX_ATTRIBUTES;

	// User-defined values
	for (auto& kv : dict)
	{
		tempDict[kv.first] = kv.second;
	}

	// Expand template
	std::string output;
	auto* tpl = ct::Template::StringToTemplate(input, ct::DO_NOT_STRIP);
	if (!tpl->Expand(&output, &tempDict))
	{
		FW_LOG_ERROR("Failed to expand template");
		return "";
	}

	return output;
}

