/* 
 * FOFOLITO - Sistema Operacional para RaspberryPi
 *
 * Marcos Medeiros
 */
#include <kernel/fb.h>
#include <memory.h>
#include <errno.h>
#include <types.h>

struct fbdev *default_fb = NULL;

/* Registra um framebuffer na arvore de dispositivos do sistema */
int fb_register_device(struct fbdev *dev)
{
	default_fb = dev;
	return -EOK;
}

void fb_set_mode()
{
	/* Se existir algum dispositivo de framebuffer */
	if (default_fb)
		default_fb->modeset(default_fb, NULL);
}

struct fbdev *fb_get_device()
{
	return default_fb;
}

/* Mapeia uma cor */
uint fb_generic_maprgb(struct fbdev *dev, uint r, uint g, uint b)
{
	uint color = 0;
	if (!dev)
		return 0;

	/* Cálcula o valor para cada BPP */
	switch (dev->bpp) {
		/* No padrão RGB565 */
		case 16:
			r = (32 * r) / 256;
			g = (64 * g) / 256;
			b = (32 * b) / 256;
			color = ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
			break;
		case 24:
		/* sem suporte para canais alpha */
		case 32:
			color = ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
			break;

	}

	return color;
}

void fb_generic_rectfill(struct fbdev *dev, struct fbdev_rect *rect, uint color)
{
	int x = rect->x;
	int y = rect->y;
	int w = rect->w;
	int h = rect->h;
	int ly = 0;
	int lx = 0;

	/* Verifica se as coordenadas estão fora do framebuffer */
	if (x >= dev->width || y >= dev->height)
		return;

	if ((x + w) >= dev->width)
		w = dev->width - x;
	if ((y + h) >= dev->height)
		h = dev->height - y;

	/* cálcula os valores finais de x e y */
	ly = y + h;
	lx = x + w;

	switch (dev->bpp) {
		case 16: {
			while (y < ly)  {
				/* reinicia o valor de x inicial */
				x = rect->x;
				ushort *ptr = __fb_pos(dev, x, y);
				while (x < lx) {
					*ptr++ = color;
					x++;
				}
				y++;
			}
			break;
		}
		case 32: {
			while (y < ly)  {
				/* reinicia o valor de x inicial */
				x = rect->x;
				uint *ptr = __fb_pos(dev, x, y);
				while (x < lx) {
					*ptr++ = color;
					x++;
				}
				y++;
			}
			break;
		}
	}
}

void fb_generic_scroll(struct fbdev *dev, int px, uint bg)
{
	void *dst = dev->base;
	void *src = __fb_pos(dev, 0, px);
	uint size = dev->size - (px * dev->pitch);
	memcpy_s(dst, src, size);

	struct fbdev_rect r;
	r.x = 0;
	r.y = dev->height - px;
	r.w = dev->width;
	r.h = px;
	fb_generic_rectfill(dev, &r, bg);
}

void fb_generic_drawchar(struct fbdev *dev, const struct fbdev_font *font,
						 int x, int y, char chr, int bg, int fg)
{
	/* Verifica se está fora do framebuffer */
	if (x >= dev->width || y >= dev->height)
		return;

	int ix = x;
	
	/* Cálcula as posições finais */
	int lx = x + font->width;
	int ly = y + font->height;

	int line = 0;

	if (lx >= dev->width)
		lx = dev->width;
	if (ly >= dev->height)
		ly = dev->height;

	while (y <= ly) {
		x = ix;
		ushort *ptr = __fb_pos(dev, x, y);

		ushort bits = font->data[chr * font->height + line];
		while (x <= lx) {
			if (bits & 0x80)
				*ptr = fg;
			else
				*ptr = bg;
			bits <<= 1;
			ptr++;
			x++;
		}
		y++;
		line++;
	}
}

/*
 * Essa é uma demonstração no framebuffer, exibe barras de cores em gradiente
 */
void fb_color_test_demo()
{
	struct fbdev *dev = fb_get_device();

	struct fbdev_rect rect;
	ushort color = 0;

	uint bars = 255;
	uint barsize = (dev->width - 20) / bars;
	const uint delta = 255 / bars;

	uint k = 0;
	rect.x = 10;
	rect.y = 10;
	rect.w = barsize;
	rect.h = 50;

	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, 0, k * delta, 0);
		rect.x += barsize;
	}

	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, k * delta, 0, 0);
		rect.x += barsize;
	}
	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, 0, 0, k * delta);
		rect.x += barsize;
	}

	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, 0, k * delta, k * delta);
		rect.x += barsize;
	}

	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, k * delta, 0, k * delta);
		rect.x += barsize;
	}
	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, k * delta, k * delta, 0);
		rect.x += barsize;
	}

	rect.y += rect.h;
	rect.x = 10;
	color = 0;
	for (k = 0; k < bars; k++) {
		fb_generic_rectfill(dev, &rect, color);
		color = dev->maprgb(dev, k * delta, k * delta, k * delta);
		rect.x += barsize;
	}
}