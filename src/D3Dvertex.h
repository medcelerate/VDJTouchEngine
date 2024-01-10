#define D3DFVF_TLVERTEX 1
#include <d3d11.h>
struct D3DXCOLOR
{
public:
	D3DXCOLOR() = default;
	D3DXCOLOR(FLOAT r, FLOAT g, FLOAT b, FLOAT a)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	operator FLOAT* ()
	{
		return &r;
	}

	FLOAT r, g, b, a;
};

struct TLVERTEX
{
	FLOAT x, y, z;      // position
	D3DXCOLOR colour;    // color
	FLOAT u, v;			// texture coordinate
};

template<class T> void setVertexDst(T* vertices, float dstX, float dstY, float width, float height, DWORD col)
{
	if (!vertices)
		return;

	const D3DXCOLOR color = D3DXCOLOR(GetRValue(col) / 255.f, GetGValue(col) / 255.f, GetBValue(col) / 255.f, 1.f);
	vertices[0].colour = color;
	vertices[0].x = dstX + width;
	vertices[0].y = dstY;
	vertices[0].z = 0.0f;

	vertices[1].colour = color;
	vertices[1].x = dstX + width;
	vertices[1].y = dstY + height;
	vertices[1].z = 0.0f;

	vertices[2].colour = color;
	vertices[2].x = dstX;
	vertices[2].y = dstY + height;
	vertices[2].z = 0.0f;

	vertices[3].colour = color;
	vertices[3].x = dstX;
	vertices[3].y = dstY + height;
	vertices[3].z = 0.0f;

	vertices[4].colour = color;
	vertices[4].x = dstX;
	vertices[4].y = dstY;
	vertices[4].z = 0.0f;

	vertices[5].colour = color;
	vertices[5].x = dstX + width;
	vertices[5].y = dstY;
	vertices[5].z = 0.0f;
}

template<class T> void setVertexSrc(T* vertices, float srcX, float srcY, float srcWidth, float srcHeight, float textureWidth, float textureHeight)
{
	if (!vertices)
		return;

	vertices[0].u = (srcX + srcWidth) / textureWidth;
	vertices[0].v = srcY / textureHeight;

	vertices[1].u = (srcX + srcWidth) / textureWidth;
	vertices[1].v = (srcY + srcHeight) / textureHeight;

	vertices[2].u = srcX / textureWidth;
	vertices[2].v = (srcY + srcHeight) / textureHeight;

	vertices[3].u = srcX / textureWidth;
	vertices[3].v = (srcY + srcHeight) / textureHeight;

	vertices[4].u = srcX / textureWidth;
	vertices[4].v = srcY / textureHeight;

	vertices[5].u = (srcX + srcWidth) / textureWidth;
	vertices[5].v = srcY / textureHeight;
}
