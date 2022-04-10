// Some TRXDIR3 test code by Ty (Fobes)
#include "defs.h"

#include <kernel.h>
#include <tamtypes.h>
#include <stdio.h>
#include <string.h>

#include <gif_tags.h>

#include <gs_gp.h>
#include <gs_psm.h>

#include <dma.h>
#include <dma_tags.h>

#include <draw.h>
#include <graph.h>

#include <packet.h>
#include <stdlib.h>
#include <fcntl.h>

#include "tex.c" // A picture of me and my snake

const u32 WIDTH = 640;
const u32 HEIGHT = 480;
const s32 FRAME_PSM = GS_PSM_32;

qword_t *draw_texture_transfer_trx(qword_t *q, void *src, int width, int height, int psm, int dest, int dest_width, u32 custtrx = 0, u32 hoz_shift = 0);
int download_from_gs(u32 address, u32 format, const char *fname, int psm_size, int width, int height, u32 custtrx = 1);

void initialize_graphics(framebuffer_t *fb, zbuffer_t *zb)
{
	printf("Initializing graphics stuff\n");

	// Reset GIF
	(*(volatile u_int *)0x10003000) = 1;

	// Allocate framebuffer and zbuffer
	fb->width = WIDTH;
	fb->height = HEIGHT;
	fb->psm = FRAME_PSM;
	fb->mask = 0x00000000;

	fb->address = graph_vram_allocate(fb->width, fb->height, fb->psm, GRAPH_ALIGN_PAGE);

	zb->enable = 1;
	zb->zsm = GS_ZBUF_24;
	zb->address = graph_vram_allocate(fb->width, fb->height, zb->zsm, GRAPH_ALIGN_PAGE);
	zb->mask = 0;
	zb->method = ZTEST_METHOD_ALLPASS;

	graph_initialize(fb->address, fb->width, fb->height, fb->psm, 0, 0);

	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	/*
		Init drawing stuff
	*/

	packet_t *packet = packet_init(50, PACKET_NORMAL);

	qword_t *q = packet->data;

	q = draw_setup_environment(q, 0, fb, zb);
	q = draw_primitive_xyoffset(q, 0, 0, 0);
	q = draw_disable_tests(q, 0, zb);
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	draw_wait_finish();

	packet_free(packet);
	printf("Finished\n");
}

int main(void)
{

	framebuffer_t fb;
	zbuffer_t zb;
	initialize_graphics(&fb, &zb);

	packet_t *packet = packet_init(100, PACKET_NORMAL);

	qword_t *q = packet->data;

	// Clear the screen
	{
		PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_PRIM_SPRITE, GIF_FLG_PACKED, 4),
					GIF_REG_AD | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZ2 << 8) | (GIF_REG_XYZ2 << 12));
		q++;
		q->dw[0] = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
		q->dw[1] = GS_REG_TEST;
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0x33) | ((u64)0x33 << 32));
		q->dw[1] = (u64)((0x33) | ((u64)0 << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((0 << 4)) | (((u64)(0 << 4)) << 32)));
		q->dw[1] = (u64)(0);
		q++;
		// XYZ2
		q->dw[0] = (u64)((((640 << 4)) | (((u64)(480 << 4)) << 32)));
		q->dw[1] = (u64)(0);
		q++;

		q = draw_finish(q);

		dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);

		draw_wait_finish();
	}

	// Draw a 128x128 sprite
	{
		q = packet->data;
		PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_SET_PRIM(GIF_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 3),
					GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4) | (GIF_REG_XYZ2 << 8));
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0) | ((u64)0xcc << 32));
		q->dw[1] = (u64)((0xcc) | ((u64)0 << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((0 << 4)) | (((u64)(0 << 4)) << 32)));
		q->dw[1] = (u64)(0);
		q++;
		// XYZ2
		q->dw[0] = (u64)(((128 << 4)) | (((u64)(128 << 4)) << 32));
		q->dw[1] = (u64)(0);
		q++;

		q = draw_finish(q);

		dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);

		draw_wait_finish();
		graph_wait_vsync();
	}

#if 0
	// Compare TRXDIR 1 (local -> host)
	// VIF / DMA stalls, never completes
	{
		printf("Going to download with a valid TRXDIR(1)\n");
		download_from_gs(fb.address, FRAME_PSM, "TRX1-Download.raw", 32, WIDTH, HEIGHT); // Download from the GS normally
		printf("Downloaded file should be called TRX1-Download.raw!\n");
		printf("Going to download with a disabled TRXDIR(3)\n");
		printf("Hardware will freeze when doing this!\n");
		download_from_gs(fb.address, FRAME_PSM, "TRX3-Download.raw", 32, WIDTH, HEIGHT, 3); // Download from the GS with a TRX of 3
		printf("Downloaded file should be called TRX3-Download.raw!\n");
	}
#endif

#if 0
	// Compare TRXDIR 0 (host -> local)
	// No transfer happens
	{
		printf("Uploading a 32x32 texture directly into the framebuffer, 32 pixels to the right with a TRX of (0)\n");
		q = packet->data;
		// Upload to framebuffer with TRXDIR of 0, offset 32 pixels
		q = draw_texture_transfer_trx(q, tex, 32,32,GS_PSM_32,fb.address,1,0,32);
		// Upload to framebuffer with TRXDIR of 3, offset 64 pixels
		printf("Uploading a 32x32 texture directly into the framebuffer, 64 pixels to the right with a disabled TRX(3)\n");
		q = draw_texture_transfer_trx(q, tex, 32,32,GS_PSM_32,fb.address,1,3,64);
		q = draw_texture_flush(q);

		dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	}
#endif

#if 0
	// Compare TRXDIR 2 (local -> local)
	// No copy happens
	{
		printf("Uploading a sample texture to the framebuffer\n");
		// Upload a texture into the framebuffer
		q = packet->data;
		q = draw_texture_transfer(q, tex, 32,32,GS_PSM_32,fb.address,64);
		q = draw_texture_flush(q);
		dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
		

		printf("Copying the texture back into the framebuffer, 32 pixels to the right\n");
		// Copy the first 32x32 pixels in the framebuffer to the framebuffer, but shifted 32 pixels over
		q = packet->data;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_BITBLTBUF(fb.address >> 6, 1, GS_PSM_32, fb.address >> 6, 1, GS_PSM_32), GS_REG_BITBLTBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 32, 0, 0), GS_REG_TRXPOS);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXREG(32, 32), GS_REG_TRXREG);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXDIR(2), GS_REG_TRXDIR);
		q++;
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);

		printf("Copying the texture back into the framebuffer, 64 pixels to the right\n TRXDIR is set to disabled (3)");
		// Copy the first 32x32 pixels in the framebuffer to the framebuffer, but shifted 64 pixels over
		// But, set TRXDIR to 3
		q = packet->data;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_BITBLTBUF(fb.address >> 6, 1, GS_PSM_32, fb.address >> 6, 1, GS_PSM_32), GS_REG_BITBLTBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 64, 0, 0), GS_REG_TRXPOS);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXREG(32, 32), GS_REG_TRXREG);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXDIR(3), GS_REG_TRXDIR);
		q++;
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	}
#endif
	SleepThread();
}

int download_from_gs(u32 address, u32 format, const char *fname, int psm_size, int width, int height, u32 custtrx)
{
	u32 destBuffer[width * height];
	// Set the buffer to 0xcc just to make sure it doesn't contain the last download_from_gs call's
	// data when we use an invalid trxdir (which has happened :^) )
	memset(destBuffer, 0xcc, sizeof(u32) * width * height);

	static union
	{
		u32 value_u32[4];
		u128 value;
	} enable_path3 ALIGNED(16) = {
		{VIF1_MSKPATH3(0), VIF1_NOP, VIF1_NOP, VIF1_NOP}};

	u32 prev_imr;
	u32 prev_chcr;

	u32 dmaChain[20 * 2] ALIGNED(16);
	u32 i = 0; // Used to index through the chain

	u32 *pdma32 = (u32 *)&dmaChain;
	u64 *pdma64 = (u64 *)(pdma32 + 4);

	// Set up VIF packet
	pdma32[i++] = VIF1_NOP;
	pdma32[i++] = VIF1_MSKPATH3(0x8000);
	pdma32[i++] = VIF1_FLUSHA;
	pdma32[i++] = VIF1_DIRECT(6);

	// Set up our GS packet
	i = 0;

	pdma64[i++] = GIFTAG(5, 1, 0, 0, 0, 1);
	pdma64[i++] = GIF_AD;
	// pdma64[i++] = GSBITBLTBUF_SET(address / 64, width / 64, format, 0, 0, 0);
	pdma64[i++] = GSBITBLTBUF_SET(address / 64, width < 64 ? 1 : width / 64, format, 0, 0, 0);
	pdma64[i++] = GSBITBLTBUF;
	pdma64[i++] = GSTRXPOS_SET(0, 0, 0, 0, 0); // SSAX, SSAY, DSAX, DSAY, DIR
	pdma64[i++] = GSTRXPOS;
	pdma64[i++] = GSTRXREG_SET(width, height); // RRW, RRh
	pdma64[i++] = GSTRXREG;
	pdma64[i++] = 0;
	pdma64[i++] = GSFINISH;
	pdma64[i++] = GSTRXDIR_SET(custtrx); // XDIR
	pdma64[i++] = GSTRXDIR;

	prev_imr = GsPutIMR(GsGetIMR() | 0x0200);
	prev_chcr = *D1_CHCR;

	if ((*D1_CHCR & 0x0100) != 0)
		return 0;

	// Finish event
	*GS_CSR = CSR_FINISH;

	// DMA crap

	FlushCache(0);
	printf("DMA stuff\n");
	*D1_QWC = 0x7;
	*D1_MADR = (u32)pdma32;
	*D1_CHCR = 0x101;

	asm __volatile__("sync.l\n");

	// check if DMA is complete (STR=0)

	printf("Waiting for DMA channel completion\n");
	while (*D1_CHCR & 0x0100)
		;
	printf("Waiting for GS_CSR\n");
	while ((*GS_CSR & CSR_FINISH) == 0)
		;
	printf("Waiting for the vif fifo to empty\n");
	// Wait for viffifo to become empty
	while ((*VIF1_STAT & (0x1f000000)))
		;

	// Reverse busdir and transfer image to host
	*VIF1_STAT = VIF1_STAT_FDR;
	*GS_BUSDIR = (u64)0x00000001;

	FlushCache(0);

	u32 trans_size = ((width * height * psm_size) / 0x100);

	printf("Download the frame in 2 transfers of qwc 0x%x\n", trans_size);

	*D1_QWC = trans_size;
	*D1_MADR = (u32)destBuffer;
	*D1_CHCR = 0x100;

	asm __volatile__(" sync.l\n");

	// check if DMA is complete (STR=0)
	while (*D1_CHCR & 0x0100)
		;

	*D1_QWC = trans_size;
	*D1_CHCR = 0x100;

	asm __volatile__(" sync.l\n");

	// Wait for viffifo to become empty
	while ((*VIF1_STAT & (0x1f000000)))
		;

	// check if DMA is complete (STR=0)
	while (*D1_CHCR & 0x0100)
		;

	*D1_CHCR = prev_chcr;

	asm __volatile__(" sync.l\n");

	*VIF1_STAT = 0;
	*GS_BUSDIR = (u64)0;
	// Put back prew imr and set finish event

	GsPutIMR(prev_imr);
	*GS_CSR = CSR_FINISH;
	// Enable path3 again
	*VIF1_FIFO = enable_path3.value;

	/*
		Write out to a file
	*/

	FILE *f = fopen(fname, "w+");
	if (f == NULL)
	{
		printf("??? couldn't open file\n");
		return 0;
	}
	printf("File should be size %d\n", (psm_size / 8) * width * height);
	fwrite(destBuffer, psm_size / 8, width * height, f);
	fclose(f);
	return 0;
}

qword_t *draw_texture_transfer_trx(qword_t *q, void *src, int width, int height, int psm, int dest, int dest_width, u32 custrx, u32 hoz_shift)
{
	int i;
	int remaining;
	int qwords = 0;

	switch (psm)
	{
	case GS_PSM_8:
	{
		qwords = (width * height) >> 4;
		break;
	}

	case GS_PSM_32:
	case GS_PSM_24:
	{
		qwords = (width * height) >> 2;
		break;
	}

	case GS_PSM_4:
	{
		qwords = (width * height) >> 5;
		break;
	}

	case GS_PSM_16:
	case GS_PSM_16S:
	{
		qwords = (width * height) >> 3;
		break;
	}

	default:
	{
		switch (psm)
		{
		case GS_PSM_8H:
		{
			qwords = (width * height) >> 4;
			break;
		}

		case GS_PSMZ_32:
		case GS_PSMZ_24:
		{
			qwords = (width * height) >> 2;
			break;
		}

		case GS_PSMZ_16:
		case GS_PSMZ_16S:
		{
			qwords = (width * height) >> 3;
			break;
		}

		case GS_PSM_4HL:
		case GS_PSM_4HH:
		{
			qwords = (width * height) >> 5;
			break;
		}
		}
		break;
	}
	}

	// Determine number of iterations based on the number of qwords
	// that can be handled per dmatag
	i = qwords / GIF_BLOCK_SIZE;

	// Now calculate the remaining image data left over
	remaining = qwords % GIF_BLOCK_SIZE;

	// Setup the transfer
	DMATAG_CNT(q, 5, 0, 0, 0);
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, dest >> 6, dest_width >> 6, psm), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, hoz_shift, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(width, height), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(custrx), GS_REG_TRXDIR);
	q++;

	while (i-- > 0)
	{

		// Setup image data dma chain
		DMATAG_CNT(q, 1, 0, 0, 0);
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(GIF_BLOCK_SIZE, 0, 0, 0, 2, 0), 0);
		q++;
		DMATAG_REF(q, GIF_BLOCK_SIZE, (unsigned int)src, 0, 0, 0);
		q++;

		// Now increment the address by the number of qwords in bytes
		src = (void *)src + (GIF_BLOCK_SIZE * 16);
	}

	if (remaining)
	{

		// Setup remaining image data dma chain
		DMATAG_CNT(q, 1, 0, 0, 0);
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(remaining, 0, 0, 0, 2, 0), 0);
		q++;
		DMATAG_REF(q, remaining, (unsigned int)src, 0, 0, 0);
		q++;
	}

	return q;
}