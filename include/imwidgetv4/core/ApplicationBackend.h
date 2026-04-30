#pragma once
#include <string>

namespace ImWidgetV4 {

// 前向声明
class ImApplication;

/**
 * @brief 应用程序后端接口
 *
 * 定义了应用程序后端需要实现的接口，用于抽象不同的窗口系统和渲染后端。
 * 例如：GLFW + OpenGL3、SDL + Vulkan、Win32 + DirectX 等。
 */
class ImApplicationBackend {
public:
    virtual ~ImApplicationBackend() = default;

    /**
     * @brief 初始化后端
     * @return 成功返回 true，失败返回 false
     */
    virtual bool Initialize() = 0;

    /**
     * @brief 关闭后端，释放资源
     */
    virtual void Shutdown() = 0;

    /**
     * @brief 运行主循环
     *
     * 这个方法会阻塞直到窗口关闭。
     * 在循环中会调用 Application 的 Tick 方法。
     */
    virtual void Run() = 0;

    /**
     * @brief 检查窗口是否应该关闭
     * @return 如果窗口应该关闭返回 true
     */
    virtual bool ShouldClose() const = 0;

    /**
     * @brief 设置窗口标题
     * @param title 新的窗口标题
     */
    virtual void SetWindowTitle(const std::string& title) = 0;

    /**
     * @brief 设置窗口大小
     * @param width 窗口宽度（像素）
     * @param height 窗口高度（像素）
     */
    virtual void SetWindowSize(int width, int height) = 0;

    /**
     * @brief 获取窗口大小
     * @param width 输出参数：窗口宽度（像素）
     * @param height 输出参数：窗口高度（像素）
     */
    virtual void GetWindowSize(int& width, int& height) const = 0;

    /**
     * @brief 开始新的一帧
     *
     * 在这个方法中应该：
     * 1. 处理窗口事件
     * 2. 开始 ImGui 新帧
     * 3. 清空渲染缓冲区
     */
    virtual void BeginFrame() = 0;

    /**
     * @brief 结束当前帧
     *
     * 在这个方法中应该：
     * 1. 渲染 ImGui 绘制数据
     * 2. 交换缓冲区
     */
    virtual void EndFrame() = 0;

    /**
     * @brief 设置关联的 Application 对象
     * @param app Application 对象指针
     */
    virtual void SetApplication(ImApplication* app) = 0;

    /**
     * @brief 获取关联的 Application 对象
     * @return Application 对象指针
     */
    virtual ImApplication* GetApplication() const = 0;

    /**
     * @brief 请求关闭窗口
     *
     * 设置窗口关闭标志，下一帧循环将退出。
     */
    virtual void RequestClose() = 0;

    /**
     * @brief 获取后端名称
     * @return 后端名称字符串（例如："GLFW + OpenGL3"）
     */
    virtual std::string GetBackendName() const = 0;
};

} // namespace ImWidgetV4
