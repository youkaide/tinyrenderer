#define _USE_MATH_DEFINES
#include<cmath>
#include<tuple>

#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(255, 128,  64, 255);
const TGAColor yellow = TGAColor(0, 200, 255, 255);
const TGAColor black = TGAColor(0, 0, 0, 0);

constexpr int width = 1280;
constexpr int height = 1280;

void line(int xOri, int yOri, int xEnd, int yEnd, TGAImage& image,TGAColor color)
{
	bool steep = std::abs(xEnd - xOri) < std::abs(yEnd - yOri);

	if (steep)
	{
		std::swap(xOri, yOri);
		std::swap(xEnd, yEnd);
	}

	if (xOri > xEnd)
	{
		std::swap(xOri, xEnd);
		std::swap(yOri, yEnd);
	}

	int y = yOri;
	int iError = 0;

	for (int x = xOri; x <= xEnd; x += 1)
	{
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		iError += 2 * std::abs(yEnd - yOri);
		if (iError > (xEnd - xOri))
		{
			y += yOri > yEnd ? -1 : 1;
			iError -= 2 * (xEnd - xOri);
		}
	}
}

double signed_triangle_area(int x1, int y1, int x2, int y2, int x3, int y3)
{
	return 0.5 * (x1 * y2 - x2 * y1 + x2 * y3 - x3 * y2 + x3 * y1 - x1 * y3);
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, int az, int bz, int cz, TGAImage& image, TGAImage& zBuffer,TGAColor color)
{
	int minX = std::max(0, std::min(std::min(ax, bx), cx));
	int minY = std::max(0, std::min(std::min(ay, by), cy));
	int maxX = std::min(width - 1, std::max(std::max(ax, bx), cx));
	int maxY = std::min(height - 1, std::max(std::max(ay, by), cy));

	double totalArea = signed_triangle_area(ax, ay, bx, by, cx, cy);

	if (totalArea < 1) return;

	for (int x = minX; x <= maxX; ++x)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			double alpha = signed_triangle_area(ax, ay, bx, by, x, y) / totalArea;
			double beta = signed_triangle_area(bx, by, cx, cy, x, y) / totalArea;
			double gamma = signed_triangle_area(cx, cy, ax, ay, x, y) / totalArea;

			if (alpha < 0 || beta < 0 || gamma < 0)
			{
				continue;
			}

			double thickness = 0.1;

			//if (alpha > thickness && beta > thickness && gamma > thickness)
			//{
			//	continue;
			//}

			uint8_t r = static_cast<uint8_t>(alpha * 255);
			uint8_t g = static_cast<uint8_t>(beta * 255);
			uint8_t b = static_cast<uint8_t>(gamma * 255);

			double z_float = alpha * az + beta * bz + gamma * cz;
			unsigned char z = static_cast<unsigned char>(std::max(0.0, std::min(255.0, z_float)));
			TGAColor zbufColor = zBuffer.get(x, y);
			if (z <= zbufColor.b) continue;

			zBuffer.set(x, y, TGAColor{ z,z,z,z });

			image.set(x, y, color);
		}
	}
}

vec3 rotate(vec3 v)
{
	constexpr double a = M_PI / 6;
	mat3 rotationYMat = mat3::identity();
	rotationYMat[0][0] = std::cos(a);
	rotationYMat[0][2] = std::sin(a);
	rotationYMat[2][0] = -std::sin(a);
	rotationYMat[2][2] = std::cos(a);
	return rotationYMat * v;
}

vec3 perspective(vec3 v)
{
	constexpr double c = 3.;
	return v / (1 - v.z / c);
}

std::tuple<int, int, int> project(vec3 v)
{
	return { (v.x + 1.) * width / 2,
		(v.y + 1.) * height / 2,
		(v.z + 1.) * 255. / 2 };
}

int main(int argc, char** argv) 
{
	Model model = "C:\\Games101\\tinyrenderer\\0\\src\\obj\\diablo3_pose\\diablo3_pose.obj";

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zBufferImage(width, height, TGAImage::GRAYSCALE);

	for (int i = 0; i < model.nfaces(); ++i)
	{
		auto [ax, ay, az] = project(perspective(rotate(model.vert(i, 0))));
		auto [bx, by, bz] = project(perspective(rotate(model.vert(i, 1))));
		auto [cx, cy, cz] = project(perspective(rotate(model.vert(i, 2))));

		TGAColor color = TGAColor(rand() % 255, rand() % 255, rand() % 255, 255);
		triangle(ax, ay, bx, by, cx, cy, az, bz, cz, image, zBufferImage, color);
	}

	/*int ax = 17, ay = 4, az = 13;
	int bx = 55, by = 39, bz = 128;
	int cx = 23, cy = 59, cz = 255;

	triangle(ax, ay, bx, by, cx, cy, az, bz, cz, image);*/

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");

	zBufferImage.flip_vertically();
	zBufferImage.write_tga_file("zBuffer.tga");
	return 0;
}

