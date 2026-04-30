#pragma once
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/core/Types.h>
#include <string>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace ImWidgetV4 {

/**
 * @brief Win32/DirectX 11 后端实现
 *
 * 提供基于 Win32 窗口系统和 DirectX 11 渲染 API 的应用程序后端。
 * 集成 ImGui 的 Win32 和 DirectX 11 后端实现。
 */
class ImWin32DX11Backend : public ImApplicationBackend {
public:
    /**
     * @brief 构造函数
     * @param windowTitle 窗口标题（宽字符）
     * @param width 窗口宽度（像素）
     * @param height 窗口高度（像素）
     */
    ImWin32DX11Backend(
        const std::wstring& windowTitle = L"ImWidgetV4 Application",
        int width = 1280,
        int height = 800);

    /**
     * @brief 析构函数
     */
    ~ImWin32DX11Backend() override;

    // ========== ImApplicationBackend 接口实现 ==========

    /**
     * @brief 初始化后端
     *
     * 创建窗口、初始化 DirectX 11 设备和 ImGui 后端。
     *
     * @return 成功返回 true，失败返回 false
     */
    bool Initialize() override;

    /**
     * @brief 关闭后端，释放资源
     *
     * 清理 ImGui、DirectX 11 资源和窗口。
     */
    void Shutdown() override;

    /**
     * @brief 运行主循环
     *
     * 阻塞执行，处理 Windows 消息循环，每帧调用 Application 的 AdvanceFrame。
     */
    void Run() override;

    /**
     * @brief 检查窗口是否应该关闭
     * @return 如果窗口应该关闭返回 true
     */
    bool ShouldClose() const override;

    /**
     * @brief 设置窗口标题
     * @param title 新的窗口标题（UTF-8）
     */
    void SetWindowTitle(const std::string& title) override;

    /**
     * @brief 设置窗口大小
     * @param width 窗口宽度（像素）
     * @param height 窗口高度（像素）
     */
    void SetWindowSize(int width, int height) override;

    /**
     * @brief 获取窗口大小
     * @param width 输出参数：窗口宽度（像素）
     * @param height 输出参数：窗口高度（像素）
     */
    void GetWindowSize(int& width, int& height) const override;

    /**
     * @brief 开始新的一帧
     *
     * 开始 ImGui 新帧，准备渲染。
     */
    void BeginFrame() override;

    /**
     * @brief 结束当前帧
     *
     * 渲染 ImGui 绘制数据，呈现到屏幕。
     */
    void EndFrame() override;

    /**
     * @brief 设置关联的 Application 对象
     * @param app Application 对象指针
     */
    void SetApplication(ImApplication* app) override;

    /**
     * @brief 获取关联的 Application 对象
     * @return Application 对象指针
     */
    ImApplication* GetApplication() const override;

    /**
     * @brief 请求关闭窗口
     */
    void RequestClose() override;

    /**
     * @brief 获取后端名称
     * @return 后端名称字符串
     */
    std::string GetBackendName() const override;

    // ========== DirectX 11 特定接口 ==========

    /**
     * @brief 获取 D3D11 设备
     * @return D3D11 设备指针
     */
    ID3D11Device* GetD3DDevice() const { return D3DDevice_; }

    /**
     * @brief 获取 D3D11 设备上下文
     * @return D3D11 设备上下文指针
     */
    ID3D11DeviceContext* GetD3DDeviceContext() const { return D3DDeviceContext_; }

    /**
     * @brief 获取窗口句柄
     * @return Win32 窗口句柄
     */
    HWND GetWindowHandle() const { return Hwnd_; }

private:
    // ========== Win32 窗口 ==========
    HINSTANCE HInstance_;
    HWND Hwnd_;
    std::wstring WindowTitle_;
    int WindowWidth_;
    int WindowHeight_;
    bool bShouldClose_;

    // ========== DirectX 11 资源 ==========
    ID3D11Device* D3DDevice_;
    ID3D11DeviceContext* D3DDeviceContext_;
    IDXGISwapChain* SwapChain_;
    ID3D11RenderTargetView* MainRenderTargetView_;

    // ========== 窗口大小调整 ==========
    UINT ResizeWidth_;
    UINT ResizeHeight_;
    bool bSwapChainOccluded_;

    // ========== Application 引用 ==========
    ImApplication* Application_;

    // ========== 内部方法 ==========

    /**
     * @brief 创建 DirectX 11 设备和交换链
     * @return 成功返回 true，失败返回 false
     */
    bool CreateDeviceD3D();

    /**
     * @brief 清理 DirectX 11 设备和交换链
     */
    void CleanupDeviceD3D();

    /**
     * @brief 创建渲染目标视图
     */
    void CreateRenderTarget();

    /**
     * @brief 清理渲染目标视图
     */
    void CleanupRenderTarget();

    /**
     * @brief 处理窗口大小调整
     */
    void HandleResize();

    /**
     * @brief 静态窗口过程（用于注册窗口类）
     */
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 实例窗口过程
     */
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace ImWidgetV4
