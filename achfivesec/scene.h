#pragma once
#ifndef ACHFIVESEC_SCENE_H
#define ACHFIVESEC_SCENE_H

#include <string>

namespace sf
{
	class RenderWindow;
}

namespace fw
{
	class GLFrameBuffer;
}

struct sync_device;

class Scene
{
public:

	Scene() {}
	virtual ~Scene() {}

public:

	virtual std::string Name() const = 0;
	virtual bool Setup(sf::RenderWindow& window, sync_device* rocket) = 0;
	virtual void Draw(sf::RenderWindow& window, double milli, fw::GLFrameBuffer& fbo) = 0;

};

#endif // ACHFIVESEC_SCENE_H