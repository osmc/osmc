
/* AFP function names */
static char * afp_command_names[] = {
"Zero", "afpByteRangeLock", "afpCloseVol", "afpCloseDir", "afpCloseFork",
"afpCopyFile", "afpCreateDir", "afpCreateFile", "afpDelete", 
"afpEnumerate", "afpFlush", "afpFlushFork", "Unknown", /* 12 */
"Unknown", /* 13 */ "afpGetForkParms", "afpGetSrvrInfo", "afpGetSrvrParms",
"afpGetVolParms", "afpLogin", "afpLoginCont", "afpLogout", 
"afpMapID", "afpMapName", "afpMoveAndRename", "afpOpenVol", "afpOpenDir",
"afpOpenFork", "afpRead", "afpRename", "afpSetDirParms", "afpSetFileParms",
"afpSetForkParms", "afpSetVolParms", "afpWrite", "afpGetFileDirParms",
"afpSetFileDirParms", "afpChangePassword", "afpGetUserInfo", "afpGetSrvrMsg", /* 38 */
"Unknown 39", "Unknown 40", "Unknown 41", "Unknown 42", "Unknown 43", "Unknown 44",
"Unknown 45", "Unknown 46", "Unknown 47", "afpOpenDT", "Unknown 49", "Unknown 50",
"afpGetIcon", "afpGetIconInfo", "Unknown 53", "Unknown 54", "Unknown 55", "afpAddComment", 
"afpRemoveComment", "afpGetComment", "afpByteRangeLockExt", "afpReadExt", "afpWriteExt",
"afpGetAuthMethods", "afp_LoginExt", "afpGetSessionToken", "afpDisconnectOldSession",
"afpEnumerateExt", "afpCatSearchExt", "afpEnumerateExt2", "afpGetExtAttr", 
"afpSetExtAttr", "afpRemoveExtAttr" , "afpListExtAttrs", /* 74 */
"Some AFP 3.2 undocumented feature",
"Unknown 76","Unknown 77","Unknown 78","Unknown 79","Unknown 80","Unknown 81",
"Unknown 82","Unknown 83","Unknown 84","Unknown 85","Unknown 86","Unknown 87",
"Unknown 88","Unknown 89","Unknown 90","Unknown 91","Unknown 92","Unknown 93",
"Unknown 94","Unknown 95","Unknown 96","Unknown 97","Unknown 98","Unknown 99",
"Unknown 100","Unknown 101","Unknown 102","Unknown 103","Unknown 104",
"Unknown 105","Unknown 106","Unknown 107","Unknown 108","Unknown 109",
"Unknown 110","Unknown 111","Unknown 112","Unknown 113","Unknown 114",
"Unknown 115","Unknown 116","Unknown 117","Unknown 118","Unknown 119",
"Unknown 120","Unknown 121", "afpZzzzz", /* 122 */
"Unknown 123","Unknown 124",
"Unknown 125","Unknown 126","Unknown 127","Unknown 128","Unknown 129",
"Unknown 130","Unknown 131","Unknown 132","Unknown 133","Unknown 134",
"Unknown 135","Unknown 136","Unknown 137","Unknown 138","Unknown 139",
"Unknown 140","Unknown 141","Unknown 142","Unknown 143","Unknown 144",
"Unknown 145","Unknown 146","Unknown 147","Unknown 148","Unknown 149",
"Unknown 150","Unknown 151","Unknown 152","Unknown 153","Unknown 154",
"Unknown 155","Unknown 156","Unknown 157","Unknown 158","Unknown 159",
"Unknown 160","Unknown 161","Unknown 162","Unknown 163","Unknown 164",
"Unknown 165","Unknown 166","Unknown 167","Unknown 168","Unknown 169",
"Unknown 170","Unknown 171","Unknown 172","Unknown 173","Unknown 174",
"Unknown 175","Unknown 176","Unknown 177","Unknown 178","Unknown 179",
"Unknown 180","Unknown 181","Unknown 182","Unknown 183","Unknown 184",
"Unknown 185","Unknown 186","Unknown 187","Unknown 188","Unknown 189",
"Unknown 190","Unknown 191", "afpAddIcon", "Unknown 193","Unknown 194",
"Unknown 195","Unknown 196","Unknown 197","Unknown 198","Unknown 199",
"Unknown 200","Unknown 201","Unknown 202","Unknown 203","Unknown 204",
"Unknown 205","Unknown 206","Unknown 207","Unknown 208","Unknown 209",
"Unknown 210","Unknown 211","Unknown 212","Unknown 213","Unknown 214",
"Unknown 215","Unknown 216","Unknown 217","Unknown 218","Unknown 219",
"Unknown 220","Unknown 221","Unknown 222","Unknown 223","Unknown 224",
"Unknown 225","Unknown 226","Unknown 227","Unknown 228","Unknown 229",
"Unknown 230","Unknown 231","Unknown 232","Unknown 233","Unknown 234",
"Unknown 235","Unknown 236","Unknown 237","Unknown 238","Unknown 239",
"Unknown 240","Unknown 241","Unknown 242","Unknown 243","Unknown 244",
"Unknown 245","Unknown 246","Unknown 247","Unknown 248","Unknown 249",
"Unknown 250","Unknown 251","Unknown 252","Unknown 253","Unknown 254",
"Unknown 255"
};

char * afp_get_command_name(char code)
{
        return afp_command_names[code];


}

