/* Kaypro IV / Kaypro 4 format, double-sided, 400k */

#include <stdio.h>
#include <stdint.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif
#include <usb.h>
#include <string.h>
#include "phys.h"
#include "fc5025.h"
#include "endec.h"
#include "crc.h"

static int min_track(struct phys *this) {
	return 0;
}

static int max_track(struct phys *this) {
	return 39;
}

static int min_side(struct phys *this) {
	return 0;
}

static int max_side(struct phys *this) {
	return 1;
}

static int min_sector(struct phys *this, int track, int side) {
	return side?10:0;
}

static int max_sector(struct phys *this, int track, int side) {
	return side?19:9;
}

static int sector_bytes(struct phys *this, int track, int side, int sector) {
	return 512;
}

static int read_sector(struct phys *this, unsigned char *out, int track, int side, int sector) {
	unsigned char id_field_raw[]={0xa1,0xa1,0xa1,0xfe,track,0,sector,2,0,0};
	unsigned int xferlen=772;
	int xferlen_out;
	unsigned char raw[773]={0xf9,0,}, plain[518]={0xa1,0xa1,0xa1,0,};
	struct {
		uint8_t opcode, flags, format;
		uint16_t bitcell;
		uint8_t sectorhole;
		uint8_t rdelayh; uint16_t rdelayl;
		uint8_t idam, id_pat[12], id_mask[12], dam[3];
	} __attribute__ ((__packed__)) cdb={OPCODE_READ_FLEXIBLE,READ_FLAG_ORUN_RECOV|side,FORMAT_MFM,htons(3333),0,0,0,0xfe,{0,},{0,},{0xf9,0,0}};
	int status=0;

	crc_append(id_field_raw,8);
	enc_mfm(cdb.id_pat,cdb.id_mask,id_field_raw+4,6);
	status|=fc_bulk_cdb(&cdb,sizeof(cdb),4000,NULL,raw+1,xferlen,&xferlen_out);
	if(xferlen_out!=xferlen)
		status|=1;
	dec_mfm(plain+3,raw,515);
	if(plain[3]!=0xfb) /* DAM */
		status|=1;
	if(crc_block(plain,518)!=0)
		status|=1;
	memcpy(out,plain+4,512);
	return status;
}

static void best_read_order(struct phys *this, struct sector_list *out, int track, int side) {
	struct sector_list side0[]={{0,0},{3,1536},{1,512},{4,2048},{2,1024},{8,4096},{6,3072},{9,4608},{7,3584},{5,2560}};
	struct sector_list side1[]={{10,0},{13,1536},{11,512},{14,2048},{12,1024},{18,4096},{16,3072},{19,4608},{17,3584},{15,2560}};

	if(side==0) {
		memcpy(out,side0,sizeof(side0));
	} else {
		memcpy(out,side1,sizeof(side1));
	}
}

struct phys phys_kaypro4={
	.min_track = min_track,
	.max_track = max_track,
	.num_tracks = phys_gen_num_tracks,
	.min_side = min_side,
	.max_side = max_side,
	.num_sides = phys_gen_num_sides,
	.min_sector = min_sector,
	.max_sector = max_sector,
	.num_sectors = phys_gen_num_sectors,
	.tpi = phys_gen_48tpi,
	.density = phys_gen_low_density,
	.sector_bytes = sector_bytes,
	.track_bytes = phys_gen_track_bytes,
	.physical_track = phys_gen_physical_track,
	.best_read_order = best_read_order,
	.read_sector = read_sector,
	.prepare = phys_gen_no_prepare
};
