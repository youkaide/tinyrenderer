#include <windows.h> // 添加此头文件以定义 MAX_PATH
#include <filesystem>
#include <string>
#include <vector>

#include <cstdlib>
#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

struct PhoneShader : IShader {
    const Model& model;
    vec2 varyingUV[3];
    vec4 l;       

    PhoneShader(const vec3 light, const Model& m) : model(m)
    {
        l = normalized((ModelView * vec4{ light.x,light.y,light.z,0 }));
    }

    virtual vec4 vertex(const int face, const int vert)
    {
        varyingUV[vert] = model.uv(face, vert);
        vec4 gl_Position = ModelView * model.vert(face, vert);
        return Perspective * gl_Position;                         // in clip coordinates
    }


    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const 
    {
        TGAColor glFragColor = { 255,255,255,255 };
        vec2 uv = bar[0] * varyingUV[0] + bar[1] * varyingUV[1] + bar[2] * varyingUV[2];
        vec4 n = normalized(ModelView.invert_transpose() * model.normal(uv));
        vec4 r = normalized(n * (n * l) * 2 - l);
        
        double ambient = 0.3;
        double diff = (std::max)(0., n * l);
        double spec = std::pow((std::max)(r.z, 0.), 35);

        for (int channel : {0, 1, 2})
        {
            glFragColor[channel] *= (std::min)(1., ambient + .75 * diff);
            glFragColor[channel] *= (std::min)(1., ambient + .4 * diff + .9 * spec);
        }

        return { false, glFragColor };                                    // do not discard the pixel
    }
};

int main(int argc, char** argv) {
    // ---------- 新的模型加载逻辑 ----------
    std::string modelPath;

    // 1. 优先使用命令行参数
    if (argc >= 2) {
        modelPath = argv[1];
    }
    else {
        std::filesystem::path exePath;
#if defined(_WIN32)
        wchar_t buf[MAX_PATH];
        GetModuleFileNameW(NULL, buf, MAX_PATH);
        exePath = std::filesystem::path(buf).parent_path();
#else
        exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
        std::filesystem::path modelsDir = exePath / "models";

        if (std::filesystem::exists(modelsDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(modelsDir)) {
                if (entry.path().extension() == ".obj") {
                    modelPath = entry.path().string();
                    break;
                }
            }
        }
        if (modelPath.empty()) {
            std::filesystem::path cwdModels = std::filesystem::current_path() / "models";
            if (std::filesystem::exists(cwdModels)) {
                for (const auto& entry : std::filesystem::directory_iterator(cwdModels)) {
                    if (entry.path().extension() == ".obj") {
                        modelPath = entry.path().string();
                        break;
                    }
                }
            }
        }

        if (modelPath.empty()) {
            std::filesystem::path candidate = "C:\\Games101\\tinyrenderer\\out\\build\\x64-debug\\obj\\african_head\\african_head.obj";
            if (std::filesystem::exists(candidate)) {
                modelPath = candidate.string();
            }
        }

        if (modelPath.empty()) {
            std::cerr << "No .obj file provided.\n"
                << "Drag a .obj file onto the exe, pass it as command line argument,\n"
                << "or place .obj files in a 'models' folder next to the executable.\n";
            return 1;
        }
    }

    // 直接在栈上创建模型并使用它（避免上面先 new 但未使用的问题）
    Model model(modelPath.c_str());
    // ----------------------------------------

    constexpr int width = 800;      // output image size
    constexpr int height = 800;
    constexpr vec3    eye{ -1, 0, 2 }; // camera position
    constexpr vec3 center{ 0, 0, 0 }; // camera direction
    constexpr vec3     up{ 0, 1, 0 }; // camera up vector
    constexpr vec3  light{ 1, 1, 1 };

    lookat(eye, center, up);                                   // build the ModelView   matrix
    init_perspective(norm(eye - center));                        // build the Perspective matrix
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
    init_zbuffer(width, height);

    TGAImage framebuffer(width, height, TGAImage::RGBA, { 177, 195, 209, 255 });

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    PhoneShader shader(light, model);
    for (int f = 0; f < model.nfaces(); f++) {      // iterate through all facets
        Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                          shader.vertex(f, 1),
                          shader.vertex(f, 2) };
        rasterize(clip, shader, framebuffer);   // rasterize the primitive
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}