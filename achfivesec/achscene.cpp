#include "pch.h"
#include "achscene.h"
#include "util.h"
#include "logger.h"
#include "gl.h"
#include "shaderutil.h"
#include "font.h"
#include <sync/sync.h>
#include <freetype-gl/freetype-gl.h>

using namespace fw;

namespace
{

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

	const std::string QuadFs = 
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}

			in vec2 vTexCoord;
			out vec4 fragColor;
			uniform sampler2D RT;

			void main()
			{
				fragColor.rgb = texture(RT, vTexCoord).rgb;
				fragColor.a = 1;
			}
		
		);

	const std::string RenderDepthFs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}

			in vec2 vTexCoord;
			out vec4 fragColor;

			uniform sampler2D DepthRT;
			uniform float Near;
			uniform float Far;

			void main()
			{
				float d = texture(DepthRT, vTexCoord).r / (Far - Near);
				fragColor.rgb = vec3(d);
				fragColor.a = 1;
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

	const std::string DoFCombineShaderFs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}

			in vec2 vTexCoord;
			out vec4 fragColor;

			uniform float Range;
			uniform sampler2D PrimaryRT;
			uniform sampler2D DepthRT;
			uniform sampler2D BlurRT;
			uniform float Focus;
			uniform float Alpha;

			void main(void)
			{
				vec3 sharp = texture(PrimaryRT, vTexCoord).rgb;
				vec3 blur = texture(BlurRT, vTexCoord).rgb;
				float depth = texture(DepthRT, vTexCoord).r;
				fragColor.rgb = mix(sharp, blur, clamp(Range * abs(Focus - depth), 0.0, 1.0)).xyz;
				fragColor.a = Alpha;
			}
		
		);

	const std::string TextRenderShaderVs =
		FW_GL_SHADER_SOURCE(
			
			{{GLShaderVersion}}
			{{GLVertexAttributes}}

			layout (location = POSITION) in vec3 position;
			layout (location = 10) in vec2 offset;
			layout (location = TEXCOORD0) in vec2 texcoord0;
			layout (location = TEXCOORD1) in vec2 texcoord1;
			layout (location = COLOR) in vec3 color;

			out VertexAttribute
			{
				vec2 offset;
				vec2 texcoord0;
				vec2 texcoord1;
				vec3 color;
			} vertex;

			void main()
			{
				vertex.offset = offset;
				vertex.texcoord0 = texcoord0;
				vertex.texcoord1 = texcoord1;
				vertex.color = color;
				gl_Position = vec4(position, 1);
			}

		);

	const std::string TextRenderShaderGs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}
		
			layout (points) in;
			layout (triangle_strip, max_vertices = 4) out;

			in VertexAttribute
			{
				vec2 offset;
				vec2 texcoord0;
				vec2 texcoord1;
				vec3 color;
			} vertex[];

			out vec3 color;
			out vec2 texcoord;
			out vec3 viewvec;

			uniform mat4 ModelMatrix;
			uniform mat4 ViewMatrix;
			uniform mat4 ProjectionMatrix;
			uniform vec2 WordScale;
			uniform float DistanceScale;

			void main()
			{
				mat4 mvMatrix = ViewMatrix * ModelMatrix;
				mat4 mvpMatrix = ProjectionMatrix * mvMatrix;

				vec4 center = gl_in[0].gl_Position;
				vec2 offset = vertex[0].offset * WordScale;

				float s0 = vertex[0].texcoord0.s;
				float t0 = vertex[0].texcoord0.t;
				float s1 = vertex[0].texcoord1.s;
				float t1 = vertex[0].texcoord1.t;

				vec4 p;

				p = center;
				p.x -= offset.x;
				p.y -= offset.y;
				gl_Position = mvpMatrix * p;
				viewvec = (mvMatrix * p).xyz * DistanceScale;
				color = vertex[0].color;
				texcoord = vec2(s0, t1);
				EmitVertex();

				p = center;
				p.x += offset.x;
				p.y -= offset.y;
				gl_Position = mvpMatrix * p;
				viewvec = (mvMatrix * p).xyz * DistanceScale;
				color = vertex[0].color;
				texcoord = vec2(s1, t1);
				EmitVertex();

				p = center;
				p.x -= offset.x;
				p.y += offset.y;
				gl_Position = mvpMatrix * p;
				viewvec = (mvMatrix * p).xyz * DistanceScale;
				color = vertex[0].color;
				texcoord = vec2(s0, t0);
				EmitVertex();

				p = center;
				p.x += offset.x;
				p.y += offset.y;
				gl_Position = mvpMatrix * p;
				viewvec = (mvMatrix * p).xyz * DistanceScale;
				color = vertex[0].color;
				texcoord = vec2(s1, t0);
				EmitVertex();

				EndPrimitive();
			}

		);

	const std::string TextRenderShaderFs =
		FW_GL_SHADER_SOURCE(
		
			{{GLShaderVersion}}

			in vec2 texcoord;
			in vec3 color;
			in vec3 viewvec;

			out vec4 fragColor;
			out vec4 depth;

			uniform sampler2D Tex;
			uniform float Alpha;
			uniform bool Mode;

			void main()
			{
				// #Tex is the distance map
				float dist  = texture(Tex, texcoord).r;
				float width = fwidth(dist);
				float alpha = smoothstep(0.5-width, 0.5+width, dist);

				//float alpha = texture(Tex, texcoord).r;

				fragColor.rgb = Mode ? color : vec3(0);
				fragColor.a = alpha * Alpha;
				depth.r = length(viewvec);
				depth.a = 1;

				//fragColor.rgb = vec3(1);
				//fragColor.a = 1;
			}
		
		);

}

bool AchScene::Setup( sf::RenderWindow& window, sync_device* rocket )
{
	// Tracks
	track_WordScale = sync_get_track(rocket, "achscene.WordScale");
	track_Angle = sync_get_track(rocket, "achscene.Angle");
	track_Pos = sync_get_track(rocket, "achscene.Pos");
	track_Focus = sync_get_track(rocket, "achscene.Focus");
	track_Range = sync_get_track(rocket, "achscene.Range");
	track_Alpha = sync_get_track(rocket, "achscene.Alpha");

	// --------------------------------------------------------------------------------

	// Shaders
	ShaderUtil::ShaderTemplateDict dict;

	FW_LOG_INFO("Loading quadShader");
	quadShader = std::make_shared<GLShader>();
	quadShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
	quadShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(QuadFs, dict));
	quadShader->Link();

	FW_LOG_INFO("Loading renderDepthShader");
	renderDepthShader = std::make_shared<GLShader>();
	renderDepthShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
	renderDepthShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(RenderDepthFs, dict));
	renderDepthShader->Link();

	FW_LOG_INFO("Loading gaussianBlurShader");
	gaussianBlurShader = std::make_shared<GLShader>();
	gaussianBlurShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
	gaussianBlurShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(GaussianBlurFs, dict));
	gaussianBlurShader->Link();

	FW_LOG_INFO("Loading dofCombineShader");
	dofCombineShader = std::make_shared<GLShader>();
	dofCombineShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
	dofCombineShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(DoFCombineShaderFs, dict));
	dofCombineShader->Link();

	FW_LOG_INFO("Loading renderShader");
	textRenderShader = std::make_shared<GLShader>();
	textRenderShader->CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(TextRenderShaderVs, dict));
	textRenderShader->CompileString(GLShaderType::GeometryShader, ShaderUtil::GenerateShaderString(TextRenderShaderGs, dict));
	textRenderShader->CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(TextRenderShaderFs, dict));
	textRenderShader->Link();

	// --------------------------------------------------------------------------------

	// Quad
	quadVao = std::make_shared<GLVertexArray>();
	quadPositionVbo = std::make_shared<GLVertexBuffer>();
	quadIbo = std::make_shared<GLIndexBuffer>();

	const glm::vec3 quadPositions[] =
	{
		glm::vec3( 1.0f,  1.0f, 0.0f),
		glm::vec3(-1.0f,  1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3( 1.0f, -1.0f, 0.0f)
	};

	const GLuint quadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	quadPositionVbo->AddStatic(12, &quadPositions[0].x);
	quadVao->Add(GLDefaultVertexAttribute::Position, quadPositionVbo.get());
	quadIbo->AddStatic(6, quadIndices);

	// --------------------------------------------------------------------------------

	// Font
	const std::string font = "OpenSans-Semibold.ttf";
	const float kerningOffset = -5.0f;
	const glm::vec2 basePos(-400.0f, -200.0f);
	FormattedString str;

	text_Morning = std::make_shared<FontText>();
	str.text = L"Morning";
	str.colors.emplace_back(0.9f, 0.9f, 0.0f);	// Yellow
	str.colors.emplace_back(0.9f, 0.0f, 0.0f);	// Red
	str.colors.emplace_back(0.0f, 0.9f, 0.9f);	// Aqua
	str.colors.emplace_back(0.0f, 0.5f, 0.9f);	// Dark aqua
	str.colors.emplace_back(0.9f, 0.0f, 0.0f);	// Red
	str.colors.emplace_back(0.5f, 0.9f, 0.0f);	// Yellowish green
	str.colors.emplace_back(0.9f, 0.6f, 0.0f);	// Orange
	if (!text_Morning->Load(font, str, basePos + glm::vec2(0.0f, 200.0f), 150.0f, kerningOffset))
	{
		return false;
	}

	text_Arch = std::make_shared<FontText>();
	str.text = L"Arch";
	str.colors.clear();
	str.colors.emplace_back(0.0f, 0.9f, 0.0f);	// Green
	str.colors.emplace_back(0.5f, 0.9f, 0.5f);	// Purple
	str.colors.emplace_back(0.9f, 0.9f, 0.0f);	// Yellow
	str.colors.emplace_back(0.7f, 0.1f, 0.5f);
	if (!text_Arch->Load(font, str, basePos + glm::vec2(230.0f, 70.0f), 150.0f, kerningOffset))
	{
		return false;
	}

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

	primaryDepthRt = std::make_shared<GLTexture2D>();
	primaryDepthRt->SetMagFilter(GL_LINEAR);
	primaryDepthRt->SetMinFilter(GL_LINEAR);
	primaryDepthRt->SetWrap(GL_CLAMP_TO_EDGE);
	primaryDepthRt->Allocate(windowSize.x, windowSize.y, GL_RGBA16F);
	//primaryDepthRt->Allocate(windowSize.x, windowSize.y, GL_R16F);
	primaryFbo->AddRenderTarget(primaryDepthRt.get());

	horizontalBlurRt = std::make_shared<GLTexture2D>();
	horizontalBlurRt->SetMagFilter(GL_LINEAR);
	horizontalBlurRt->SetMinFilter(GL_LINEAR);
	horizontalBlurRt->SetWrap(GL_CLAMP_TO_EDGE);
	horizontalBlurRt->Allocate(windowSize.x / 2, windowSize.y / 2, GL_RGBA16F);
	horizontalBlurFbo = std::make_shared<GLFrameBuffer>(
		windowSize.x / 2, windowSize.y / 2,
		glm::vec4(glm::vec3(), 1.0f));
	horizontalBlurFbo->AddRenderTarget(horizontalBlurRt.get());

	verticalBlurRt = std::make_shared<GLTexture2D>();
	verticalBlurRt->SetMagFilter(GL_LINEAR);
	verticalBlurRt->SetMinFilter(GL_LINEAR);
	verticalBlurRt->SetWrap(GL_CLAMP_TO_EDGE);
	verticalBlurRt->Allocate(windowSize.x / 2, windowSize.y / 2, GL_RGBA16F);
	verticalBlurFbo = std::make_shared<GLFrameBuffer>(
		windowSize.x / 2, windowSize.y / 2,
		glm::vec4(glm::vec3(), 1.0f));
	verticalBlurFbo->AddRenderTarget(verticalBlurRt.get());

	return true;
}

void AchScene::Draw( sf::RenderWindow& window, double milli, fw::GLFrameBuffer& fbo )
{
	// Current row number
	double row = Util::MilliToRow(milli);

	// --------------------------------------------------------------------------------

#if 1

	float pos = sync_get_val(track_Pos, row);
	float angle = sync_get_val(track_Angle, row);
	auto modelMatrix = 
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), -40.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));

	auto viewMatrix = glm::lookAt(
		glm::vec3(0.0f, 1.0f, 3.0f),
		glm::vec3(),
		glm::vec3(0.0f, 1.0f, 0.0f));

	//auto viewMatrix = glm::mat4(1.0f);

	const float zNear = 0.1f;
	const float zFar = 10.0f;
	auto size = window.getSize();
	auto projectionMatrix = glm::perspective(
		70.0f,
		(float)size.x / size.y,
		zNear,
		zFar);
	//auto projectionMatrix = glm::ortho(0.0f, (float)size.x, 0.0f, (float)size.y);

	primaryFbo->Begin();
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	textRenderShader->Begin();
	textRenderShader->SetUniform("ModelMatrix", modelMatrix);
	textRenderShader->SetUniform("ViewMatrix", viewMatrix);
	textRenderShader->SetUniform("ProjectionMatrix", projectionMatrix);
	textRenderShader->SetUniform("Tex", 0);
	textRenderShader->SetUniform("DistanceScale", 1.0f);

	float wordScale = sync_get_val(track_WordScale, row);
	const float baseXScale = 1.1f;
	textRenderShader->SetUniform("WordScale", glm::vec2(baseXScale, 1.0f));
	textRenderShader->SetUniform("Alpha", 1.0f);
	textRenderShader->SetUniform("Mode", false);
	text_Morning->Draw();
	text_Arch->Draw();
	
	textRenderShader->SetUniform("WordScale", glm::vec2(baseXScale * wordScale, wordScale));
	textRenderShader->SetUniform("Alpha", 0.2f);
	textRenderShader->SetUniform("Mode", true);
	text_Morning->Draw();
	text_Arch->Draw();

	textRenderShader->End();
	
	glPopAttrib();
	glPopAttrib();
	primaryFbo->End();

#endif

	// --------------------------------------------------------------------------------

	// Texel size
	auto sizeF = glm::vec2((float)size.x, (float)size.y);
	glm::vec2 texelSize = 1.0f / sizeF;

	// Blur factor
	//const float sigmaFactor = 0.7f;
	//const int kernelSize = 10;
	//float sigmaFactor = sync_get_val(track_SigmaFactor, row);
	//int kernelSize = sync_get_val(track_KernelSize, row);
	//float blurStrength = sync_get_val(track_BlurStrength, row);
	float sigmaFactor = 0.8f;
	int kernelSize = 7.0f;
	float blurStrength = 1.0f;

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
	verticalBlurFbo->Begin();
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
	verticalBlurFbo->End();

	// Combine
	fbo.Begin();
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	dofCombineShader->Begin();
	dofCombineShader->SetUniform("PrimaryRT", 0);
	dofCombineShader->SetUniform("DepthRT", 1);
	dofCombineShader->SetUniform("BlurRT", 2);
	dofCombineShader->SetUniform("Range", sync_get_val(track_Range, row));
	dofCombineShader->SetUniform("Focus", sync_get_val(track_Focus, row));
	dofCombineShader->SetUniform("Alpha", sync_get_val(track_Alpha, row));
	primaryRt->Bind(0);
	primaryDepthRt->Bind(1);
	verticalBlurRt->Bind(2);
	quadVao->Draw(GL_TRIANGLES, quadIbo.get());
	verticalBlurRt->Unbind();
	primaryDepthRt->Unbind();
	primaryRt->Unbind();
	dofCombineShader->End();
	glPopAttrib();
	fbo.End();

	// --------------------------------------------------------------------------------

#if 0
	quadShader->Begin();
	quadShader->SetUniform("RT", 0);
	primaryRt->Bind();
	quadVao->Draw(GL_TRIANGLES, quadIbo.get());
	primaryRt->Unbind();
	quadShader->End();
#endif

#if 0
	renderDepthShader->Begin();
	renderDepthShader->SetUniform("DepthRT", 0);
	renderDepthShader->SetUniform("Near", zNear);
	renderDepthShader->SetUniform("Far", zFar);
	primaryDepthRt->Bind();
	quadVao->Draw(GL_TRIANGLES, quadIbo.get());
	primaryDepthRt->Unbind();
	renderDepthShader->End();
#endif
}
