#include <algorithm>
#include "our_gl.h"

#define M_PI 3.14159265358979323846

mat<4, 4> ModelView, Viewport, Perspective; // "OpenGL" state matrices
std::vector<double> zbuffer;               // depth buffer

void init_view(const vec3 eye, const vec3 target, const vec3 up)
{
    vec3 forward = normalized(target - eye);
    vec3 right = normalized(cross(forward, up));
    vec3 newUp = normalized(cross(right, forward));

    ModelView = mat<4, 4>
    { {
        {right.x,right.y,right.z,-dot(right,eye)},
        {newUp.x,newUp.y,newUp.z,-dot(newUp,eye)},
        {-forward.x,-forward.y,-forward.z,dot(forward,eye)},//相机朝着-z看
        {0,0,0,1}
    } };
}

void init_perspective(const double f, const int w, const int h)
{
    double fov = 60.0 * M_PI / 180.0;
    double near = f * 0.1;
    double far = f * 100.0;
    double t = std::tan(fov / 2);
    double aspect = (double)w / h;

    Perspective = { {{1.0 / (aspect * t),0,0,0}, {0,1 / t,0,0}, {0,0,(near + far) / (near - far),2.0 * near * far / (near - far)}, {0,0,-1,0}} };
}

void init_viewport(const int x, const int y, const int w, const int h)
{
    Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
}

void init_zbuffer(const int width, const int height) 
{
    zbuffer = std::vector(width * height, 1.);
}

void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer)
{
    vec4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };                // normalized device coordinates
    vec2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; // screen coordinates

    mat<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} } };
    if (ABC.det() < 1) return; // backface culling + discarding triangles that cover less than a pixel

    auto [bbminx, bbmaxx] = std::minmax({ screen[0].x, screen[1].x, screen[2].x }); // bounding box for the triangle
    auto [bbminy, bbmaxy] = std::minmax({ screen[0].y, screen[1].y, screen[2].y }); // defined by its top left and bottom right corners
#pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) 
    {         // clip the bounding box by the screen
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++)
        {
            vec3 bc = ABC.invert_transpose() * vec3 { static_cast<double>(x), static_cast<double>(y), 1. }; // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                    // negative barycentric coordinate => the pixel is outside the triangle
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };  // linear interpolation of the depth

            if (z >= zbuffer[x + y * framebuffer.width()]) continue;   // discard fragments that are too deep w.r.t the z-buffer
            auto [discard, color] = shader.fragment(bc);

            if (discard) continue;                                 // fragment shader can discard current fragment
            zbuffer[x + y * framebuffer.width()] = z;                  // update the z-buffer
            framebuffer.set(x, y, color);                          // update the framebuffer
        }
    }
}