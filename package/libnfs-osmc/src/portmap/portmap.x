/*
 * From RFC1833
 */

const PMAP_PORT = 111;      /* portmapper port number */

struct pmap_mapping {
       unsigned int prog;
       unsigned int vers;
       unsigned int prot;
       unsigned int port;
};

struct pmap_call_args {
       unsigned int prog;
       unsigned int vers;
       unsigned int proc;
       opaque args<>;
};

struct pmap_call_result {
	unsigned int port;
	opaque res<>;
};

program PMAP_PROGRAM {
	version PMAP_V2 {
        	void
		PMAP_NULL(void)              = 0;

		bool
            	PMAP_SET(pmap_mapping)       = 1;

            	bool
            	PMAP_UNSET(pmap_mapping)     = 2;

            	unsigned int
            	PMAP_GETPORT(pmap_mapping)   = 3;

		pmap_call_result
		PMAP_CALLIT(pmap_call_args)  = 5;
	} = 2;
} = 100000;

