/*
 * init.c
 * 
 * (c) 2014 Sam Nazarko
 * email@samnazarko.co.uk
 */


#include <stdio.h>
#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include <blkid/blkid.h>

int parse_option(const char *line, const char *option, char *value, size_t size)                                                                              
{                                                                                                                                                             
    const char *p0, *p1;                                                                                                                                      
    int len;                                                                                                                                                  

    p0 = strstr(line, option);                                                                                                                                
    if (!p0)                                                                                                                                                  
        return 0;                                                                                                                                             
    p0 += strlen(option);                                                                                                                                     
    p1  = strchr(p0, ' ');                                                                                                                                    
    if (!p1)                                                                                                                                                  
       p1 = p0 + strlen(p0);                                                                                                                                  
    len = p1 - p0;                                                                                                                                            
    if (len > size - 1)                                                                                                                                       
        len = size - 1;                                                                                                                                       
    memcpy(value, p0, len);                                                                                                                                   
    value[len] = '\0';                                                                                                                                        
    return len;                                                                                                                                               
}

void get_cmdline_option(const char *option, char *value, size_t size)                                                                                         
{                                                                                                                                                             
    FILE  *fp;                                                                                                                                                
    char  *line = NULL;                                                                                                                                       
    size_t len = 0;                                                                                                                                           
    size_t read;                                                                                                                                              

    if (!size)                                                                                                                                                
        return;                                                                                                                                               
    *value = '\0';                                                                                                                                            
    fp = fopen("/proc/cmdline", "r");                                                                                                                         
    if (fp == NULL)                                                                                                                                           
         return;                                                                                                                                              
    while ((read = getline(&line, &len, fp)) != -1) {                                                                                                                                                                                                                                
        if (parse_option(line, option, value, size))                                                                                                          
            break;                                                                                                                                            
    }                                                                                                                                                         
    fclose(fp);                                                                                                                                           
    if (line)                                                                                                                                                 
        free(line);
    return;                                                                                                                                                   
}  

int main(int argc, char **argv)
{
	/* Mounts */
	mount("none", "/proc", "proc", 0, "");
	mount("none", "/sys", "sysfs", 0, "");
	
	/* Configure devices */
	char mdev_cmd[32];
	sprintf(mdev_cmd, "/sbin/mdev -s");
	system(mdev_cmd);
	
	/* BusyBox symlinks */
	char busybox_cmd[32];
	sprintf(busybox_cmd, "/bin/busybox --install -s");
	system(busybox_cmd);
	
	/* Clear rubbish */
	char clear_cmd[32]; 
	sprintf(clear_cmd, "/usr/bin/clear");
	system(clear_cmd);
	
	/* Splash screen */
	system("/usr/bin/splash /splash.png");
	
	/* Read /proc/cmdline */
	int boot_type = 0; /* 0: /dev/XYZ; 1: UUID label */
	char root_dir[32] = "/newroot";
	char root[128];                                                                                                                                           
        get_cmdline_option("root=", root, sizeof(root));
	char fstype[12];
	get_cmdline_option("rootfstype", fstype, sizeof(fstype));
	char root_delay[2];
	get_cmdline_option("rootdelay", root_delay, sizeof(root_delay));
	if (root_delay[0] !='\0')
		sleep(atoi(root_delay));
	
	/* Check for root= and fstype= */
	if (root[0] == '\0' || fstype[0] =='\0')
		goto fail;
		
	/* UUID labels */
	char uuid[128];
	char *dev; 
	get_cmdline_option("root=UUID=", uuid, sizeof(uuid));
	if (uuid[0] != '\0')
	{
		dev = blkid_evaluate_tag("UUID", uuid, NULL);
		boot_type = 1;
	}	

	/* Mount root filesystem */
	int res = 0;
	switch (boot_type)
	{
		case 0:
		res = mount(root, root_dir, fstype, 0, "");
		break;
		
		case 1:
		res = mount(dev, root_dir, fstype, 0, "");
		break; 
	}
	
	if (res != 0)
		goto fail;
	
	/* Change root and handover to init */
	chdir(root_dir);
	int result = chroot(root_dir);
	if (result != 0)
		goto fail;
	putenv("PWD=/");
	system("/sbin/init");
	
	return 0;
	
	/* Failure */
	fail:
		system("/usr/bin/splash /splash_sad.png");
		while (1)
		{
			/* Do not let /init die to prevent a further kernel panic */
			sleep(1);
		}
}

