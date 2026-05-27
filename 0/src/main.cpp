#include<cmath>
#include<tuple>

#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(255, 128,  64, 255);
const TGAColor yellow = TGAColor(0, 200, 255, 255);

constexpr int width = 800;
constexpr int height = 800;

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

	float y = yOri;
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

std::tuple<int, int> project(vec3 v)
{
	return { (v.x + 1.) * width / 2,
		(v.y + 1.) * height / 2 };
}

int main(int argc, char** argv) 
{
	Model model("C:/Games101/tinyrenderer/0/src/obj/african_head/african_head.obj");

	TGAImage image(width, height, TGAImage::RGB);

	for (int i = 0; i < model.nfaces(); i++)
	{
		auto [ax, ay] = project(model.vert(i, 0));
		auto [bx, by] = project(model.vert(i, 1));
		auto [cx, cy] = project(model.vert(i, 2));

		line(ax, ay, bx, by, image, red);
		line(bx, by, cx, cy, image, red);
		line(cx, cy, ax, ay, image, red);
	}
	

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

