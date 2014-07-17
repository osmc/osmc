/* implementation based on wireshark c-code */

const RQUOTAPATHLEN = 1024; /* Guess this is max. It is max for mount so probably rquota too */

enum rquotastat {
     RQUOTA_OK		= 1,
     RQUOTA_NOQUOTA	= 2,
     RQUOTA_EPERM	= 3
};

typedef string exportpath<RQUOTAPATHLEN>;

struct GETQUOTA1args {
       exportpath export;
       int uid;
};

enum quotatype {
     RQUOTA_TYPE_UID = 0,
     RQUOTA_TYPE_GID = 1
};

struct GETQUOTA2args {
       exportpath export;
       quotatype type;
       int uid;
};

struct GETQUOTA1res_ok {
       int bsize;
       int active;
       int bhardlimit;
       int bsoftlimit;
       int curblocks;
       int fhardlimit;
       int fsoftlimit;
       int curfiles;
       int btimeleft;
       int ftimeleft;
};

union GETQUOTA1res switch (rquotastat status) {
      case RQUOTA_OK:
            GETQUOTA1res_ok quota;
      default:
            void;
};

program RQUOTA_PROGRAM {
	version RQUOTA_V1 {
		void
		RQUOTA1_NULL(void)                 = 0;

		GETQUOTA1res
		RQUOTA1_GETQUOTA(GETQUOTA1args)    = 1;

		GETQUOTA1res
		RQUOTA1_GETACTIVEQUOTA(GETQUOTA1args)    = 2;
	} = 1;

	version RQUOTA_V2 {
		void
		RQUOTA2_NULL(void)                 = 0;

		GETQUOTA1res
		RQUOTA2_GETQUOTA(GETQUOTA2args)    = 1;

		GETQUOTA1res
		RQUOTA2_GETACTIVEQUOTA(GETQUOTA2args)    = 2;
	} = 2;
} = 100011;

