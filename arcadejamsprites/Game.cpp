//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <iostream>
#include <sstream>

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const XMVECTORF32 START_POSITION = { -200.f, 0.f, 0.f, 0.f };
    const XMVECTORF32 ROOM_BOUNDS = { 18.f, 16.f, 12.f, 0.f };
    constexpr float ROTATION_GAIN = 0.004f;
    constexpr float MOVEMENT_GAIN = 4.f;
}

/* TODO:
Add more enemies (random spawn system)
Bound player to single screen
Add sound effects and music
Fix hitbox on ball
Add animations to octo, dog

*/

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_UNKNOWN, 
        2,
        D3D_FEATURE_LEVEL_12_0);
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    //   Add DX::DeviceResources::c_ReverseDepth to optimize depth buffer clears for 0 instead of 1.
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
    windowWidth = width;
    windowHeight = height;
    BOUNDS = Vector2(width, -2 * height);
    m_cameraPos = START_POSITION;

    // World creation
    World W();
    Dog D();
    NAME = GenerateName(1.f, 1.f);
}

#pragma region Frame Update
// execute the basic game loop
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

Vector3 Game::ProcessInput(const Keyboard::State& kb, const Mouse::State& mouse, Vector3& m_cameraPos, Dog& D) {

    if (kb.Home)
    {
        m_cameraPos = START_POSITION.v;
    }

    Vector3 move = Vector3::Zero;

    if (kb.Up || kb.W) {
        move.y += 1.f;
        D.Up();
    }

    if (kb.Down || kb.S) {
        move.y -= 1.f;
        D.Down();
    }

    if (kb.Left || kb.A) {
        move.x += 1.f;
        D.Left();
    }

    if (kb.Right || kb.D) {
        move.x -= 1.f;
        D.Right();
    }

    if (kb.PageUp || kb.Space)
        // TODO: jump
        // JUMPING = true;

        if (kb.PageDown || kb.X)
            move.z -= 1.f;

    return move;
}

void Game::UpdatePlay(DX::StepTimer const& timer, const float &elapsedTime, const float &totalTime, const Keyboard::State& kb, const Mouse::State& mouse) {

    if (!D.alive) {
        Mode = Score;
        all_scores.push_back({ SCORE, NAME });
        NAME = GenerateName(totalTime * 1.5f, totalTime);
    }

    // prepare for player movement
    Vector3 move = ProcessInput(kb, mouse, m_cameraPos, D);
    Quaternion q = Quaternion::CreateFromYawPitchRoll(0.f, 0.f, 0.f);
    move = Vector3::Transform(move, q);
    move *= MOVEMENT_GAIN;
    Vector3 newPos = m_cameraPos + move;

    // check for terrain and crab collisions
    if (!W.checkForCollisions(newPos)) {
        // move player in sync with camera
        m_cameraPos = newPos;
        D.pos = Vector2(newPos.x, newPos.y);
    }

    // fire projectile
    if (mouse.leftButton && W.projectiles.size() < 1) {
        Vector2 to = m_cameraPos - Vector2(m_cameraPos.x + mouse.x - windowWidth / 2, m_cameraPos.y + mouse.y - windowHeight / 2);
        W.createBall(m_cameraPos, to * 4.f);
    }

    // active projectile state
    if (W.projectiles.size() > 0) {
        Vector2 projPos = W.projectiles[0]->pos;
        W.octo->update(totalTime, projPos);

        for (auto& p : W.projectiles) {
            // dead ball expire
            if (p->velocity.Length() < 10.f) {
                W.deleteBall();
            }
            else if (p->pos.x < -500.f) {
                p->velocity.x = -1.5f * p->velocity.x;
                p->pos.x = -499.f;
            }
            else {
                p->update(elapsedTime);
                // boolean hit = W.checkForCollisions(p->pos)
            }
        }
    }

    // process crab and player updates
    for (Animal* entity : W.animals) {
        if (entity->alive) {
            entity->update();
            // smush crabs
            if (W.projectiles.size() > 0 && W.checkForCollision(W.projectiles[0]->pos, entity->pos)) {
                entity->smush();
                SCORE++;
                //W.newCrabs(2);
            }
            // damage player
            if (W.checkForCollision(D.pos, entity->pos)) {
                D.dmg(totalTime);
                D.velocity = entity->pos - D.pos;

            }
        }
    }

    // camera bump and player velocity wind down
    m_cameraPos -= Vector3(D.velocity.x, D.velocity.y, 0.f);
    D.pos = Vector2(m_cameraPos.x, m_cameraPos.y);
    D.velocity *= 0.4;

    if (m_cameraPos.y > BOUNDS.y - windowHeight) {
        W.generateChunkAt(-windowWidth * 1.55f, BOUNDS.y);
        BOUNDS.y += windowHeight;
    }
}

// Update the world
void Game::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());
    float totalTime = float(timer.GetTotalSeconds());

    elapsedTime;

    auto kb = m_keyboard->GetState();
    if (kb.Escape) {
        ExitGame();
    }

    auto mouse = m_mouse->GetState();
    
    // swap between game modes
    if (Mode == Play) {
        // tick main play mode
        UpdatePlay(timer, elapsedTime, totalTime, kb, mouse);
    } else if (Mode == Score) {
        // TODO: Move to input processor
        if (kb.Enter) {
            D.restart();
            Mode = Title;
        }
    } else if (Mode == Title) {
        World W();
        SCORE = 0;
        // TODO: Move to input processor
        if (kb.Enter) {
            Mode = Play;
            D.restart();
        }
    }

    PIXEndEvent();
}
#pragma endregion

void Game::RenderTitle() {
    const wchar_t* titletext = L"Rufus Goes To The Beach";
    const wchar_t* titlesub = L"Episode 1: Them Dog'gone Crabs";
    const wchar_t* instruction = L"PRESS ENTER and WASD + M1";
    Vector2 origin = { 0.f, 0.f };

    m_font->DrawString(m_spriteBatch.get(), titletext,
        Vector2(100.f, 100.f), Colors::White, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), titlesub,
        Vector2(100.f, 200.f), Colors::White, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), instruction,
        Vector2(windowWidth / 2, windowHeight / 2), Colors::White, 0.f, origin);
}

void Game::RenderScore() {
    const wchar_t* titletext = L"Leaderboard";
    Vector2 origin = { 0.f, 0.f };

    const wchar_t* instruction = L"PRESS ENTER and WASD + M1";
    m_font->DrawString(m_spriteBatch.get(), instruction,
        Vector2(windowWidth / 2, windowHeight / 2), Colors::White, 0.f, origin);

    m_font->DrawString(m_spriteBatch.get(), titletext,
        Vector2(100.f, 100.f), Colors::White, 0.f, origin);

    int row = 1;

        for (auto& pair : all_scores) {
        ++row;

        const std::wstring score_str = std::to_wstring(pair.first);
        Vector2 origin = { 0.f, 0.f };

        m_font->DrawString(m_spriteBatch.get(), score_str.c_str(),
            Vector2(100.f, 100.f * row), Colors::White, 0.f, origin);

        std::wstring name_str;
        StringToWString(name_str, pair.second);

        m_font->DrawString(m_spriteBatch.get(), name_str.c_str(),
            Vector2(400.f, 100.f * row), Colors::White, 0.f, origin);

    }
}

void Game::RenderUI() {
    // Name for scoreboard
    std::wstring wstmp;
    StringToWString(wstmp, NAME);
    Vector2 name_origin = m_font->MeasureString(wstmp.c_str()) / 2.f;
    m_font->DrawString(m_spriteBatch.get(), wstmp.c_str(),
        Vector2(windowWidth - 100.f, windowHeight - 125.f), Colors::White, 0.f, name_origin);

    // Score
    const wchar_t* output = L"Score:";
    const std::wstring score_str = std::to_wstring(SCORE);
    Vector2 origin = m_font->MeasureString(output) / 2.f;
    m_font->DrawString(m_spriteBatch.get(), score_str.c_str(),
        Vector2(100.f, windowHeight - 100.f), Colors::White, 0.f, origin);
}

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render"); 

    Vector3 offset = { static_cast<float>(windowWidth / 2), static_cast<float>(windowHeight / 2), 0.f };

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // begin drawing sprite batch
    m_spriteBatch->Begin(commandList);
    
    for (const auto &tile : W.tiles) {
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Sand),
            GetTextureSize(m_texture_sand.Get()),
           offset + m_cameraPos - tile->pos, &tile->rect, Colors::White, 0.f, Vector2(0, 0), 4.f);
    }

    for (const auto& proj : W.projectiles) {
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Ball),
            GetTextureSize(m_texture_ball.Get()), 
            offset + m_cameraPos - proj->pos, &proj->rect, Colors::White, 0.f, Vector2(0, 0), 1.f);
    }

    for (const auto& animal : W.animals) {
        if (animal->alive) {
            m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Crab),
                GetTextureSize(m_texture_crab.Get()),
                offset + m_cameraPos - animal->pos, &animal->rect, Colors::White, 0.f, Vector2(0, 0), 4.f);
        }
    }

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Octo),
        GetTextureSize(m_texture_octo.Get()),
        offset + m_cameraPos - W.octo->pos, &W.octo->rect, Colors::White, 0.f, Vector2(0, 0), 4.f);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture_cat.Get()),
        Vector2(windowWidth / 2.f, windowHeight / 2.f), &D.rect, Colors::White, 0.f, Vector2(0, 0), 4.f);

    // draw HP
    for (int i = 0; i < D.hp; ++i) {
        RECT hp_rect{ 0, 32, 32, 64 };
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
            GetTextureSize(m_texture_cat.Get()),
            Vector2(windowWidth - 40.f * i - 100.f, windowHeight - 100.f), &hp_rect, Colors::White, 0.f, Vector2(0, 0), 1.5f);
    }

    RenderUI();

    if (Mode == Score) {
        RenderScore();
    }

    if (Mode == Title) {
        RenderTitle();
    }

    // finish drawing
    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    // auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    // commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    windowWidth = width;
    windowHeight = height;
     
    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Check Shader Model 6 support
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"dog.png",
            m_texture_cat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"desert.png",
            m_texture_sand.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"ball.png",
            m_texture_ball.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"crab.png",
            m_texture_crab.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"octo.png",
            m_texture_octo.ReleaseAndGetAddressOf()));

    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        L"myfileb.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::MyFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::MyFont));

    CreateShaderResourceView(device, m_texture_cat.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Cat));

    CreateShaderResourceView(device, m_texture_sand.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Sand));

    CreateShaderResourceView(device, m_texture_ball.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Ball));

    CreateShaderResourceView(device, m_texture_crab.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Crab));

    CreateShaderResourceView(device, m_texture_octo.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Octo));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    SpriteBatchPipelineStateDescription pd(rtState,
        &CommonStates::NonPremultiplied);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    XMUINT2 catSize = GetTextureSize(m_texture_cat.Get());

    m_origin.x = float(catSize.x / 2);
    m_origin.y = float(catSize.y / 2);

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
    
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    auto size = m_deviceResources->GetOutputSize();
    m_screenPos.x = float(size.right) / 2.f;
    m_screenPos.y = float(size.bottom) / 2.f ;
    m_fontPos.x = float(size.right) / 2.f;
    m_fontPos.y = float(size.bottom) / 2.f;
}

void Game::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_texture_cat.Reset();
    m_texture_sand.Reset();
    m_resourceDescriptors.reset();
    m_spriteBatch.reset();
    m_font.reset();
    delete &W;
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

std::string Game::GenerateName(float input1, float input2) {
    std::string consonants = "bcdfghjklmnprstv";
    std::string vowels = "aeiouy";
    Rand_int c(0, consonants.length());
    Rand_int v(0, vowels.length());
    c.seed(std::time(nullptr));
    v.seed(std::time(nullptr));
    int v1 = v();
    int v2 = v();
    int c1 = c();
    int c2 = c();
    int c3 = c();
    int c4 = c();
    std::ostringstream mouth;
    mouth << static_cast<char>(std::toupper(consonants[c1])) << vowels[v1] << consonants[c3] << vowels[v2] << consonants[c4] << vowels[v2];
    return mouth.str();
}
#pragma endregion
