#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective;
extern std::vector<double> zbuffer;

// ============================================================================
// 全局状态机与配置
// ============================================================================

enum class RenderMode {
    WIREFRAME = 0,
    FLAT_SHADING = 1,
    PHONG_SHADING = 2,
    TEXTURE_MAPPING = 3
};

struct AppState {
    int screen_width = 800;
    int screen_height = 800;
    
    // 相机参数
    vec3 eye = { -1.0, 0.0, 2.0 };
    vec3 center = { 0.0, 0.0, 0.0 };
    vec3 up = { 0.0, 1.0, 0.0 };
    vec3 light = { 1.0, 1.0, 1.0 };

    // 物体旋转角度
    double obj_rot_y = 0.0;
    double obj_rot_x = 0.0;
    
    // 模型与渲染
    std::unique_ptr<Model> model = nullptr;
    std::vector<std::string> model_list = {
       "obj/african_head/african_head.obj",
       "obj/boggie/body.obj",
       "obj/diablo3_pose/diablo3_pose.obj"
    };
    int current_model_idx = 0;
    std::string current_model_path = "obj/african_head/african_head.obj";
    RenderMode current_mode = RenderMode::PHONG_SHADING;
    
    // 交互状态
    bool should_exit = false;
    bool need_redraw = true;
    
    // 鼠标状态
    int last_mouse_x = 0;
    int last_mouse_y = 0;
    bool mouse_dragging = false;
};

// ============================================================================
// Shader 实现
// ============================================================================

struct WireframeShader : IShader
{
    const Model& model;
    
    WireframeShader(const Model& m) : model(m) {}
    
    virtual vec4 vertex(const int face, const int vert)
    {
        vec4 gl_Position = ModelView * model.vert(face, vert);
        return Perspective * gl_Position;
    }
    
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const 
    {
        return { false, TGAColor{200, 200, 200, 255} };
    }
};

struct FlatShader : IShader {
    const Model& model;
    vec3 tri[3];  // triangle in eye coordinates
    vec4 l;
    
    FlatShader(const vec3 light, const Model& m) : model(m) 
    {
        l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0.0 }));
    }
    
    virtual vec4 vertex(const int face, const int vert)
    {
        vec4 gl_Position = ModelView * model.vert(face, vert);
        tri[vert] = gl_Position.xyz();
        return Perspective * gl_Position;
    }
    
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const 
    {
        // Flat Shading：使用三角形法线（每个面使用同一个法线）
        TGAColor glFragColor = { 255, 255, 255, 255 };
        
        // 计算三角形法线
        vec3 n = normalized(cross(tri[1] - tri[0], tri[2] - tri[0]));
        double diff = (std::max)(0., n * l.xyz());
        
        // 应用漫反射
        double ambient = 0.3;
        for (int channel : {0, 1, 2})
        {
            glFragColor[channel] = static_cast<std::uint8_t>(glFragColor[channel] * (std::min)(1., ambient + 0.7 * diff));
        }
        
        return { false, glFragColor };
    }
};

struct PhongShader : IShader 
{
    const Model& model;
    vec3 tri[3];       // triangle in eye coordinates
    vec3 varying_n[3]; // per-vertex normal
    vec2 varyingUV[3]; vec4 l;

    PhongShader(const vec3 light, const Model& m) : model(m) 
    {
        l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0.0 }));
    }

    virtual vec4 vertex(const int face, const int vert) 
    {
        varyingUV[vert] = model.uv(face, vert);
        varying_n[vert] = (ModelView.invert_transpose() * model.normal(face, vert)).xyz();
        vec4 gl_Position = ModelView * model.vert(face, vert);
        tri[vert] = gl_Position.xyz();
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const 
    {
        TGAColor glFragColor = { 255, 255, 255, 255 };

        // 法线插值
        vec3 n = normalized(bar[0] * varying_n[0] + bar[1] * varying_n[1] + bar[2] * varying_n[2]);

        // 计算反射向量
        vec3 r = normalized(n * ((std::max)(0., n * l.xyz())) * 2.0 - l.xyz());

        // 光照计算
        double ambient = 0.3;
        double diff = (std::max)(0., n * l.xyz());
        double spec = std::pow((std::max)(0., r.z), 35.0);

        for (int channel : {0, 1, 2}) {
            glFragColor[channel] = static_cast<std::uint8_t>(
                glFragColor[channel] * (std::min)(1., ambient + 0.75 * diff) *
                (std::min)(1., ambient + 0.4 * diff + 0.9 * spec)
                );
        }

        return { false, glFragColor };
    }
};

struct TextureShader : IShader 
{
    const Model& model;
    vec2  varying_uv[3]; // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    vec4 varying_nrm[3]; // normal per vertex to be interpolated by the fragment shader
    vec4 tri[3];         // triangle in view coordinates
    vec4 l;

    TextureShader(const vec3 light, const Model& m) : model(m) 
    {
        l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0.0 }));
    }

    virtual vec4 vertex(const int face, const int vert)
    {
        varying_uv[vert] = model.uv(face, vert);
        varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        vec4 gl_Position = ModelView * model.vert(face, vert);
        tri[vert] = gl_Position;
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const 
    {
        mat<2, 4> E = { tri[1] - tri[0], tri[2] - tri[0] };
        mat<2, 2> U = { varying_uv[1] - varying_uv[0], varying_uv[2] - varying_uv[0] };
        mat<2, 4> T = U.invert() * E;
        mat<4, 4> D = { normalized(T[0]),  // tangent vector
                        normalized(T[1]),  // bitangent vector
                        normalized(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]), // interpolated normal
                        {0,0,0,1} }; // Darboux frame
        vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];

        // 从法线图读取法线
        vec4 n = normalized(D.transpose() * model.normal(uv));
        vec4 r = normalized(n * (n * l) * 2 - l);

        // 光照计算
        double ambient = 0.3;
        double diff = (std::max)(0., n * l);
        double spec = std::pow((std::max)(0., r.z), 35.0);

        TGAColor glFragColor = { 255, 255, 255, 255 };
        for (int channel : {0, 1, 2})
        {
            glFragColor[channel] = static_cast<std::uint8_t>(
            glFragColor[channel] * (std::min)(1., ambient + 0.75 * diff) * 
            (std::min)(1., ambient + 0.4 * diff + 0.9 * spec)
            );
        }

        return { false, glFragColor };
    }
};

    // ============================================================================
    // 相机与矩阵更新
    // ============================================================================

    void UpdateCamera(AppState& state) 
    {
        // 应用物体自身的旋转
        mat<4, 4> RotY = { {{std::cos(state.obj_rot_y), 0, std::sin(state.obj_rot_y), 0},
                           {0, 1, 0, 0},
                           {-std::sin(state.obj_rot_y), 0, std::cos(state.obj_rot_y), 0},
                           {0, 0, 0, 1}} };

        mat<4, 4> RotX = { {{1, 0, 0, 0},
                           {0, std::cos(state.obj_rot_x), -std::sin(state.obj_rot_x), 0},
                           {0, std::sin(state.obj_rot_x), std::cos(state.obj_rot_x), 0},
                           {0, 0, 0, 1}} };

        ModelView = ModelView * RotY * RotX;
    }

    void HandleMouseRotation(AppState& state, int delta_x, int delta_y) {
        double rotation_speed = 0.01;

        double angle_y = delta_x * rotation_speed;
        double angle_x = delta_y * rotation_speed;

        vec3 rel_eye = state.eye - state.center;

        // 绕Y轴旋转
        double cos_y = std::cos(angle_y);
        double sin_y = std::sin(angle_y);
        vec3 rotated_y = {
            rel_eye.x * cos_y - rel_eye.z * sin_y,
            rel_eye.y,
            rel_eye.x * sin_y + rel_eye.z * cos_y
        };

        // 绕X轴旋转
        vec3 right = normalized(cross(state.up, rotated_y));
        double cos_x = std::cos(angle_x);
        double sin_x = std::sin(angle_x);
        vec3 new_up = cross(rotated_y, right);
        vec3 rotated_x = rotated_y * cos_x + new_up * sin_x;

        state.eye = state.center + rotated_x;
        state.up = normalized(cross(rotated_x, right));

        UpdateCamera(state);
    }

    void HandleMouseZoom(AppState& state, int delta) {
        double zoom_speed = 0.1;
        double distance = norm(state.eye - state.center);
        double new_distance = distance - delta * zoom_speed;

        if (new_distance > 0.1 && new_distance < 50.0) {
            vec3 direction = normalized(state.eye - state.center);
            state.eye = state.center + direction * new_distance;
            UpdateCamera(state);
        }
    }

    void LoadModel(AppState& state, const std::string& model_path) {
        try {
            state.model = std::make_unique<Model>(model_path.c_str());
            state.current_model_path = model_path;
            state.need_redraw = true;
            std::cout << "成功加载模型: " << model_path << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "加载模型失败: " << model_path << " - " << e.what() << std::endl;
        }
    }

    // ============================================================================
    // 渲染函数
    // ============================================================================

    void RenderCurrentFrame(AppState& state, TGAImage& framebuffer) {
        if (!state.model) {
            return;
        }

        // 清空画布与Z-Buffer
        framebuffer = TGAImage(state.screen_width, state.screen_height, TGAImage::RGBA,
            TGAColor{ 177, 195, 209, 255 });
        init_zbuffer(state.screen_width, state.screen_height);

        // 根据当前渲染模式分发
        switch (state.current_mode) {
        case RenderMode::WIREFRAME: {
            WireframeShader shader(*state.model);
            for (int f = 0; f < state.model->nfaces(); f++) {
                Triangle clip = {
                    shader.vertex(f, 0),
                    shader.vertex(f, 1),
                    shader.vertex(f, 2)
                };
                rasterize(clip, shader, framebuffer);
            }
            break;
        }

        case RenderMode::FLAT_SHADING: {
            FlatShader shader(state.light, *state.model);
            for (int f = 0; f < state.model->nfaces(); f++) {
                Triangle clip = {
                    shader.vertex(f, 0),
                    shader.vertex(f, 1),
                    shader.vertex(f, 2)
                };
                rasterize(clip, shader, framebuffer);
            }
            break;
        }

        case RenderMode::PHONG_SHADING: {
            PhongShader shader(state.light, *state.model);
            for (int f = 0; f < state.model->nfaces(); f++) {
                Triangle clip = {
                    shader.vertex(f, 0),
                    shader.vertex(f, 1),
                    shader.vertex(f, 2)
                };
                rasterize(clip, shader, framebuffer);
            }
            break;
        }

        case RenderMode::TEXTURE_MAPPING: {
            TextureShader shader(state.light, *state.model);
            for (int f = 0; f < state.model->nfaces(); f++) {
                Triangle clip = {
                    shader.vertex(f, 0),
                    shader.vertex(f, 1),
                    shader.vertex(f, 2)
                };
                rasterize(clip, shader, framebuffer);
            }
            break;
        }
        }
    }

    // ============================================================================
    // SDL3 事件处理与主循环
    // ============================================================================

    bool ProcessSDLEvent(AppState& state, const SDL_Event& event) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            state.should_exit = true;
            return true;

        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
            case SDLK_ESCAPE:
                state.should_exit = true;
                return true;

            case SDLK_1:
            {
                state.current_model_idx = 0;
                LoadModel(state, state.model_list[0]);
                std::cout << "切换到模型 1 " << std::endl;
                return true;
            }

            case SDLK_2:
            {
                state.current_model_idx = 1;
                LoadModel(state, state.model_list[1]);
                std::cout << "切换到模型 2 " << std::endl;
                return true;
            }

            case SDLK_3:
            {
                state.current_model_idx = 2;
                LoadModel(state, state.model_list[2]);
                std::cout << "切换到模型 3 " << std::endl;
                return true;
            }

            case SDLK_4:
                state.current_mode = RenderMode::WIREFRAME;
                state.need_redraw = true;
                std::cout << "切换到 Wireframe 模式" << std::endl;
                return true;

            case SDLK_5:
                state.current_mode = RenderMode::FLAT_SHADING;
                state.need_redraw = true;
                std::cout << "切换到 Flat Shading 模式" << std::endl;
                return true;

            case SDLK_6:
                state.current_mode = RenderMode::PHONG_SHADING;
                state.need_redraw = true;
                std::cout << "切换到 Phong Shading 模式" << std::endl;
                return true;

            case SDLK_7:
                state.current_mode = RenderMode::TEXTURE_MAPPING;
                state.need_redraw = true;
                std::cout << "切换到 Texture Mapping 模式" << std::endl;
                return true;

            case SDLK_S:
                // S 键被按下，标记待保存
                state.need_redraw = true;
                std::cout << "已保存渲染结果" << std::endl;
                return true;

            case SDLK_LEFT:
                state.obj_rot_y -= 0.1;
                UpdateCamera(state);
                state.need_redraw = true;
                std::cout << "模型向左旋转" << std::endl;
                return true;

            case SDLK_RIGHT:
                state.obj_rot_y += 0.1;
                UpdateCamera(state);
                state.need_redraw = true;
                std::cout << "模型向右旋转" << std::endl;
                return true;

            case SDLK_UP:
                state.obj_rot_x -= 0.1;
                UpdateCamera(state);
                state.need_redraw = true;
                std::cout << "模型向上旋转" << std::endl;
                return true;

            case SDLK_DOWN:
                state.obj_rot_x += 0.1;
                UpdateCamera(state);
                state.need_redraw = true;
                std::cout << "模型向下旋转" << std::endl;
                return true;

            default:
                break;
            }
            break;

            // 在 ProcessSDLEvent 函数中替换鼠标相关的 case：

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                float mx = event.button.x;
                float my = event.button.y;

                if (mx < 800.0f) {
                    state.mouse_dragging = true;
                    state.last_mouse_x = static_cast<int>(mx);
                    state.last_mouse_y = static_cast<int>(my);
                }
                else {
                    // UI 区域碰撞检测
                    if (mx >= 820.0f && mx <= 980.0f) {
                        // 上一个模型 (Y: 50~90)
                        if (my >= 50.0f && my <= 90.0f) {
                            state.current_model_idx = (state.current_model_idx - 1 + state.model_list.size()) % state.model_list.size();
                            LoadModel(state, state.model_list[state.current_model_idx]);
                            state.need_redraw = true;
                        }
                        // 下一个模型 (Y: 110~150)
                        else if (my >= 110.0f && my <= 150.0f) {
                            state.current_model_idx = (state.current_model_idx + 1) % state.model_list.size();
                            LoadModel(state, state.model_list[state.current_model_idx]);
                            state.need_redraw = true;
                        }
                    }

                    // 旋转按钮碰撞检测 (Y: 190~230)
                    if (my >= 190.0f && my <= 230.0f) {
                        // 向左转 (X: 820~895)
                        if (mx >= 820.0f && mx <= 895.0f) {
                            state.obj_rot_y -= 0.2; // 调整旋转步长
                            UpdateCamera(state);
                            state.need_redraw = true;
                            std::cout << "模型向左旋转" << std::endl;
                        }
                        // 向右转 (X: 905~980)
                        else if (mx >= 905.0f && mx <= 980.0f) {
                            state.obj_rot_y += 0.2;
                            UpdateCamera(state);
                            state.need_redraw = true;
                            std::cout << "模型向右旋转" << std::endl;
                        }
                    }
                }
            }
            return true;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                state.mouse_dragging = false;
            }
            return true;

        case SDL_EVENT_MOUSE_MOTION:
            if (state.mouse_dragging) {
                int current_x = static_cast<int>(event.motion.x);
                int current_y = static_cast<int>(event.motion.y);
                int delta_x = current_x - state.last_mouse_x;
                int delta_y = current_y - state.last_mouse_y;

                HandleMouseRotation(state, delta_x, delta_y);
                state.need_redraw = true;

                state.last_mouse_x = current_x;
                state.last_mouse_y = current_y;
            }
            return true;

        case SDL_EVENT_MOUSE_WHEEL:
        {
            // event.wheel.y: 正数向上滚，负数向下滚
            int delta = static_cast<int>(event.wheel.y);
            HandleMouseZoom(state, delta);
            state.need_redraw = true;
        }
        return true;

        default:
            break;
        }

        return false;
    }

    void CopyTGAToTexture(const TGAImage& tga, SDL_Texture* texture) {
        // 改用 SDL_UpdateTexture 避免锁定问题
        std::vector<uint32_t> pixels(tga.width() * tga.height());
        for (int y = 0; y < tga.height(); y++) {
            for (int x = 0; x < tga.width(); x++) {
                TGAColor color = tga.get(x, y);
                // SDL_PIXELFORMAT_RGBA32：字节序 R, G, B, A（小端时）
                uint32_t pixel = (static_cast<uint32_t>(color[0]) << 24) | // R
                    (static_cast<uint32_t>(color[1]) << 16) | // G
                    (static_cast<uint32_t>(color[2]) << 8) | // B
                    (static_cast<uint32_t>(color[3]));        // A
                // 翻转 Y：TGA 第 y 行（从底部数）→ SDL 第 (height-1-y) 行（从顶部数）
                pixels[(tga.height() - 1 - y) * tga.width() + x] = pixel;
            }
        }
        SDL_UpdateTexture(texture, nullptr, pixels.data(), tga.width() * 4);
    }

    void DrawButtonWithText(SDL_Renderer* renderer, TTF_Font* font,
        float x, float y, float w, float h,
        const char* text, SDL_Color bgColor) {
        // 1. 绘制按钮背景
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_FRect btn_rect = { x, y, w, h };
        SDL_RenderFillRect(renderer, &btn_rect);

        if (!font) return;

        // 2. 渲染文字为 Surface (使用 UTF8 处理中文)
        SDL_Color textColor = { 255, 255, 255, 255 }; // 白色文字
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, 0, textColor);
        if (textSurface) {
            // 3. 将 Surface 转换为可以由 GPU 渲染的 Texture
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

            // 4. 计算文字居中的位置
            SDL_FRect text_rect;
            text_rect.w = static_cast<float>(textSurface->w);
            text_rect.h = static_cast<float>(textSurface->h);
            text_rect.x = x + (w - text_rect.w) / 2.0f;
            text_rect.y = y + (h - text_rect.h) / 2.0f;

            // 5. 渲染文字纹理
            SDL_RenderTexture(renderer, textTexture, nullptr, &text_rect);

            // 6. 务必释放资源，否则每秒 60 帧会瞬间耗尽内存
            SDL_DestroyTexture(textTexture);
            SDL_DestroySurface(textSurface);
        }
    }

    int main(int argc, char** argv) {
        // 初始化 SDL3
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL 初始化失败: " << SDL_GetError() << std::endl;
            return 1;
        }

        // 在 SDL_Init 之后初始化 TTF
        if (TTF_Init() == -1) {
            std::cerr << "TTF 初始化失败: " << SDL_GetError() << std::endl;
            return 1;
        }

        // 加载支持中文的字体 (Windows 下的微软雅黑，字号 18)
        // 请确保路径正确，如果是其他系统或相对路径，请替换为你的 .ttf 文件路径
        TTF_Font* font = TTF_OpenFont("C:\\Windows\\Fonts\\msyh.ttc", 18);
        if (!font) {
            std::cerr << "加载字体失败: " << SDL_GetError() << std::endl;
            // 记得处理错误
        }

        // ========== 创建 SDL 窗口 ==========
        SDL_Window* window = SDL_CreateWindow(
            "TinyRenderer - Interactive Viewer",
            1000, 800,
            SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            std::cerr << "窗口创建失败: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        // 创建渲染器
        SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            std::cerr << "渲染器创建失败: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        // 创建纹理用于显示 TGA 图像
        SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            800, 800
        );

        if (!texture) {
            std::cerr << "纹理创建失败: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        // ========== 应用状态初始化 ==========
        AppState state;
        state.screen_width = 800;
        state.screen_height = 800;

        int vp_w = state.screen_width * 7 / 8;
        int vp_h = state.screen_height * 7 / 8;

        double dist = norm(state.center - state.eye);
        init_view(state.eye, state.center, state.up);
        init_perspective(dist, vp_w, vp_h);

        // 初始化视口矩阵
        init_viewport(state.screen_width / 16, state.screen_height / 16, vp_w, vp_h);


        // 尝试加载默认模型
        LoadModel(state, state.current_model_path);
        if (!state.model) {
            std::cerr << "无法加载默认模型，程序退出。" << std::endl;
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        // 初始化相机
        UpdateCamera(state);

        // 创建帧缓冲
        TGAImage framebuffer(state.screen_width, state.screen_height, TGAImage::RGBA,
            TGAColor{ 177, 195, 209, 255 });

        // ========== 主事件循环 ==========
        bool screenshot_pending = false;

        while (!state.should_exit) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_S) {
                    screenshot_pending = true;
                }
                ProcessSDLEvent(state, event);
            }

            // 如果需要重绘，则渲染新帧
            if (state.need_redraw) {
                RenderCurrentFrame(state, framebuffer);
                CopyTGAToTexture(framebuffer, texture);
                state.need_redraw = false;

                // 如果有截图待处理，立即保存
                if (screenshot_pending) {
                    framebuffer.write_tga_file("screenshot.tga");
                    std::cout << "截图已保存: screenshot.tga" << std::endl;
                    screenshot_pending = false;
                }
            }

            // 1. 清屏
            SDL_RenderClear(renderer);

            // 2. 渲染 3D 画面到左侧 (0~800)
            SDL_FRect dst_rect = { 0.0f, 0.0f, 800.0f, 800.0f };
            SDL_RenderTexture(renderer, texture, nullptr, &dst_rect);

            // 3. 绘制右侧 UI 面板背景
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_FRect ui_panel = { 800.0f, 0.0f, 200.0f, 800.0f };
            SDL_RenderFillRect(renderer, &ui_panel);

            // 4. 使用辅助函数绘制带文字的按钮
            SDL_Color btnColor = { 100, 150, 200, 255 }; // 统一的按钮颜色
            SDL_Color tipColor = { 50, 50, 50, 255 }; // 提示文字背景色，与面板融合

            // 模型切换按钮
            DrawButtonWithText(renderer, font, 820.0f, 50.0f, 160.0f, 40.0f, "next", btnColor);
            DrawButtonWithText(renderer, font, 820.0f, 110.0f, 160.0f, 40.0f, "previous", btnColor);

            // 角色旋转按钮
            DrawButtonWithText(renderer, font, 820.0f, 190.0f, 75.0f, 40.0f, "left", btnColor);
            DrawButtonWithText(renderer, font, 905.0f, 190.0f, 75.0f, 40.0f, "right", btnColor);

            // 操作提示文字
            DrawButtonWithText(renderer, font, 820.0f, 260.0f, 160.0f, 25.0f, "Key 1/2/3: Switch Model", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 295.0f, 160.0f, 25.0f, "Key 4: Wireframe", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 330.0f, 160.0f, 25.0f, "Key 5: Flat Shading", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 365.0f, 160.0f, 25.0f, "Key 6: Phong Shading", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 400.0f, 160.0f, 25.0f, "Key 7: Texture Map", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 450.0f, 160.0f, 25.0f, "Mouse Wheel: Zoom", tipColor);
            DrawButtonWithText(renderer, font, 820.0f, 485.0f, 160.0f, 25.0f, "Left Click Drag: Rotate", tipColor);

            // 5. 提交渲染
            SDL_RenderPresent(renderer);

            // 控制帧率（60 FPS）
            SDL_Delay(16);
        }

        // ========== 清理资源 ==========
        std::cout << "关闭应用程序..." << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        TTF_Quit();

        return 0;
    }