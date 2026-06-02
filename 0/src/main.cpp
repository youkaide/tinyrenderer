#include<cmath>
#include<tuple>

#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(255, 128,  64, 255);
const TGAColor yellow = TGAColor(0, 200, 255, 255);

constexpr int width = 128;
constexpr int height = 128;

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

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& image, TGAColor color)
{
	int maxX = std::max(std::max(ax, bx), cx);
	int maxY = std::max(std::max(ay, by), cy);
	int minX = std::min(std::min(ax, bx), cx);
	int minY = std::min(std::min(ay, by), cy);

	double totalArea = signed_triangle_area(ax, ay, bx, by, cx, cy);

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

			image.set(x, y, color);
		}
	}
}

std::tuple<int, int> project(vec3 v)
{
	return { (v.x + 1.) * width / 2,
		(v.y + 1.) * height / 2 };
}

int main(int argc, char** argv) 
{
	TGAImage image(width, height, TGAImage::RGB);

	triangle(7, 45, 35, 100, 45, 60, image, red);
	triangle(120, 35, 90, 5, 45, 110, image, white);
	triangle(115, 83, 80, 90, 85, 120, image, green);
	

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

