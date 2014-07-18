
#ifndef __DSI_PROTOCOL_H_
#define __DSI_PROTOCOL_H_

/* These are DSI constants */

#define DSI_REQUEST 0x0
#define DSI_REPLY 0x1

#define DSI_DSICloseSession 1
#define DSI_DSICommand 2
#define DSI_DSIGetStatus 3
#define DSI_DSIOpenSession 4
#define DSI_DSITickle 5
#define DSI_DSIWrite 6
#define DSI_DSIAttention 8



struct dsi_header {
	uint8_t flags;
	uint8_t command;
	uint16_t requestid;
	union {
		int error_code;
		unsigned int data_offset;
	} return_code;
	uint32_t length;
	uint32_t reserved;
};

void dsi_setup_header(struct afp_server * server, struct dsi_header * header, char command);


#endif
