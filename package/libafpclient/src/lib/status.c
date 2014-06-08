#include <string.h>
#include <stdio.h>
#include "map_def.h"
#include "dsi.h"
#include "afp.h"

int afp_status_header(char * text, int * len) 
{
	int pos;
	memset(text,0,*len);

	pos=snprintf(text,*len,"AFPFS Version: %s\n"
		"UAMs compiled in: %s\n",
		AFPFS_VERSION,
		get_uam_names_list());
	*len-=pos;
	if (*len==0) return -1;
	return pos;
}

static void print_volume_status(struct afp_volume * v, 
	char * text,int * pos_p, int * len)
{
	struct afp_server * s = v->server;
	int pos = *pos_p;
	unsigned int fl = v->extra_flags;

	pos+=snprintf(text+pos,*len-pos,
		"    Volume %s, id %d, attribs 0x%x mounted: %s%s\n",
		v->volume_name_printable,v->volid,
		v->attributes,
		(v->mounted==AFP_VOLUME_MOUNTED) ? v->mountpoint:"No",
		((v->mounted==AFP_VOLUME_MOUNTED) && (volume_is_readonly(v))) ?
			" (read only)":"");

	if (v->mounted==AFP_VOLUME_MOUNTED) {
		pos+=snprintf(text+pos,*len-pos,
		"        did cache stats: %llu miss, %llu hit, %llu expired, %llu force removal\n        uid/gid mapping: %s (%d/%d)\n",
		v->did_cache_stats.misses, v->did_cache_stats.hits,
		v->did_cache_stats.expired, 
		v->did_cache_stats.force_removed,
		get_mapping_name(v),
		s->server_uid,s->server_gid);
		pos+=snprintf(text+pos,*len-pos,
		"        Unix permissions: %s",
			(v->extra_flags&VOLUME_EXTRA_FLAGS_VOL_SUPPORTS_UNIX)?
				"Yes":"No");

		if (s->server_type==AFPFS_SERVER_TYPE_NETATALK) {
			pos+=snprintf(text+pos,*len-pos,
			", Netatalk permissions broken: ");
			
			if (fl&VOLUME_EXTRA_FLAGS_VOL_CHMOD_KNOWN)
				if (fl&VOLUME_EXTRA_FLAGS_VOL_CHMOD_BROKEN)
					pos+=snprintf(text+pos,*len-pos,
						"Yes\n");
				else
					pos+=snprintf(text+pos,*len-pos,
						"No\n");
			else 
				pos+=snprintf(text+pos,*len-pos,
					"Unknown\n");
		}
	}
	*pos_p=pos;
}

int afp_status_server(struct afp_server * s, char * text, int * len) 
{
	int j;
	struct afp_volume *v;
	char signature_string[AFP_SIGNATURE_LEN*2+1];
	int pos=0;
	int firsttime=0;
	struct dsi_request * request;

	memset(text,0,*len);

	if (s==NULL) {
		pos+=snprintf(text+pos,*len-pos,
			"Not connected to any servers\n");
		goto out;
	}
		
	for (j=0;j<AFP_SIGNATURE_LEN;j++)
		sprintf(signature_string+(j*2),"%02x",
			(unsigned int) ((char) s->signature[j]));
	
	pos+=snprintf(text+pos,*len-pos,
		"Server %s\n"
		"    connection: %s:%d %s\n"
		"    using AFP version: %s\n",
		s->server_name_printable,
		inet_ntoa(s->address.sin_addr),ntohs(s->address.sin_port),
			(s->connect_state==SERVER_STATE_DISCONNECTED ? 
			"Disconnected" : "(active)"),
		s->using_version->av_name
	);

	pos+=snprintf(text+pos,*len-pos,
		"    server UAMs: ");

	for (j=1;j<0x100;j<<=1) {
		if (j & s->supported_uams) {
			if (firsttime!=0) 
				pos+=snprintf(text+pos,*len-pos,
					", ");
			if (j==s->using_uam)
				pos+=snprintf(text+pos,*len-pos,
					"%s (used)",
					uam_bitmap_to_string(j));
			else
				pos+=snprintf(text+pos,*len-pos,
					"%s",
					uam_bitmap_to_string(j));
			firsttime=1;
		}
	};

	pos+=snprintf(text+pos,*len-pos,
		"\n    login message: %s\n"
		"    type: %s",
		s->loginmesg, s->machine_type);


	pos+=snprintf(text+pos,*len-pos,
		"\n"
		"    signature: %s\n"
		"    transmit delay: %ums\n"
		"    quantums: %u(tx) %u(rx)\n"
		"    last request id: %d in queue: %llu\n",
	signature_string,
	s->tx_delay,
	s->tx_quantum, s->rx_quantum,
	s->lastrequestid,s->stats.requests_pending);

	for (request=s->command_requests;request;request=request->next) {
		pos+=snprintf(text+pos,*len-pos,
			"         request %d, %s\n",
			request->requestid, afp_get_command_name(request->subcommand)); 
	}

	pos+=snprintf(text+pos,*len-pos,
		"    transfer: %llu(rx) %llu(tx)\n"
		"    runt packets: %llu\n",
	s->stats.rx_bytes,s->stats.tx_bytes,
	s->stats.runt_packets);

	if (*len==0) goto out;

	for (j=0;j<s->num_volumes;j++) {
		v=&s->volumes[j];
		print_volume_status(v,text,&pos,len);
		pos+=snprintf(text+pos,*len-pos,"\n");
	}

out:
	*len-=pos;
	return pos;

}
