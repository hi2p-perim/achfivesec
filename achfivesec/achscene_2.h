#pragma once
#ifndef ACHFIVESEC_ACH_SCENE_2_H
#define ACHFIVESEC_ACH_SCENE_2_H

#include "scene.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

struct sync_track;

namespace fw
{
	class GLShader;
	class GLVertexArray;
	class GLVertexBuffer;
	class GLIndexBuffer;
	class GLTexture2D;
}

class AchScene_2 : public Scene
{
public:

	virtual std::string Name() const { return "AchScene_2"; }
	virtual bool Setup( sf::RenderWindow& window, sync_device* rocket );
	virtual void Draw( sf::RenderWindow& window, double milli, fw::GLFrameBuffer& fbo );

private:

	const sync_track* track_Scale;
	const sync_track* track_X;
	
	const sync_track* track_Scale2;
	const sync_track* track_X2;
	const sync_track* track_X3;
	const sync_track* track_TexIndex2;
	const sync_track* track_TexIndex3;

	const sync_track* track_KernelSize;
	const sync_track* track_SigmaFactor;
	const sync_track* track_BlurStrength;

private:

	sf::Texture skyTexture;
	sf::Sprite skySprite;

private:

	std::shared_ptr<fw::GLShader> renderShader;
	std::shared_ptr<fw::GLShader> gaussianBlurShader;

	std::shared_ptr<fw::GLVertexArray> meshVao;
	std::shared_ptr<fw::GLVertexBuffer> meshPositionVbo;
	std::shared_ptr<fw::GLVertexBuffer> meshNormalVbo;
	std::shared_ptr<fw::GLVertexBuffer> meshTexcoordVbo;
	std::shared_ptr<fw::GLIndexBuffer> meshIbo;

	std::shared_ptr<fw::GLVertexArray> quadVao;
	std::shared_ptr<fw::GLVertexBuffer> quadPositionVbo;
	std::shared_ptr<fw::GLVertexBuffer> quadTexcoordVbo;
	std::shared_ptr<fw::GLIndexBuffer> quadIbo;

	std::vector<std::shared_ptr<fw::GLTexture2D>> signTextures;

	std::shared_ptr<fw::GLTexture2D> primaryRt;
	std::shared_ptr<fw::GLFrameBuffer> primaryFbo;
	std::shared_ptr<fw::GLTexture2D> horizontalBlurRt;
	std::shared_ptr<fw::GLFrameBuffer> horizontalBlurFbo;

};

#endif // ACHFIVESEC_ACH_SCENE_2_H