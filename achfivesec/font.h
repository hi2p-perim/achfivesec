#pragma once
#ifndef ACHFIVESEC_FONT_H
#define ACHFIVESEC_FONT_H

#include "common.h"
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct texture_atlas_t;
struct texture_font_t;

namespace fw
{
	class GLVertexArray;
	class GLVertexBuffer;
	class GLIndexBuffer;
	class GLTexture2D;
}

struct FormattedString
{
	std::wstring text;
	std::vector<glm::vec3> colors;
};

class FontText
{
public:

	FontText();
	~FontText();

private:

	FW_DISABLE_COPY_AND_MOVE(FontText);

public:

	bool Load(const std::string& path, const FormattedString& str, const glm::vec2& pos, float size, float kerningOffset);
	bool Load(const std::string& path, const std::wstring& text, const glm::vec2& pos, float size, float kerningOffset);
	void Unload();
	void Bind(int unit = 0) const;
	void Unbind() const;
	void Draw(int unit = 0) const;

private:

	unsigned char* MakeDistanceMap(unsigned char *img, int width, int height);

private:

	bool loaded;
	texture_atlas_t* atlas;
	texture_font_t* font;

private:

	int textLength;
	std::shared_ptr<fw::GLVertexArray> textVao;
	std::shared_ptr<fw::GLVertexBuffer> textPositionVbo;		// Center points (in pixels)
	std::shared_ptr<fw::GLVertexBuffer> textPositionOffsetVbo;	// Offset of positions relative to cente point (in pixels)
	std::shared_ptr<fw::GLVertexBuffer> textTexcoord0Vbo;
	std::shared_ptr<fw::GLVertexBuffer> textTexcoord1Vbo;
	std::shared_ptr<fw::GLVertexBuffer> textColorVbo;
	std::shared_ptr<fw::GLTexture2D> textAtlasDistanceMap;

};

#endif // ACHFIVESEC_FONT_H