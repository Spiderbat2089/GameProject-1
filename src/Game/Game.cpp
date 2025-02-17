//
//	Created by MarcasRealAccount on 17. Oct. 2020
//

//	Press the screen to move camera, and press Escape to go out.
//	Currently the camera only has a freecam controller using wasd and space, shift.
//	The freecam moves in screen space so w moves forward from the camera's perspective,
//	space moves up from the camera's perspective and right moves right from the camera's perspective.

#include "Game.h"

#include "Engine/Application.h"
#include "Engine/Events/KeyboardEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Renderer/DebugRenderer.h"
#include "Engine/Utility/Locale/LocaleManager.h"

#include <cmath>

#define DEBUG_WORLD false
#if DEBUG_WORLD //todo(Izodn): Remove this, a game doesn't always have a world (E.g. main menu)
#include "Game/World/World.h"
#endif

Application* Application::CreateApplication()
{
	return new Game();
}

Game::Game()
    : Application(), m_Logger("Game")
{
	// Load the sources
#if false // TODO: Remove when audio library is cross platform
	TestWAV  = audio::AudioSource::LoadFromFile("Sounds/TestWAV.wav");
	TestMP3  = audio::AudioSource::LoadFromFile("Sounds/TestMP3.mp3");
	TestFLAC = audio::AudioSource::LoadFromFile("Sounds/TestFLAC.flac");

	input::InputGroup*         audioInput = input::InputHandler::GetOrCreateInputGroup("audioInput");
	input::ButtonInputBinding* playWav    = audioInput->CreateButtonInputBinding("playWav", input::buttons::key1);
	input::ButtonInputBinding* playMp3    = audioInput->CreateButtonInputBinding("playMp3", input::buttons::key2);
	input::ButtonInputBinding* playFlac   = audioInput->CreateButtonInputBinding("playFlac", input::buttons::key3);
	playWav->BindCallback(std::bind(&Game::PlayWAVCallback, this, std::placeholders::_1));
	playMp3->BindCallback(std::bind(&Game::PlayMP3Callback, this, std::placeholders::_1));
	playFlac->BindCallback(std::bind(&Game::PlayFLACCallback, this, std::placeholders::_1));
#endif

#if DEBUG_WORLD //todo(Izodn): Remove this, a game doesn't always have a world (E.g. main menu)
	// Custom world params
	int              seed              = 0;
	world::WorldType worldType         = world::WorldType::Standard;
	uint8_t          chunkLoadDiameter = 12;
	uint8_t          chunkDiameter     = 16;
	uint8_t          chunkHeight       = 255;
	uint8_t          oceanLevel        = 63;

	// Rolling hills
	uint8_t          minGenHeight      = 127;
	uint8_t          maxGenHeight      = 23;

	// Plins
	//uint8_t minGenHeight = oceanLevel + 10;
	//uint8_t maxGenHeight = oceanLevel - 10;

	world::World world(
	    seed,
	    worldType,
	    chunkLoadDiameter,
	    chunkDiameter,
	    chunkHeight,
	    oceanLevel,
	    minGenHeight,
	    maxGenHeight);

	// Default world (more MC like, more demanding (read near-impossible w/ debug renderer))
	//uint8_t      oceanLevel = world::World::DEFAULT_OCEAN_LEVEL;
	//world::World world;
#endif

	input::InputGroup*         inMenu    = input::InputHandler::GetOrCreateInputGroup("inMenu");
	input::ButtonInputBinding* closeMenu = inMenu->CreateButtonInputBinding("closeMenu", input::buttons::gamepadTriangle, input::ButtonInputType::PRESS, input::InputLocation::GAMEPAD);
	closeMenu->BindCallback(std::bind(&Game::CloseMenuCallback, this, std::placeholders::_1));
	m_Logger.LogDebug("CloseMenu keybind is: %u, on device: %u", closeMenu->GetIndex(), static_cast<uint32_t>(closeMenu->GetLocation()));

	// First get or create a new InputGroup with InputHandler::GetOrCreateInputGroup(name):
	input::InputGroup* onFoot = input::InputHandler::GetOrCreateInputGroup("onFoot");
	onFoot->SetCaptureMouse(true);
	// Second create either a ButtonInputBinding or AxisInputBinding with their respective create functions:
	input::ButtonInputBinding* openMenu = onFoot->CreateButtonInputBinding("openMenu", input::buttons::gamepadTriangle, input::ButtonInputType::PRESS, input::InputLocation::GAMEPAD);
	// Thirdly if you didn't set the binding in the create function you can do it like this:
	openMenu->BindCallback(std::bind(&Game::OpenMenuCallback, this, std::placeholders::_1));
	// The callback is void(gp1::input::ButtonCallbackData data) or void(gp1::input::AxisCallbackData data) depending on what type of binding.
	// For an example setup look at the callbacks in this class.
	m_Logger.LogDebug("OpenMenu keybind is: %u, on device: %u", openMenu->GetIndex(), static_cast<uint32_t>(openMenu->GetLocation()));

	input::AxisInputBinding* lookX = onFoot->CreateAxisInputBinding("lookX", input::axises::gamepadLeftTrigger, input::InputLocation::GAMEPAD);
	lookX->BindCallback(std::bind(&Game::LookCallback, this, std::placeholders::_1));
	m_Logger.LogDebug("LookX keybind is: %u, on device: %u", lookX->GetIndex(), static_cast<uint32_t>(lookX->GetLocation()));

	input::AxisInputBinding* lookY = onFoot->CreateAxisInputBinding("lookY", input::axises::gamepadRightTrigger, input::InputLocation::GAMEPAD);
	lookY->BindCallback(std::bind(&Game::LookCallback, this, std::placeholders::_1));
	m_Logger.LogDebug("LookY keybind is: %u, on device: %u", lookY->GetIndex(), static_cast<uint32_t>(lookY->GetLocation()));

	gp1::renderer::DebugRenderer::DebugDrawPoint({ 0.0f, 0.0f, -2.0f }, 9999999999.0f);
	gp1::renderer::DebugRenderer::DebugDrawSphere({ 0.0f, 0.0f, 2.0f }, 1.0f, 9999999999.0f);
	gp1::renderer::DebugRenderer::DebugDrawLine({ -2.0f, 1.0f, -4.0f }, { 2.0f, -1.0f, -2.0f }, 9999999999.0f);
	gp1::renderer::DebugRenderer::DebugDrawBox({ 0.0f, 0.0f, -3.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 45.0f, 0.0f }, 9999999999.0f);

#if DEBUG_WORLD //todo(Izodn): Remove this, a game doesn't always have a world (E.g. main menu)
	std::vector<world::Chunk*> chunks = world.GetChunks();
	for (world::Chunk* chunk : chunks)
	{
		std::vector<blocks::BlockType> blocks   = chunk->GetBlocks();
		uint8_t                        diameter = chunk->GetDiameter();
		uint8_t                        height   = chunk->GetHeight();
		world::vec2                    chunkPos = chunk->GetPosition();

		//todo(Izodn): Inefficient way of obtaining blocks & location. May fix later
		for (int x = 0; x < diameter; x++)
		{
			for (int z = 0; z < diameter; z++)
			{
				int rx = x + chunkPos.x * diameter;
				int rz = z + chunkPos.y * diameter;
				for (int y = height - 1; y >= 0; y--)
				{
					blocks::BlockType blockType    = chunk->GetBlock({ x, y, z });
					glm::fvec4        outlineColor = { 1, 1, 1, 1 };

					if (blockType == blocks::BlockType::Air || blockType == blocks::BlockType::Void)
					{
						continue; // Comment out to render void/air
						          // outlineColor = { 1.0f, 0.0f, 1.0f, 0.025f }; // Uncomment to render void/air
					}
					else if (blockType == blocks::BlockType::MoonRock)
					{
						outlineColor = { 1, 1, 1, 1 };
					}
					else if (blockType == blocks::BlockType::Ocean) // Flat doesn't generate ocean, so this won't get hit
					{
						outlineColor = { 0, 0, 255, 0.5 };
					}
					else
					{
						m_Logger.LogDebug("Shouldn't have hit this. If so, then a block type was unaccounted for.");
					}

					gp1::renderer::DebugRenderer::DebugDrawBox(
					    { rx, y - oceanLevel - 1, rz },
					    { 1.0f, 1.0f, 1.0f },
					    { 0.0f, 0.0f, 0.0f },
					    9999999999.0f,
					    outlineColor);
					break;
				}
			}
		}
	}
#endif
}

void Game::LookCallback(input::AxisCallbackData data)
{
	if (data.m_Id == "lookX")
	{
		m_Logger.LogDebug("Moved mouse X to %f", data.m_Value);
	}
	else if (data.m_Id == "lookY")
	{
		m_Logger.LogDebug("Moved mouse Y to %f", data.m_Value);
	}
}

void Game::OpenMenuCallback(input::ButtonCallbackData data)
{
	m_Logger.LogDebug("Opened Menu %u", (uint32_t) data.m_InputType);
	input::InputHandler::SetCurrentActiveInputGroup("inMenu");
}

void Game::CloseMenuCallback(input::ButtonCallbackData data)
{
	m_Logger.LogDebug("Closed Menu %u", (uint32_t) data.m_InputType);
	input::InputHandler::SetCurrentActiveInputGroup("onFoot");
}

#if false // TODO: Remove when audio library is cross platform

void Game::PlayMP3Callback([[maybe_unused]] input::ButtonCallbackData data)
{
	// Play the source
	audio::Audio::Play(TestMP3);
}

void Game::PlayWAVCallback([[maybe_unused]] input::ButtonCallbackData data)
{
	// Play the source
	audio::Audio::Play(TestWAV);
}

void Game::PlayFLACCallback([[maybe_unused]] input::ButtonCallbackData data)
{
	// Play the source
	audio::Audio::Play(TestFLAC);
}

#endif