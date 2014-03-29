#include "pch.h"
#include "gl.h"
#include "logger.h"
#include "util.h"
#include "shaderutil.h"
#include "achscene.h"
#include "achscene_2.h"
#include <boost/program_options.hpp>
#include <SFML/Audio.hpp>
#include <sync/sync.h>

using namespace fw;
namespace po = boost::program_options;

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

			uniform sampler2D RT1;
			uniform sampler2D RT2;
			uniform float Alpha;
			uniform float Blend;

			void main()
			{
				vec3 c1 = texture(RT1, vTexCoord).rgb;
				vec3 c2 = texture(RT2, vTexCoord).rgb;
				fragColor.rgb = mix(c1, c2, Blend);
				fragColor.a = Alpha;
			}
		
		);

}

class Application
{
public:

	Application()
		: paused(false)
	{

	}

public:

	bool ParseArguments( int argc, char** argv )
	{
		// Define options
		po::options_description opt("Allowed options");
		opt.add_options()
			("help", "Display help message")
			("log,l", po::value<std::string>(&logFilePath)->default_value(""), "Output image path");

		po::variables_map vm;

		try
		{
			// Parse options
			po::store(po::command_line_parser(argc, argv).options(opt).run(), vm);

			if (vm.count("help"))
			{
				PrintHelpMessage(opt);
				return false;
			}

			po::notify(vm);
		}
		catch (po::required_option& e)
		{
			// Some options are missing
			std::cout << "ERROR : " << e.what() << std::endl;
			PrintHelpMessage(opt);
			return false;
		}
		catch (po::error& e)
		{
			// Error on parsing options
			std::cout << "ERROR : " << e.what() << std::endl;
			PrintHelpMessage(opt);
			return false;
		}

		return true;
	}

	bool Run()
	{
		// Create window and OpenGL context
		sf::ContextSettings settings;
		settings.majorVersion = 4;
		settings.minorVersion = 2;
		settings.antialiasingLevel = 8;
		sf::RenderWindow window(sf::VideoMode(1280, 720), "achfivesec", sf::Style::Titlebar, settings);

		// Initialize GLEW
		if (!GLUtils::InitializeGlew())
		{
			FW_LOG_ERROR("Failed to initialize GLEW");
			return false;
		}

		// Enable error handling
		GLUtils::EnableDebugOutput(GLUtils::DebugOutputFrequencyHigh);

		// --------------------------------------------------------------------------------

		// Setup GNU rocket
		sync_device* rocket = sync_create_device("sync");
		if (!rocket)
		{
			std::cerr << "Failed to initialize GNU rocket" << std::endl;
			return 1;
		}

#ifndef SYNC_PLAYER
		struct sync_cb syncCallback =
		{
			pause,
			set_row,
			is_playing
		};

		sync_set_callbacks(rocket, &syncCallback, (void*)this);
		if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
		{
			std::cerr << "failed to connect to host" << std::endl;
			return false;
		}
#endif

		// Some tracks
		//const sync_track* track_ActiveSceneIndex_1;
		//const sync_track* track_ActiveSceneIndex_2;
		const sync_track* track_Blend;
		const sync_track* track_Alpha;
		track_Blend = sync_get_track(rocket, "global.Blend");
		track_Alpha = sync_get_track(rocket, "global.Alpha");

		// --------------------------------------------------------------------------------

		// Setup scene
		std::vector<std::unique_ptr<Scene>> scenes;
		scenes.emplace_back(new AchScene);
		scenes.emplace_back(new AchScene_2);
		for (auto& scene : scenes)
		{
			if (!scene->Setup(window, rocket))
			{
				std::cerr << "Failed to setup " << scene->Name() << std::endl;
				return false;
			}
		}

		// Some GL resources
		auto windowSize = window.getSize();
		
		GLFrameBuffer scene1Fbo(windowSize.x, windowSize.y, glm::vec4(glm::vec3(1.0f), 1.0f));
		GLTexture2D scene1Rt;
		scene1Rt.SetMagFilter(GL_LINEAR);
		scene1Rt.SetMinFilter(GL_LINEAR);
		scene1Rt.SetWrap(GL_CLAMP_TO_EDGE);
		scene1Rt.Allocate(windowSize.x, windowSize.y, GL_RGBA16F);
		scene1Fbo.AddRenderTarget(&scene1Rt);

		GLFrameBuffer scene2Fbo(windowSize.x, windowSize.y, glm::vec4(glm::vec3(1.0f), 1.0f));
		GLTexture2D scene2Rt;
		scene2Rt.SetMagFilter(GL_LINEAR);
		scene2Rt.SetMinFilter(GL_LINEAR);
		scene2Rt.SetWrap(GL_CLAMP_TO_EDGE);
		scene2Rt.Allocate(windowSize.x, windowSize.y, GL_RGBA16F);
		scene2Fbo.AddRenderTarget(&scene2Rt);

		ShaderUtil::ShaderTemplateDict dict;
		GLShader quadShader;
		quadShader.CompileString(GLShaderType::VertexShader, ShaderUtil::GenerateShaderString(QuadVs, dict));
		quadShader.CompileString(GLShaderType::FragmentShader, ShaderUtil::GenerateShaderString(QuadFs, dict));
		quadShader.Link();

		GLVertexArray quadVao;
		GLVertexBuffer quadPositionVbo;
		GLIndexBuffer quadIbo;

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

		quadPositionVbo.AddStatic(12, &quadPositions[0].x);
		quadVao.Add(GLDefaultVertexAttribute::Position, &quadPositionVbo);
		quadIbo.AddStatic(6, quadIndices);

		// --------------------------------------------------------------------------------

		// Load music
		if (!buffer.loadFromFile("achop.wav"))
		{
			std::cerr << "Failed to load music" << std::endl;
			return false;
		}

		sound.setBuffer(buffer);
		sound.play();

		// --------------------------------------------------------------------------------

		while (window.isOpen())
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				switch (event.type)
				{
					case sf::Event::Closed:
					{
						window.close();
						break;
					}

					case sf::Event::KeyPressed:
					{
						if (event.text.unicode == sf::Keyboard::Escape)
						{
							window.close();
						}

						break;
					}
				}
			}

			double time = sound.getPlayingOffset().asMilliseconds();
			double row = Util::MilliToRow(time);
#ifndef SYNC_PLAYER
			if (sync_update(rocket, (int)std::floor(row)))
			{
				sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
			}
#endif

			if ((int)std::floor(Util::MilliToBeats(time)) > 18)
			{
				sound.setPlayingOffset(sf::Time::Zero);
			}

			// Active scenes
			//int activeSceneIndex_1 = (int)sync_get_val(track_ActiveSceneIndex_1, row);
			//int activeSceneIndex_2 = (int)sync_get_val(track_ActiveSceneIndex_2, row);
			int activeSceneIndex_1 = 0;
			int activeSceneIndex_2 = 1;
			Scene* activeScene_1 = activeSceneIndex_1 >= 0
				? scenes[activeSceneIndex_1].get()
				: nullptr;
			Scene* activeScene_2 = activeSceneIndex_2 >= 0
				? scenes[activeSceneIndex_2].get()
				: nullptr;

			// Clear buffers
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			const glm::vec4 clearColor(1.0f);
			glClearBufferfv(GL_COLOR, 0, glm::value_ptr(clearColor));

			// Draw scenes
			glEnable(GL_DEPTH_TEST);
			float blend = sync_get_val(track_Blend, row);
			if (activeScene_1 && blend < 1.0f)
			{
				activeScene_1->Draw(window, time, scene1Fbo);
			}
			if (activeScene_2 && blend > 0.0f)
			{
				activeScene_2->Draw(window, time, scene2Fbo);
			}

			// Draw mixed scene
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			quadShader.Begin();
			quadShader.SetUniform("RT1", 0);
			quadShader.SetUniform("RT2", 1);
			quadShader.SetUniform("Blend", blend);
			quadShader.SetUniform("Alpha", sync_get_val(track_Alpha, row));
			scene1Rt.Bind(0);
			scene2Rt.Bind(1);
			quadVao.Draw(GL_TRIANGLES, &quadIbo);
			scene2Rt.Unbind();
			scene1Rt.Unbind();
			quadShader.End();
			glPopAttrib();

			window.display();
		}

		return true;
	}

	void StartLogging()
	{
		// Configure the logger
		if (!logFilePath.empty())
		{
			Logger::SetOutputMode(Logger::LogOutputMode::Stdout | Logger::LogOutputMode::File);
			Logger::SetOutputFileName(logFilePath);
		}
		else
		{
			Logger::SetOutputMode(Logger::LogOutputMode::Stdout);
		}

		// Start the logger thread
		logResult = std::async(std::launch::async, [this]()
		{
			// Event loop for logger process
			while (!logThreadDone || !Logger::Empty())
			{
				// Process log output
				if (!Logger::Empty())
				{
					Logger::ProcessOutput();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
	}

	void FinishLogging()
	{
		// Wait for the finish of the logger thread
		logThreadDone = true;
		logResult.wait();
	}

private:

	void PrintHelpMessage(const po::options_description& opt)
	{
		std::cout << "Usage: achfivesec [arguments]" << std::endl;
		std::cout << std::endl;
		std::cout << opt << std::endl;
	}

public:

#ifndef SYNC_PLAYER

	static void pause(void* d, int flag)
	{
		auto* app = static_cast<Application*>(d);
		if (flag)
		{
			app->paused = true;
			app->sound.pause();
		}
		else
		{
			app->paused = false;
			app->sound.play();
		}
	}

	static void set_row(void* d, int row)
	{
		auto* app = static_cast<Application*>(d);
		double offset = Util::RowToMilli(row);
		app->sound.setPlayingOffset(sf::milliseconds((int)offset));
	}

	static int is_playing(void* d)
	{
		auto* app = static_cast<Application*>(d);
		return app->sound.getStatus() == sf::Sound::Playing;
	}

#endif

private:

	bool paused;
	sf::SoundBuffer buffer;
	sf::Sound sound;

	// Log file
	std::string logFilePath;

	// Logging thread related variables
	std::atomic<bool> logThreadDone;
	std::future<void> logResult;

};

int main(int argc, char** argv)
{
	int result = EXIT_SUCCESS;
	Application app;

	if (app.ParseArguments(argc, argv))
	{
		app.StartLogging();
		FW_LOG_INFO("Started");

		try
		{
			if (!app.Run())
			{
				result = EXIT_FAILURE;
			}
		}
		catch (const std::exception& e)
		{
			FW_LOG_ERROR(boost::str(boost::format("EXCEPTION | %s") % e.what()));
			result = EXIT_FAILURE;
		}

		FW_LOG_INFO("Finished");
		app.FinishLogging();
	}

//#ifdef FW_DEBUG_MODE
//	std::cerr << "Press any key to exit ...";
//	std::cin.get();
//#endif

	return result;
}