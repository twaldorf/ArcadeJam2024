//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "World.h"
#include "Animals.h"
#include "Descriptors.h"


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

    struct Score {
        int score;
        std::string name;
    };

private:

    void Update(DX::StepTimer const& timer);
    void UpdatePlay(DX::StepTimer const& timer, const float& elapsedTime, const float& totalTime, const Keyboard::State& keyboard, const Mouse::State& mouse);
    Vector3 ProcessInput(const Keyboard::State& kb, const Mouse::State& mouse, Vector3& m_cameraPos, Dog&);
    void Render();
    void RenderTitle();
    void RenderScore();
    void RenderUI();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    std::string GenerateName(float f1, float f2);

    int StringToWString(std::wstring& ws, const std::string& s)
    {
        std::wstring wsTmp(s.begin(), s.end());

        ws = wsTmp;

        return 0;
    }

    enum ModeList {
        Title,
        Play,
        Score,
    };

    std::vector<std::pair<int, std::string>> all_scores;

    ModeList Mode = Title;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_cat;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_sand;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_ball;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_dog;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_octo;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_crab;

    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    DirectX::SimpleMath::Vector2 m_screenPos;
    DirectX::SimpleMath::Vector2 m_origin;

    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;
    DirectX::SimpleMath::Vector3 m_cameraPos;

    std::unique_ptr<DirectX::SpriteFont> m_font;
    DirectX::SimpleMath::Vector2 m_fontPos;

    World W;
    Dog D;

    int windowWidth = 0;
    int windowHeight = 0;
    Vector2 BOUNDS;

    int SCORE = 0;
    boolean INPUT = false;

    std::string NAME;
};
