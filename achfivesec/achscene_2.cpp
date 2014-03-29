#include "pch.h"
#include "achscene_2.h"
#include "util.h"
#include "logger.h"
#include "gl.h"
#include "shaderutil.h"
#include "font.h"
#include <sync/sync.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <boost/regex.hpp>

using namespace fw;

namespace
{

	const std::string RenderVs =
		FW_GL_SHADER_SOURCE(

			{{GLShaderVersion}}
			{{GLVertexAttributes}}

			layout (location = POSITION) in vec3 position;
			layout (location = NORMAL) in vec3 normal;
			layout (location = TEXCOORD0) in vec2 texcoord;

			out vec3 vNormal;
			out vec2 vTexcoord;
			out vec3 vViewvec;

			uniform mat4 ModelMatrix;
			uniform mat4 ViewMatrix;
			uniform mat4 ProjectionMatrix;

			void main()
			{
				mat4 mvMatrix = ViewMatrix * ModelMatrix;
				mat4 mvpMatrix = ProjectionMatrix * mvMatrix;
				mat3 normalMatrix = mat3(transpose(inverse(mvMatrix)));
				
				vNormal = normalMatrix * normal;
				vTexcoord = texcoord;
				vViewvec = vec3(mvMatrix * vec4(position, 1));

				gl_Position = mvpMatrix * vec4(position, 1);
			}

		);

	const std::string RenderFs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}
		
			in vec3 vNormal;
			in vec2 vTexcoord;
			in vec3 vViewvec;

			out vec4 fragColor;

			uniform sampler2D RT;
			uniform bool Mode;

			const vec3 lightDir = vec3(1);

			void main()
			{
				//vec3 nvNormal = normalize(vNormal);
				//vec3 nLightDir = normalize(lightDir);
				//fragColor.rgb = vec3(1, 0, 0);
				//fragColor.rgb = vec3(max(dot(nLightDir, nvNormal), 0));

				if (Mode)
				{
					vec4 c = texture(RT, vTexcoord);
					fragColor.rgb = c.rgb;
					fragColor.a = c.a;
				}
				else
				{
					fragColor.rgb = vec3(0.5);
					fragColor.a = 1;
				}
			}

		);

	const std::string QuadVs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}
			{{GLVertexAttributes}}

			layout (location = POSITION) in vec3 position;
			out vec2 vTexCoord;

			void main()
			{
				vTexCoord = (position.xy + 1) * 0.5;
				gl_Position = vec4(position, 1);
			}

		);

	const std::string GaussianBlurFs =
		FW_GL_SHADER_SOURCE(
			
			{{GLShaderVersion}}

			in vec2 vTexCoord;
			out vec4 fragColor;

			uniform int Orientation; // 0 : horizontal, 1 : vertical
			uniform vec2 TexelSize;
			uniform sampler2D RT;
			uniform float SigmaFactor; // = 0.5
			uniform int KernelSize; // = 40
			uniform float BlurStrength;// = 1;

			float Gaussian(float x, float sigma2)
			{
				return (1 / sqrt(3.14159265358979 * sigma2 * 2)) * exp(-((x*x) / (sigma2 * 2)));
			}

			void main()
			{
				vec3 color = vec3(0);
				float sigma = float(KernelSize) * SigmaFactor;
				float sigma2 = sigma * sigma;
				float strength = 1.0 - BlurStrength;

				vec2 offset;
				if (Orientation == 0)
				{
					offset = vec2(TexelSize.x, 0);
				}
				else
				{
					offset = vec2(0, TexelSize.y);
				}

				for (int i = -KernelSize; i <= KernelSize; i++)
				{
					vec2 vOffset = offset * float(i);
					color +=
						texture(RT, vTexCoord + vOffset).rgb *
						Gaussian(offset * strength, sigma2);
				}

				fragColor.rgb = color;
				fragColor.a = texture(RT, vTexCoord).a;
			}

		);

}

class LogStream : public Assimp::LogStream
{
public:

	LogStream(Logger::LogLevel level)
		: level(level)
	{

	}

	virtual void write( const char* message )
	{
		// Remove new line
		std::string str(message);
		str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

		// Remove initial string
		boost::regex re("[a-zA-Z]+, +T[0-9]+: (.*)");
		str = boost::regex_replace(str, re, "$1");

		switch (level)
		{
			case Logger::LogLevel::Debug:
				FW_LOG_DEBUG(str);
				break;
			case Logger::LogLevel::Warning:
				FW_LOG_WARN(str);
				break;
			case Logger::LogLevel::Error:
				FW_LOG_ERROR(str);
				break;
			default:
				FW_LOG_INFO(str);
		}
	}

private:

	Logger::LogLevel level;

};

bool AchScene_2::Setup( sf::RenderWindow& window, sync_device* rocket )
{
	// Tracks
	track_Scale = sync_get_track(rocket, "achscene2.Scale");
	track_X = sync_get_track(rocket, "achscene2.X");

	track_Scale2 = sync_get_track(rocket, "achscene2.Scale2");
	track_X2 = sync_get_track(rocket, "achscene2.X2");
	track_X3 = sync_get_track(rocket, "achscene2.X3");
	track_TexIndex2 = sync_get_track(rocket, "achscene2.TexIndex2");
	track_TexIndex3 = sync_get_track(rocket, "achscene2.TexIndex3");

	track_SigmaFactor = sync_get_track(rocket, "achscene.SigmaFactor");
	track_KernelSize = sync_get_track(rocket, "achscene.KernelSize");
	track_BlurStrength = sync_get_track(rocket, "achscene.BlurStrength");

	// --------------------------------------------------------------------------------

	if (!skyTexture.loadFromFile("sky.png"))
	{
		FW_LOG_ERROR("Failed to load image");
		return false;
	}

	skySprite.setTexture(skyTexture);
	skySprite.setPosition(0.0f, 0.0f);

	// --------------------------------------------------------------------------------

	// Shaders
	ShaderUtil::ShaderTemplateDict dict;

	FW_LOG_INFO("Loading renderShader");
	renderShader = std::make_shared<GLShader>();
	renderShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(RenderVs, dict));
	renderShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(RenderFs, dict));
	renderShader->Link();

	FW_LOG_INFO("Loading gaussianBlurShader");
	gaussianBlurShader = std::make_shared<GLShader>();
	gaussianBlurShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
	gaussianBlurShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(GaussianBlurFs, dict));
	gaussianBlurShader->Link();

	// --------------------------------------------------------------------------------

	// Sign textures
	const std::string signTexturePaths[] = 
	{
		"tsugaku.png",
		"susume.png",
		"tsukodome.png",
		"oudan.png"
	};

	for (const auto& path : signTexturePaths)
	{
		sf::Image image;
		if (!image.loadFromFile(path))
		{
			FW_LOG_ERROR("Failed to load " + path);
			return false;
		}

		auto size = image.getSize();
		auto texture = std::make_shared<GLTexture2D>();
		texture->SetMagFilter(GL_LINEAR);
		texture->SetMinFilter(GL_LINEAR);
		texture->SetWrap(GL_CLAMP_TO_EDGE);
		texture->Allocate(size.x, size.y, GL_RGBA16F, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
		
		signTextures.push_back(texture);
	}

	// --------------------------------------------------------------------------------

	// Mesh for traffic sign
	quadVao = std::make_shared<GLVertexArray>();
	quadPositionVbo = std::make_shared<GLVertexBuffer>();
	quadTexcoordVbo = std::make_shared<GLVertexBuffer>();
	quadIbo = std::make_shared<GLIndexBuffer>();

	const glm::vec3 quadPositions[] =
	{
		glm::vec3( 1.0f,  1.0f, 0.0f),
		glm::vec3(-1.0f,  1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3( 1.0f, -1.0f, 0.0f)
	};

	const glm::vec2 quadTexcoords[] =
	{
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(0.0f, 1.0f)
	};

	const GLuint quadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	quadPositionVbo->AddStatic(12, &quadPositions[0].x);
	quadVao->Add(GLDefaultVertexAttribute::Position, quadPositionVbo.get());

	quadTexcoordVbo->AddStatic(8, &quadTexcoords[0].x);
	quadVao->Add(GLDefaultVertexAttribute::TexCoord0, quadTexcoordVbo.get());
	
	quadIbo->AddStatic(6, quadIndices);

	// --------------------------------------------------------------------------------

	// Vertex data
	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> texcoords;
	std::vector<unsigned int> faces;

	{
		FW_LOG_INDENTER();

		// Prepare for the logger of Assimp
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		Assimp::DefaultLogger::get()->attachStream(new LogStream(Logger::LogLevel::Information), Assimp::Logger::Info);
		Assimp::DefaultLogger::get()->attachStream(new LogStream(Logger::LogLevel::Warning), Assimp::Logger::Warn);
		Assimp::DefaultLogger::get()->attachStream(new LogStream(Logger::LogLevel::Error), Assimp::Logger::Err);
#ifdef _DEBUG
		Assimp::DefaultLogger::get()->attachStream(new LogStream(Logger::LogLevel::Debug), Assimp::Logger::Debugging);
#endif

		// Load file
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile("pole.obj",
			//aiProcess_GenNormals |
			aiProcess_GenSmoothNormals |
			//aiProcess_CalcTangentSpace |
			aiProcess_Triangulate);
			//aiProcess_JoinIdenticalVertices);

		if (!scene)
		{
			FW_LOG_ERROR(importer.GetErrorString());
			return false;
		}

		// Load triangle meshes
		// TODO : select mesh by name
		unsigned int lastNumFaces = 0;
		for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++)
		{
			auto* mesh = scene->mMeshes[meshIdx];

			// Positions and normals
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				auto& p = mesh->mVertices[i];
				auto& n = mesh->mNormals[i];
				positions.push_back(p.x);
				positions.push_back(p.y);
				positions.push_back(p.z);
				normals.push_back(n.x);
				normals.push_back(n.y);
				normals.push_back(n.z);
			}

			// Texture coordinates
			if (mesh->HasTextureCoords(0))
			{
				for (unsigned int i = 0; i < mesh->mNumVertices; i++)
				{
					auto& uv = mesh->mTextureCoords[0][i];
					texcoords.push_back(uv.x);
					texcoords.push_back(uv.y);
				}
			}

			// Faces
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				// The mesh is already triangulated
				auto& f = mesh->mFaces[i];
				faces.push_back(lastNumFaces + f.mIndices[0]);
				faces.push_back(lastNumFaces + f.mIndices[1]);
				faces.push_back(lastNumFaces + f.mIndices[2]);
			}

			lastNumFaces += mesh->mNumFaces;
		}

		Assimp::DefaultLogger::kill();
	}

	// --------------------------------------------------------------------------------

	// Setup vertex buffer
	meshVao = std::make_shared<fw::GLVertexArray>();

	meshPositionVbo = std::make_shared<fw::GLVertexBuffer>();
	meshPositionVbo->AddStatic((int)positions.size(), &positions[0]);
	meshVao->Add(GLDefaultVertexAttribute::Position, meshPositionVbo.get());
	
	meshNormalVbo = std::make_shared<fw::GLVertexBuffer>();
	meshNormalVbo->AddStatic((int)normals.size(), &normals[0]);
	meshVao->Add(GLDefaultVertexAttribute::Normal, meshNormalVbo.get());

	if (!texcoords.empty())
	{
		meshTexcoordVbo = std::make_shared<fw::GLVertexBuffer>();
		meshTexcoordVbo->AddStatic((int)texcoords.size(), &texcoords[0]);
		meshVao->Add(GLDefaultVertexAttribute::TexCoord0, meshTexcoordVbo.get());
	}
	
	meshIbo = std::make_shared<GLIndexBuffer>();
	meshIbo->AddStatic((int)faces.size(), &faces[0]);

	// --------------------------------------------------------------------------------

	// FBOs
	auto windowSize = window.getSize();
	primaryRt = std::make_shared<GLTexture2D>();
	primaryRt->SetMagFilter(GL_LINEAR);
	primaryRt->SetMinFilter(GL_LINEAR);
	primaryRt->SetWrap(GL_CLAMP_TO_EDGE);
	primaryRt->Allocate(windowSize.x, windowSize.y, GL_RGBA16F);
	primaryFbo = std::make_shared<GLFrameBuffer>(
		windowSize.x, windowSize.y,
		glm::vec4(glm::vec3(1.0f), 1.0f));
	primaryFbo->AddRenderTarget(primaryRt.get());

	horizontalBlurRt = std::make_shared<GLTexture2D>();
	horizontalBlurRt->SetMagFilter(GL_LINEAR);
	horizontalBlurRt->SetMinFilter(GL_LINEAR);
	horizontalBlurRt->SetWrap(GL_CLAMP_TO_EDGE);
	horizontalBlurRt->Allocate(windowSize.x / 2, windowSize.y / 2, GL_RGBA16F);
	horizontalBlurFbo = std::make_shared<GLFrameBuffer>(
		windowSize.x / 2, windowSize.y / 2,
		glm::vec4(glm::vec3(), 1.0f));
	horizontalBlurFbo->AddRenderTarget(horizontalBlurRt.get());

	return true;
}

void AchScene_2::Draw( sf::RenderWindow& window, double milli, fw::GLFrameBuffer& fbo )
{
	// Current row number
	double row = Util::MilliToRow(milli);

	// --------------------------------------------------------------------------------

	primaryFbo->Begin();
	
	// Render background
	{
		window.pushGLStates();
		window.draw(skySprite);
		window.popGLStates();
	}

	auto viewMatrix = glm::lookAt(
		glm::vec3(2.5f, 4.0f, 6.0f),
		glm::vec3(0.0f, 5.5f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	auto size = window.getSize();
	auto projectionMatrix = glm::perspective(
		60.0f,
		(float)size.x / size.y,
		0.1f,
		1000.0f);

	renderShader->Begin();
	renderShader->SetUniform("ViewMatrix", viewMatrix);
	renderShader->SetUniform("ProjectionMatrix", projectionMatrix);

	// Render poles
	{
		glm::mat4 modelMatrix = 
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(
					sync_get_val(track_X, row),
					0.0f,
					0.0f)) *
			glm::scale(
				glm::mat4(1.0f),
				glm::vec3(
					sync_get_val(track_Scale, row)));

		renderShader->SetUniform("ModelMatrix", modelMatrix);
		renderShader->SetUniform("Mode", 0);
		meshVao->Draw(GL_TRIANGLES, meshIbo.get());
	}

	// Render signs
	{
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const float zs[] = { 2.0f, 3.0f };
		float xs[2] =
		{
			sync_get_val(track_X2, row),
			sync_get_val(track_X3, row)
		};
		int texIndices[2] =
		{
			glm::clamp((int)sync_get_val(track_TexIndex2, row), 0, (int)signTextures.size()-1),
			glm::clamp((int)sync_get_val(track_TexIndex3, row), 0, (int)signTextures.size()-1)
		};
		for (int i = 0; i < 2; i++)
		{
			glm::mat4 modelMatrix = 
				glm::translate(
					glm::mat4(1.0f),
					glm::vec3(xs[i], 4.0f, zs[i])) *
				glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(0.0f, 1.0f, 0.0f)) *
				glm::scale(
					glm::mat4(1.0f),
					glm::vec3(sync_get_val(track_Scale2, row))) *
				glm::scale(
					glm::mat4(1.0f),
					glm::vec3(1.0f, 1.5f, 0.0f));
		
			renderShader->SetUniform("ModelMatrix", modelMatrix);
			renderShader->SetUniform("Mode", 1);
			renderShader->SetUniform("RT", 0);
		
			signTextures[texIndices[i]]->Bind(0);
			quadVao->Draw(GL_TRIANGLES, quadIbo.get());
			signTextures[texIndices[i]]->Unbind();
		}

		glPopAttrib();
	}

	renderShader->End();
	primaryFbo->End();

	// --------------------------------------------------------------------------------

	// Texel size
	auto sizeF = glm::vec2((float)size.x, (float)size.y);
	glm::vec2 texelSize = 1.0f / sizeF;

	// Blur factor
	//const float sigmaFactor = 0.7f;
	//const int kernelSize = 10;
	float sigmaFactor = sync_get_val(track_SigmaFactor, row);
	int kernelSize = sync_get_val(track_KernelSize, row);
	float blurStrength = sync_get_val(track_BlurStrength, row);
	//float sigmaFactor = 0.8f;
	//int kernelSize = 7.0f;
	//float blurStrength = 1.0f;

	// Horizontal blur
	horizontalBlurFbo->Begin();
	gaussianBlurShader->Begin();
	gaussianBlurShader->SetUniform("RT", 0);
	gaussianBlurShader->SetUniform("Orientation", 0);
	gaussianBlurShader->SetUniform("TexelSize", texelSize);
	gaussianBlurShader->SetUniform("SigmaFactor", sigmaFactor);
	gaussianBlurShader->SetUniform("KernelSize", kernelSize);
	gaussianBlurShader->SetUniform("BlurStrength", blurStrength);
	primaryRt->Bind();
	quadVao->Draw(GL_TRIANGLES, quadIbo.get());
	primaryRt->Unbind();
	gaussianBlurShader->End();
	horizontalBlurFbo->End();

	// Vertical blur
	fbo.Begin();
	gaussianBlurShader->Begin();
	gaussianBlurShader->SetUniform("RT", 0);
	gaussianBlurShader->SetUniform("Orientation", 1);
	gaussianBlurShader->SetUniform("TexelSize", texelSize);
	gaussianBlurShader->SetUniform("SigmaFactor", sigmaFactor);
	gaussianBlurShader->SetUniform("KernelSize", kernelSize);
	gaussianBlurShader->SetUniform("BlurStrength", blurStrength);
	horizontalBlurRt->Bind();
	quadVao->Draw(GL_TRIANGLES, quadIbo.get());
	horizontalBlurRt->Unbind();
	gaussianBlurShader->End();
	fbo.End();
}
