/*****************************************************************
|
|      Neptune - System Log Config
|
|      (c) 2001-2008 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#import <Foundation/Foundation.h>

#import "NptLogging.h"

NPT_Result
NPT_GetSystemLogConfig(NPT_String& config)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
#if !TARGET_OS_IPHONE
    NSDictionary* env_vars = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"LSEnvironment"];
    NSString* npt_log_config = [env_vars objectForKey:@"NEPTUNE_LOG_CONFIG"];
#else
    NSString *npt_log_config = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NEPTUNE_LOG_CONFIG"];
#endif
    
    NPT_Result result = NPT_SUCCESS;
	if (npt_log_config) {
		NSLog(@"NEPTUNE_LOG_CONFIG in plist is: %@", npt_log_config);
		config = (const char*)[npt_log_config UTF8String];
	} else {
		NSLog(@"NEPTUNE_LOG_CONFIG not found in 'Info.plist'");
        result = NPT_ERROR_NO_SUCH_PROPERTY;
    }
    [pool release];
    return result;
}
