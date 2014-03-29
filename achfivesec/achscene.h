#pragma once
#ifndef ACHFIVESEC_ACH_SCENE_H
#define ACHFIVESEC_ACH_SCENE_H

#include "scene.h"

struct sync_track;

namespace fw
{
	class GLShader;
	class GLVertexArray;
	class GLVertexBuffer;
	class GLIndexBuffer;
	class GLTexture2D;
	class GLFrameBuffer;
}

class FontText;

class AchScene : public Scene
{
public:

	virtual std::string Name() const { return "AchScene"; }
	virtual bool Setup( sf::RenderWindow& window, sync_device* rocket );
	virtual void Draw( sf::RenderWindow& window, double milli, fw::GLFrameBuffer& fbo );

private:

	// Tracks
	const sync_track* track_WordScale;
	const sync_track* track_Angle;
	const sync_track* track_Pos;
	const sync_track* track_Focus;
	const sync_track* track_Range;
	const sync_track* track_Alpha;

private:

	std::shared_ptr<fw::GLShader> gaussianBlurShader;
	std::shared_ptr<fw::GLShader> dofCombineShader;

	std::shared_ptr<fw::GLShader> quadShader;
	std::shared_ptr<fw::GLShader> renderDepthShader;
	std::shared_ptr<fw::GLVertexArray> quadVao;
	std::shared_ptr<fw::GLVertexBuffer> quadPositionVbo;
	std::shared_ptr<fw::GLIndexBuffer> quadIbo;

	std::shared_ptr<fw::GLShader> textRenderShader;
	std::shared_ptr<FontText> text_Morning;
	std::shared_ptr<FontText> text_Arch;

	std::shared_ptr<fw::GLTexture2D> primaryRt;
	std::shared_ptr<fw::GLTexture2D> primaryDepthRt;
	std::shared_ptr<fw::GLFrameBuffer> primaryFbo;
	std::shared_ptr<fw::GLTexture2D> horizontalBlurRt;
	std::shared_ptr<fw::GLFrameBuffer> horizontalBlurFbo;
	std::shared_ptr<fw::GLTexture2D> verticalBlurRt;
	std::shared_ptr<fw::GLFrameBuffer> verticalBlurFbo;

};

#endif // ACHFIVESEC_ACH_SCENE_H