#include "pch.h"
#include "font.h"
#include "gl.h"
#include "logger.h"
#include "edtaa3func.h"
#include <freetype-gl/freetype-gl.h>

using namespace fw;

FontText::FontText()
	: loaded(false)
	, atlas(nullptr)
	, font(nullptr)
{

}

FontText::~FontText()
{
	Unload();
}

bool FontText::Load( const std::string& path, const std::wstring& text, const glm::vec2& pos, float size, float kerningOffset )
{
	FormattedString str;
	str.text = text;
	for (size_t i = 0; i < text.size(); i++)
	{
		str.colors.push_back(glm::vec3(1.0f));
	}
	return Load(path, str, pos, size, kerningOffset);
}

bool FontText::Load( const std::string& path, const FormattedString& str, const glm::vec2& pos, float size, float kerningOffset )
{
	if (loaded) return false;

	// Create texture atlas
	atlas = texture_atlas_new(512, 512, 1);
	
	// Load font
	font = texture_font_new_from_file(atlas, size, path.c_str());
	if (font == nullptr)
	{
		Unload();
		FW_LOG_ERROR("Failed to load font");
		return false;
	}

	// Load glyphs
	if (texture_font_load_glyphs(font, str.text.c_str()) > 0)
	{
		FW_LOG_ERROR("Failed to load glyphs");
		return false;
	}

	// Create atlas texture using distance map
	auto* distanceMap = MakeDistanceMap(atlas->data, (int)atlas->width, (int)atlas->height);
	textAtlasDistanceMap = std::make_shared<GLTexture2D>();
	textAtlasDistanceMap->SetMagFilter(GL_LINEAR);
	textAtlasDistanceMap->SetMinFilter(GL_LINEAR);
	textAtlasDistanceMap->SetWrap(GL_CLAMP_TO_EDGE);
	textAtlasDistanceMap->Allocate((int)atlas->width, (int)atlas->height, GL_RED, GL_RED, GL_UNSIGNED_BYTE, distanceMap);
	free(distanceMap);

	// Create vertices
	std::vector<glm::vec3> textPositions;
	std::vector<glm::vec2> textPositionOffsets;
	std::vector<glm::vec2> textTexcoords0;
	std::vector<glm::vec2> textTexcoords1;
	std::vector<glm::vec3> textColors;
	std::vector<GLuint> textIndices;

	glm::vec2 pen = pos;
	textLength = (int)str.text.size();
	for (size_t i = 0; i < str.text.size(); i++)
	{
		auto* glyph = texture_font_get_glyph(font, str.text[i]);
		if (glyph != nullptr)
		{
			float kerning = 0.0f;
			if (i > 0)
			{
				kerning = texture_glyph_get_kerning(glyph, str.text[i-1]);
			}

			pen.x += kerning + kerningOffset;

			int x0  = static_cast<int>(pen.x + glyph->offset_x);
			int y0  = static_cast<int>(pen.y + glyph->offset_y);
			int x1  = static_cast<int>(x0 + glyph->width);
			int y1  = static_cast<int>(y0 - glyph->height);

			auto center = glm::vec2(x0 + x1, y0 + y1) * 0.5f;
			textPositions.push_back(glm::vec3(center, 0.0f));
			textPositionOffsets.push_back(glm::vec2(glyph->width, glyph->height) * 0.5f);
			textTexcoords0.push_back(glm::vec2(glyph->s0, glyph->t0));
			textTexcoords1.push_back(glm::vec2(glyph->s1, glyph->t1));
			textColors.push_back(str.colors[i]);

			pen.x += glyph->advance_x;
		}
	}

	textVao = std::make_shared<GLVertexArray>();
	textPositionVbo = std::make_shared<GLVertexBuffer>();
	textPositionOffsetVbo = std::make_shared<GLVertexBuffer>();
	textTexcoord0Vbo = std::make_shared<GLVertexBuffer>();
	textTexcoord1Vbo = std::make_shared<GLVertexBuffer>();
	textColorVbo = std::make_shared<GLVertexBuffer>();

	textPositionVbo->AddStatic((int)textPositions.size() * 3, &textPositions[0].x);
	textPositionOffsetVbo->AddStatic((int)textPositionOffsets.size() * 2, &textPositionOffsets[0].x);
	textTexcoord0Vbo->AddStatic((int)textTexcoords0.size() * 2, &textTexcoords0[0].x);
	textTexcoord1Vbo->AddStatic((int)textTexcoords1.size() * 2, &textTexcoords1[0].x);
	textColorVbo->AddStatic((int)textColors.size() * 3, &textColors[0].x);
	textVao->Add(GLDefaultVertexAttribute::Position, textPositionVbo.get());
	textVao->Add(10, 2, textPositionOffsetVbo.get());
	textVao->Add(GLDefaultVertexAttribute::TexCoord0, textTexcoord0Vbo.get());
	textVao->Add(GLDefaultVertexAttribute::TexCoord1, textTexcoord1Vbo.get());
	textVao->Add(GLDefaultVertexAttribute::Color, textColorVbo.get());

	loaded = true;
	return true;
}

void FontText::Unload()
{
	loaded = false;
	if (atlas != nullptr) texture_atlas_delete(atlas);
	if (font != nullptr) texture_font_delete(font);
	textVao = nullptr;
	textPositionVbo = nullptr;
	textTexcoord0Vbo = nullptr;
	textTexcoord1Vbo = nullptr;
	textColorVbo = nullptr;
	textAtlasDistanceMap = nullptr;
}

void FontText::Draw( int unit /*= 0*/ ) const
{
	Bind(unit);
	textVao->Draw(GL_POINTS, textLength);
	glActiveTexture(GL_TEXTURE0);
	Unbind();
}

void FontText::Bind( int unit /*= 0*/ ) const
{
	textAtlasDistanceMap->Bind(unit);
	//glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
	//glBindTexture(GL_TEXTURE_2D, atlas->id);
}

void FontText::Unbind() const
{
	textAtlasDistanceMap->Unbind();
	//glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned char* FontText::MakeDistanceMap( unsigned char *img, int width, int height )
{
	short* xdist	= (short*)malloc(width * height * sizeof(short));
	short* ydist	= (short*)malloc(width * height * sizeof(short));
	double* gx		= (double*)calloc(width * height, sizeof(double));
	double* gy		= (double*)calloc(width * height, sizeof(double));
	double* data	= (double*)calloc(width * height, sizeof(double));
	double* outside	= (double*)calloc(width * height, sizeof(double));
	double* inside	= (double*)calloc(width * height, sizeof(double));

	// Convert img into double (data)
	double img_min = 255, img_max = -255;
	for(int i = 0; i<width*height; ++i)
	{
		double v = img[i];
		data[i] = v;
		if (v > img_max) img_max = v;
		if (v < img_min) img_min = v;
	}

	// Rescale image levels between 0 and 1
	for(int i = 0; i < width*height; ++i)
	{
		data[i] = (img[i]-img_min)/img_max;
	}

	// Compute outside = edtaa3(bitmap); % Transform background (0's)
	computegradient(data, height, width, gx, gy);
	edtaa3(data, gx, gy, height, width, xdist, ydist, outside);
	for(int i = 0; i < width*height; ++i)
	{
		if(outside[i] < 0)
		{
			outside[i] = 0.0;
		}
	}

	// Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
	memset(gx, 0, sizeof(double)*width*height);
	memset(gy, 0, sizeof(double)*width*height);
	for(int i = 0; i < width*height; ++i)
	{
		data[i] = 1 - data[i];
	}

	computegradient(data, height, width, gx, gy);
	edtaa3(data, gx, gy, height, width, xdist, ydist, inside);
	for(int i = 0; i < width*height; ++i)
	{
		if( inside[i] < 0 )
		{
			inside[i] = 0.0;
		}
	}

	// distmap = outside - inside; % Bipolar distance field
	unsigned char *out = (unsigned char *)malloc(width * height * sizeof(unsigned char));
	for(int i = 0; i < width*height; ++i)
	{
		outside[i] -= inside[i];
		outside[i] = 128+outside[i]*16;
		if(outside[i] < 0  ) outside[i] = 0;
		if(outside[i] > 255) outside[i] = 255;
		out[i] = 255 - (unsigned char) outside[i];
		//out[i] = (unsigned char) outside[i];
	}

	free(xdist);
	free(ydist);
	free(gx);
	free(gy);
	free(data);
	free(outside);
	free(inside);

	return out;
}