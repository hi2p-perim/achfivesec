#include "pch.h"
#include "gl.h"
#include "logger.h"

FW_NAMESPACE_BEGIN

static void _stdcall DebugOutput( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam )
{
	std::string sourceString;
	std::string typeString;
	std::string severityString;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API_ARB:
		{
			sourceString = "OpenGL";
			break;
		}

		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
		{
			sourceString = "Windows";
			break;
		}

		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
		{
			sourceString = "Shader Compiler";
			break;
		}

		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
		{
			sourceString = "Third Party";
			break;
		}

		case GL_DEBUG_SOURCE_APPLICATION_ARB:
		{
			sourceString = "Application";
			break;
		}

		case GL_DEBUG_SOURCE_OTHER_ARB:
		default:
		{
			sourceString = "Other";
			break;
		}
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR_ARB:
		{
			typeString = "Error";
			break;
		}

		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
		{
			typeString = "Deprecated behavior";
			break;
		}

		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		{
			typeString = "Undefined behavior";
			break;
		}

		case GL_DEBUG_TYPE_PORTABILITY_ARB:
		{
			typeString = "Portability";
			break;
		}

		case GL_DEBUG_TYPE_OTHER_ARB:
		default:
		{
			typeString = "Message";
			break;
		}
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:
		{
			severityString = "High";
			break;
		}

		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		{
			severityString = "Medium";
			break;
		}

		case GL_DEBUG_SEVERITY_LOW_ARB:
		{
			severityString = "Low";
			break;
		}
	}

	std::string str =
		(boost::format("%s: %s(%s) %d: %s\n")
		% sourceString % typeString % severityString % id % message).str();

	if (severity == GL_DEBUG_SEVERITY_LOW_ARB)
	{
		FW_LOG_INFO(str);
	}
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
	{
		FW_LOG_WARN(str);
	}
	else if (severity == GL_DEBUG_SEVERITY_HIGH)
	{
		FW_LOG_ERROR(str);
	}
}

bool GLUtils::InitializeGlew(bool experimental)
{
	if (experimental)
	{
		// Some extensions e.g. GL_ARB_debug_output doesn't work
		// unless in the experimental mode.
		glewExperimental = GL_TRUE;
	}
	else
	{
		glewExperimental = GL_FALSE;
	}

	GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		FW_LOG_ERROR(boost::str(boost::format("Failed to initialize GLEW: %s") % glewGetErrorString(err)));
		return false;
	}

	// GLEW causes GL_INVALID_ENUM on GL 3.2 core or higher profiles
	// because GLEW uses glGetString(GL_EXTENSIONS) internally.
	// cf. http://www.opengl.org/wiki/OpenGL_Loading_Library
	glGetError();
	FW_GL_CHECK_ERRORS();

	return true;
}

bool GLUtils::EnableDebugOutput(DebugOutputFrequency freq)
{
	// Initialize GL_ARB_debug_output
	if (GLEW_ARB_debug_output)
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

		if (freq == DebugOutputFrequencyMedium)
		{
			glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_FALSE);
		}
		else if (freq == DebugOutputFrequencyLow)
		{
			glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM_ARB, 0, NULL, GL_FALSE);
			glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_FALSE);
		}
			
		glDebugMessageCallbackARB(&DebugOutput, NULL);
	}
	else
	{
		FW_LOG_ERROR("GL_ARB_debug_output is not supported");
		return false;
	}

	return true;
}

bool GLUtils::CheckExtension( const std::string& name )
{
	GLint c;

	glGetIntegerv(GL_NUM_EXTENSIONS, &c);

	for (GLint i = 0; i < c; ++i)
	{
		std::string s = (char const*)glGetStringi(GL_EXTENSIONS, i);
		if (s == name)
		{
			return true;
		}
	}

	return false;
}

void GLUtils::CheckGLErrors( const char* filename, const int line )
{
	int err;

	if ((err = glGetError()) != GL_NO_ERROR)
	{
		std::string errstr;

		switch (err)
		{
			case GL_INVALID_ENUM:
				errstr = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				errstr = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				errstr = "GL_INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errstr = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				errstr = "GL_OUT_OF_MEMORY";
				break;
		}

		FW_LOG_ERROR(Logger::FormattedDebugInfo(filename, line) + errstr);
	}
}

// ----------------------------------------------------------------------

const GLVertexAttribute GLDefaultVertexAttribute::Position(0, 3);
const GLVertexAttribute GLDefaultVertexAttribute::Normal(1, 3);
const GLVertexAttribute GLDefaultVertexAttribute::TexCoord0(2, 2);
const GLVertexAttribute GLDefaultVertexAttribute::TexCoord1(3, 2);
const GLVertexAttribute GLDefaultVertexAttribute::TexCoord2(4, 2);
const GLVertexAttribute GLDefaultVertexAttribute::TexCoord3(5, 2);
const GLVertexAttribute GLDefaultVertexAttribute::TexCoord4(6, 2);
const GLVertexAttribute GLDefaultVertexAttribute::Tangent(7, 2);
const GLVertexAttribute GLDefaultVertexAttribute::Color(8, 3);

// ----------------------------------------------------------------------

GLResource::GLResource()
{

}

GLResource::~GLResource()
{

}

unsigned int GLResource::ID()
{
	return id;
}

// ----------------------------------------------------------------------

GLBufferObject::GLBufferObject()
{
	glGenBuffers(1, &id);
}

GLBufferObject::~GLBufferObject()
{
	glDeleteBuffers(1, &id);
}

void GLBufferObject::Bind()
{
	glBindBuffer(target, ID());
}

void GLBufferObject::Unbind()
{
	glBindBuffer(target, 0);
}

void GLBufferObject::Allocate( int size, const void* data, GLenum usage )
{
	Bind();
	glBufferData(target, size, data, usage);
	Unbind();
	this->size = size;
}

void GLBufferObject::Replace( int offset, int size, const void* data )
{
	Bind();
	glBufferSubData(target, offset, size, data);
	Unbind();
}

void GLBufferObject::Clear( GLenum internalformat, GLenum format, GLenum type, const void* data )
{
	Bind();
	glClearBufferData(target, internalformat, format, type, data);
	Unbind();
}

void GLBufferObject::Clear( GLenum internalformat, int offset, int size, GLenum format, GLenum type, const void* data )
{
	Bind();
	glClearBufferSubData(target, internalformat, offset, size, format, type, data);
	Unbind();
}

void GLBufferObject::Copy( GLBufferObject& writetarget, int readoffset, int writeoffset, int size )
{
	Bind();
	writetarget.Bind();
	glCopyBufferSubData(target, writetarget.target, readoffset, writeoffset, size);
	writetarget.Unbind();
	Unbind();
}

void GLBufferObject::Map( int offset, int length, unsigned int access, void** data )
{
	Bind();
	*data = glMapBufferRange(target, offset, length, access);
}

void GLBufferObject::Unmap()
{
	glUnmapBuffer(target);
	Unbind();
}

// ----------------------------------------------------------------------

GLPixelPackBuffer::GLPixelPackBuffer()
{
	target = GL_PIXEL_PACK_BUFFER;
}

// ----------------------------------------------------------------------

GLPixelUnpackBuffer::GLPixelUnpackBuffer()
{
	target = GL_PIXEL_UNPACK_BUFFER;
}

// ----------------------------------------------------------------------

GLVertexBuffer::GLVertexBuffer()
{
	target = GL_ARRAY_BUFFER;
}

void GLVertexBuffer::AddStatic( int n, const float* v )
{
	Allocate(n * sizeof(float), v, GL_STATIC_DRAW);
}

// ----------------------------------------------------------------------

GLIndexBuffer::GLIndexBuffer()
{
	target = GL_ELEMENT_ARRAY_BUFFER;
}

void GLIndexBuffer::AddStatic( int n, const GLuint* idx )
{
	Allocate(n * sizeof(GLuint), idx, GL_STATIC_DRAW);
}

void GLIndexBuffer::Draw( GLenum mode )
{
	Bind();
	glDrawElements(mode, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);
	Unbind();
}

// ----------------------------------------------------------------------

GLVertexArray::GLVertexArray()
{
	glGenVertexArrays(1, &id);
}

GLVertexArray::~GLVertexArray()
{
	glDeleteVertexArrays(1, &id);
}

void GLVertexArray::Bind()
{
	glBindVertexArray(ID());
}

void GLVertexArray::Unbind()
{
	glBindVertexArray(0);
}

void GLVertexArray::Add( const GLVertexAttribute& attr, GLVertexBuffer* vb )
{
	Bind();
	vb->Bind();
	glVertexAttribPointer(attr.index, attr.size, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attr.index);
	vb->Unbind();
	Unbind();
}

void GLVertexArray::Add( int index, int size, GLVertexBuffer* vb )
{
	Bind();
	vb->Bind();
	glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(index);
	vb->Unbind();
	Unbind();
}

void GLVertexArray::Draw( GLenum mode, GLIndexBuffer* ib )
{
	Bind();
	ib->Draw(mode);
	Unbind();
}

void GLVertexArray::Draw( GLenum mode, int count )
{
	Bind();
	glDrawArrays(mode, 0, count);
	Unbind();
}

void GLVertexArray::Draw( GLenum mode, int first, int count )
{
	Bind();
	glDrawArrays(mode, first, count);
	Unbind();
}

// ----------------------------------------------------------------------

class GLShader::Impl
{
public:

	std::string ShaderTypeString(GLShaderType type);
	bool InferShaderType(const std::string& path, GLShaderType& type);
	bool LoadShaderFile(const std::string& path, std::string& content);
	GLuint GetOrCreateUniformID(const std::string& name);

public:

	GLuint id;
	typedef boost::unordered_map<std::string, GLuint> UniformLocationMap;
	UniformLocationMap uniformLocationMap;

};

GLShader::GLShader()
	: p(new Impl)
{
	id = p->id = glCreateProgram();
}

GLShader::~GLShader()
{
	glDeleteProgram(ID());
	FW_SAFE_DELETE(p);
}

void GLShader::Begin()
{
	glUseProgram(ID());
}

void GLShader::End()
{
	glUseProgram(0);
}

bool GLShader::Compile( const std::string& path )
{
	GLShaderType type;
	
	if (!p->InferShaderType(path, type) || Compile(type, path))
	{
		return false;
	}

	return true;
}

bool GLShader::Compile( GLShaderType type, const std::string& path )
{
	std::string content;
	
	if (!p->LoadShaderFile(path, content) || CompileString(type, content))
	{
		return false;
	}
	
	return true;
}

void GLShader::SetUniform( const std::string& name, const glm::mat4& mat )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniformMatrix4fv(uniformID, 1, GL_FALSE, glm::value_ptr(mat));
}

void GLShader::SetUniform( const std::string& name, const glm::mat3& mat )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniformMatrix3fv(uniformID, 1, GL_FALSE, glm::value_ptr(mat));
}

void GLShader::SetUniform( const std::string& name, float v )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniform1f(uniformID, v);
}

void GLShader::SetUniform( const std::string& name, const glm::vec2& v )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniform2fv(uniformID, 1, glm::value_ptr(v));
}

void GLShader::SetUniform( const std::string& name, const glm::vec3& v )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniform3fv(uniformID, 1, glm::value_ptr(v));
}

void GLShader::SetUniform( const std::string& name, const glm::vec4& v )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniform4fv(uniformID, 1, glm::value_ptr(v));
}

void GLShader::SetUniform( const std::string& name, int v )
{
	GLuint uniformID = p->GetOrCreateUniformID(name);
	glUniform1i(uniformID, v);
}

bool GLShader::CompileString( GLShaderType type, const std::string& content )
{
	// Create and compile shader
	GLuint shaderID = glCreateShader((GLenum)type);
	const char* contentPtr = content.c_str();

	glShaderSource(shaderID, 1, &contentPtr, NULL);
	glCompileShader(shaderID);

	// Check errors
	int ret;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &ret);

	if (ret == 0)
	{
		// Required size
		int length;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);

		// Get info log
		boost::scoped_array<char> infoLog(new char[length]);
		glGetShaderInfoLog(shaderID, length, NULL, infoLog.get());
		glDeleteShader(shaderID);

		std::stringstream ss;
		ss << "[" << p->ShaderTypeString(type) << "]" << std::endl;
		ss << infoLog.get() << std::endl;

		FW_LOG_ERROR(ss.str());
		return false;
	}

	// Attach to the program
	glAttachShader(id, shaderID);
	glDeleteShader(shaderID);

	return true;
}

bool GLShader::Link()
{
	// Link program
	glLinkProgram(id);

	// Check error
	GLint ret;
	glGetProgramiv(id, GL_LINK_STATUS, &ret);

	if (ret == GL_FALSE)
	{
		// Required size
		GLint length;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);

		boost::scoped_array<char> infoLog(new char[length]);
		glGetProgramInfoLog(id, length, NULL, infoLog.get());

		FW_LOG_ERROR(infoLog.get());
		return false;
	}

	return true;
}

std::string GLShader::Impl::ShaderTypeString( GLShaderType type )
{
	switch (type)
	{
		case GLShaderType::VertexShader:
			return "VertexShader";

		case GLShaderType::TessControlShader:
			return "TessControlShader";

		case GLShaderType::TessEvaluationShader:
			return "TessEvaluationShader";

		case GLShaderType::GeometryShader:
			return "GeometryShader";

		case GLShaderType::FragmentShader:
			return "FragmentShader";
	}

	return "";
}

bool GLShader::Impl::InferShaderType( const std::string& path, GLShaderType& type )
{
	std::string extension = path.substr(path.find_last_of("."));

	if (extension == ".vert" || extension == ".vsh")
	{
		type = GLShaderType::VertexShader;
	}
	else if (extension == ".tessctrl" || extension == ".tcsh")
	{
		type = GLShaderType::TessControlShader;
	}
	else if (extension == ".tesseval" || extension == ".tesh")
	{
		type = GLShaderType::TessEvaluationShader;
	}
	else if (extension == ".geom" || extension == ".gsh")
	{
		type = GLShaderType::GeometryShader;
	}
	else if (extension == ".frag" || extension == ".fsh")
	{
		type = GLShaderType::FragmentShader;
	}
	else
	{
		FW_LOG_ERROR("Invalid shader file extension " + extension);
		return false;
	}

	return true;
}

bool GLShader::Impl::LoadShaderFile( const std::string& path, std::string& content )
{
	std::ifstream ifs(path.c_str(), std::ios::in);

	if (!ifs.is_open())
	{
		FW_LOG_ERROR("Failed to load shader " + path);
		return false;
	}

	std::string line = "";
	content = "";

	while (std::getline(ifs, line))
	{
		content += ("\n" + line);
	}

	ifs.close();
	return true;
}

GLuint GLShader::Impl::GetOrCreateUniformID( const std::string& name )
{
	UniformLocationMap::iterator it = uniformLocationMap.find(name);

	if (it == uniformLocationMap.end())
	{
		GLuint loc = glGetUniformLocation(id, name.c_str());

		uniformLocationMap[name] = loc;

		return loc;
	}

	return it->second;
}

// ----------------------------------------------------------------------

GLProxyTexture2D::GLProxyTexture2D( GLuint id )
{
	this->id = id;
}

void GLProxyTexture2D::Bind( int unit )
{
	glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
	glBindTexture(GL_TEXTURE_2D, id);
}

void GLProxyTexture2D::Unbind()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// ----------------------------------------------------------------------

GLTexture::GLTexture()
{
	glGenTextures(1, &id);
}

GLTexture::~GLTexture()
{
	glDeleteTextures(1, &id);
}

void GLTexture::Bind( int unit )
{
	glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
	glBindTexture(target, id);
}

void GLTexture::Unbind()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(target, 0);
}

// ----------------------------------------------------------------------

GLTexture2D::GLTexture2D()
{
	target = GL_TEXTURE_2D;
	minFilter = GL_LINEAR_MIPMAP_LINEAR;
	magFilter = GL_LINEAR;
	wrap = GL_REPEAT;
	anisotropicFiltering = true;
}

void GLTexture2D::Allocate( int width, int height )
{
	Allocate(width, height, GL_RGBA8);
}

void GLTexture2D::Allocate( int width, int height, GLenum internalFormat )
{
	Allocate(width, height, internalFormat, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void GLTexture2D::Allocate( int width, int height, GLenum internalFormat, GLenum format, GLenum type, const void* data )
{
	this->width = width;
	this->height = height;
	this->internalFormat = internalFormat;

	Bind();
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
	GenerateMipmap();
	UpdateTextureParams();
	Unbind();
}

void GLTexture2D::Replace( const glm::ivec4& rect, GLenum format, GLenum type, const void* data )
{
	Bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.z, rect.w, format, type, data);
	GenerateMipmap();
	Unbind();
}

void GLTexture2D::Replace( GLPixelUnpackBuffer* pbo, const glm::ivec4& rect, GLenum format, GLenum type, int offset /*= 0*/ )
{
	pbo->Bind();
	Replace(rect, format, type, (void*)offset);
	pbo->Unbind();
}

void GLTexture2D::GetInternalData( GLenum format, GLenum type, void* data )
{
	Bind();
	glGetTexImage(GL_TEXTURE_2D, 0, format, type, data);
	Unbind();
}

void GLTexture2D::GenerateMipmap()
{
	if (minFilter == GL_LINEAR_MIPMAP_LINEAR && magFilter == GL_LINEAR)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void GLTexture2D::UpdateTextureParams()
{
	if (anisotropicFiltering)
	{
		// If the anisotropic filtering can be used,
		// set to the maximum possible value.
		float maxAnisoropy;

		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoropy);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisoropy);
	}

	// Wrap mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

	// Filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

// ------------------------------------------------------------------------

GLRenderBuffer::GLRenderBuffer( int width, int height, GLenum format )
{
	glGenRenderbuffers(1, &id);
	Bind();
	glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
	Unbind();
}

GLRenderBuffer::~GLRenderBuffer()
{
	glDeleteRenderbuffers(1, &id);
}

void GLRenderBuffer::Bind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, id);
}

void GLRenderBuffer::Unbind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

// ------------------------------------------------------------------------

class GLFrameBuffer::Impl
{
public:

	int width;
	int height;
	glm::vec4 clearColor;
	GLRenderBuffer* depthStencilRBO;
	std::vector<GLenum> colorAttachmentList;
	std::vector<GLTexture2D*> renderTargets;
	glm::ivec4 viewport;

};

GLFrameBuffer::GLFrameBuffer( int width, int height, const glm::vec4& clearColor )
	: p(new Impl)
{
	p->width = width;
	p->height = height;
	p->clearColor = clearColor;

	p->depthStencilRBO = new GLRenderBuffer(width, height, GL_DEPTH_STENCIL);

	// Generate FBO
	glGenFramebuffers(1, &id);

	// Attach to FBO
	Bind();
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, p->depthStencilRBO->ID());

	// Check FBO status
	GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		{
			FW_LOG_ERROR("FBO is not complete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		}
		else if (status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
		{
			FW_LOG_ERROR("FBO is not complete: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT");
		}
		else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		{
			FW_LOG_ERROR("FBO is not complete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		}
		else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
		{
			FW_LOG_ERROR("FBO is not complete: GL_FRAMEBUFFER_UNSUPPORTED");
		}
	}

	glDrawBuffer(GL_NONE);

	Unbind();
}

GLFrameBuffer::~GLFrameBuffer()
{
	glDeleteFramebuffers(1, &id);
	FW_SAFE_DELETE(p->depthStencilRBO);
	FW_SAFE_DELETE(p);
}

void GLFrameBuffer::Bind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
}

void GLFrameBuffer::Unbind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void GLFrameBuffer::AddRenderTarget( GLTexture2D* texture )
{
	GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + (int)p->colorAttachmentList.size());

	p->colorAttachmentList.push_back(attachment);
	p->renderTargets.push_back(texture);

	Bind();
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->ID(), 0);
	Unbind();
}

void GLFrameBuffer::Begin()
{
	// Save current viewport
	glGetIntegerv(GL_VIEWPORT, &p->viewport.x);

	Bind();

	// Enable buffers
	glDrawBuffers((int)p->colorAttachmentList.size(), &p->colorAttachmentList[0]);

	float depth = 1.0f;
	glClearBufferfv(GL_DEPTH, 0, &depth);
	for (int i = 0; i < (int)p->colorAttachmentList.size(); i++)
	{
		// Second argument of glClearBufferfv is not GL_COLOR_ATTACHMENTi​ values
		// but the buffer index specified by glDrawBuffers
		// cf. https://www.opengl.org/wiki/Framebuffer#Buffer_clearing
		glClearBufferfv(GL_COLOR, i, glm::value_ptr(p->clearColor));
	}
	glViewportIndexedfv(0, glm::value_ptr(glm::vec4(0.0f, 0.0f, (float)p->width, (float)p->height)));
}

void GLFrameBuffer::End()
{
	Unbind();

	// Generate mipmap if needed
	for (size_t i = 0; i < p->renderTargets.size(); i++)
	{
		p->renderTargets[i]->Bind();
		p->renderTargets[i]->GenerateMipmap();
		p->renderTargets[i]->Unbind();
	}

	// Restore
	glDrawBuffer(GL_BACK_LEFT);
	glViewportIndexedfv(
		0,
		glm::value_ptr(glm::vec4(
			(float)p->viewport[0], (float)p->viewport[1],
			(float)p->viewport[2], (float)p->viewport[3])));
}

FW_NAMESPACE_END