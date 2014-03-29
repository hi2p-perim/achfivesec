#ifndef LIB_FW_CORE_GL_H
#define LIB_FW_CORE_GL_H

#include "common.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>	// glew 1.10.0
#include <string>

FW_NAMESPACE_BEGIN

//! OpenGL utilities.
class GLUtils
{
public:

	enum DebugOutputFrequency
	{
		DebugOutputFrequencyHigh,
		DebugOutputFrequencyMedium,
		DebugOutputFrequencyLow
	};

private:

	GLUtils();
	GLUtils(const GLUtils&);
	GLUtils(GLUtils&&);
	void operator=(const GLUtils&);
	void operator=(GLUtils&&);

public:

	static bool InitializeGlew(bool experimental = true);
	static bool EnableDebugOutput(DebugOutputFrequency freq = DebugOutputFrequencyHigh);
	static bool CheckExtension(const std::string& name);
	static void CheckGLErrors(const char* filename, const int line);

};

//! OpenGL vertex attribute.
class GLVertexAttribute
{
public:

	GLVertexAttribute(int index, int size)
		: index(index)
		, size(size)
	{

	}

public:

	int index;
	int size;

};

/*!
	Default vertex attributes.
	Vertex attribute defined in the framework for simpleness.
*/
class GLDefaultVertexAttribute
{
private:

	GLDefaultVertexAttribute();
	GLDefaultVertexAttribute(const GLDefaultVertexAttribute&);
	GLDefaultVertexAttribute(GLDefaultVertexAttribute&&);
	void operator=(const GLDefaultVertexAttribute&);
	void operator=(GLDefaultVertexAttribute&&);

public:

	static const GLVertexAttribute Position;
	static const GLVertexAttribute Normal;
	static const GLVertexAttribute TexCoord0;
	static const GLVertexAttribute TexCoord1;
	static const GLVertexAttribute TexCoord2;
	static const GLVertexAttribute TexCoord3;
	static const GLVertexAttribute TexCoord4;
	static const GLVertexAttribute Tangent;
	static const GLVertexAttribute Color;

};

/*!
	OpenGL resource.
	Base class for OpenGL resources.
*/
class GLResource
{
public:

	GLResource();
	virtual ~GLResource();

private:

	GLResource(const GLResource&);
	GLResource(GLResource&&);
	void operator=(const GLResource&);
	void operator=(GLResource&&);

public:

	GLuint ID();

protected:

	GLuint id;

};

class GLBufferObject : public GLResource
{
public:

	GLBufferObject();
	virtual ~GLBufferObject();

public:

	void Bind();
	void Unbind();
	void Allocate(int size, const void* data, GLenum usage);
	void Replace(int offset, int size, const void* data);
	void Clear(GLenum internalformat, GLenum format, GLenum type, const void* data);
	void Clear(GLenum internalformat, int offset, int size, GLenum format, GLenum type, const void* data);
	void Copy(GLBufferObject& writetarget, int readoffset, int writeoffset, int size);
	void Map(int offset, int length, unsigned int access, void** data);
	void Unmap();
	int Size() { return size; }

protected:

	int size;
	GLenum target;

};

class GLPixelPackBuffer : public GLBufferObject
{
public:

	GLPixelPackBuffer();

};

class GLPixelUnpackBuffer : public GLBufferObject
{
public:

	GLPixelUnpackBuffer();

};

class GLVertexBuffer : public GLBufferObject
{
public:

	GLVertexBuffer();
	void AddStatic(int n, const float* v);

};

class GLIndexBuffer : public GLBufferObject
{
public:

	GLIndexBuffer();
	void AddStatic(int n, const GLuint* idx);
	void Draw(GLenum mode);

};

class GLVertexArray : public GLResource
{
public:

	GLVertexArray();
	~GLVertexArray();

	void Bind();
	void Unbind();
	void Add(const GLVertexAttribute& attr, GLVertexBuffer* vb);
	void Add(int index, int size, GLVertexBuffer* vb);
	void Draw(GLenum mode, int count);
	void Draw(GLenum mode, int first, int count);
	void Draw(GLenum mode, GLIndexBuffer* ib);

};

enum class GLShaderType
{
	VertexShader = GL_VERTEX_SHADER,
	TessControlShader = GL_TESS_CONTROL_SHADER,
	TessEvaluationShader = GL_TESS_EVALUATION_SHADER,
	GeometryShader = GL_GEOMETRY_SHADER,
	FragmentShader = GL_FRAGMENT_SHADER
};

class GLShader : public GLResource
{	
public:

	GLShader();
	~GLShader();

	void Begin();
	void End();
	bool Compile(const std::string& path);
	bool Compile(GLShaderType type, const std::string& path);
	bool CompileString(GLShaderType type, const std::string& content);
	bool Link();
	void SetUniform(const std::string& name, const glm::mat4& mat);
	void SetUniform(const std::string& name, const glm::mat3& mat);
	void SetUniform(const std::string& name, float v);
	void SetUniform(const std::string& name, const glm::vec2& v);
	void SetUniform(const std::string& name, const glm::vec3& v);
	void SetUniform(const std::string& name, const glm::vec4& v);
	void SetUniform(const std::string& name, int v);

private:

	class Impl;
	Impl* p;

};

class GLProxyTexture2D : public GLResource
{
public:

	GLProxyTexture2D(GLuint id);

public:

	void Bind(int unit = 0);
	void Unbind();

};

class GLTexture : public GLResource
{
public:

	GLTexture();
	virtual ~GLTexture();

	void Bind(int unit = 0);
	void Unbind();

protected:

	GLenum target;

};

class GLTexture2D : public GLTexture
{
public:

	GLTexture2D();

public:

	void Allocate(int width, int height);
	void Allocate(int width, int height, GLenum internalFormat);
	void Allocate(int width, int height, GLenum internalFormat, GLenum format, GLenum type, const void* data);
	void Replace(const glm::ivec4& rect, GLenum format, GLenum type, const void* data);
	void Replace(GLPixelUnpackBuffer* pbo, const glm::ivec4& rect, GLenum format, GLenum type, int offset = 0);
	void GetInternalData(GLenum format, GLenum type, void* data);
	void GenerateMipmap();
	void UpdateTextureParams();

	int Width() { return width; }
	int Height() { return height; }
	GLenum InternalFormat() { return internalFormat; }

	GLenum MinFilter() { return minFilter; }
	GLenum MagFilter() { return magFilter; }
	GLenum Wrap() { return wrap; }
	bool AnisotropicFiltering() { return anisotropicFiltering; }
	void SetMinFilter(GLenum minFilter) { this->minFilter = minFilter; }
	void SetMagFilter(GLenum magFilter) { this->magFilter = magFilter; }
	void SetWrap(GLenum wrap) { this->wrap = wrap; }
	void SetAnisotropicFiltering(bool anisotropicFiltering) { this->anisotropicFiltering = anisotropicFiltering; }

private:

	int width;
	int height;
	GLenum internalFormat;
	GLenum minFilter;
	GLenum magFilter;
	GLenum wrap;
	bool anisotropicFiltering;

};

class GLRenderBuffer : public GLResource
{
public:

	GLRenderBuffer(int width, int height, GLenum format);
	~GLRenderBuffer();

public:

	void Bind();
	void Unbind();

};

class GLFrameBuffer : public GLResource
{
public:

	GLFrameBuffer(int width, int height, const glm::vec4& clearColor);
	~GLFrameBuffer();

public:

	void Bind();
	void Unbind();
	void AddRenderTarget(GLTexture2D* texture);
	void Begin();
	void End();

private:

	class Impl;
	Impl* p;

};

FW_NAMESPACE_END

#define FW_GL_SHADER_SOURCE(CODE) #CODE
#define FW_GL_CHECK_ERRORS() fw::GLUtils::CheckGLErrors(__FILE__, __LINE__)
#define FW_GL_SHADER_VERSION "#version 420 core\n"
#define FW_GL_VERTEX_ATTRIBUTES \
	"#define POSITION 0\n" \
	"#define NORMAL 1\n" \
	"#define TEXCOORD0 2\n" \
	"#define TEXCOORD1 3\n" \
	"#define TEXCOORD2 4\n" \
	"#define TEXCOORD3 5\n" \
	"#define TEXCOORD4 6\n" \
	"#define TANGENT 7\n" \
	"#define COLOR 8\n"

#endif // LIB_FW_CORE_GL_H