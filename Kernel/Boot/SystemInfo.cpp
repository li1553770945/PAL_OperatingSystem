#include <Boot/SystemInfo.h>
#include <Library/Kout.hpp>
using namespace POS;

void PrintSystemInfo()
{
	kout<<DarkGrayBG<<LightRed
		<<"PAL_OperatingSystem       "<<ResetBG<<endline
		<<DarkGrayBG<<LightYellow
		<<"Version 0.2               "<<ResetBG<<endline
		<<DarkGrayBG<<LightBlue
		<<"By: qianpinyi&&peaceSheep "<<ResetBG<<endl;
	kout<<LightCyan<<R"(
______  ___   _      _____                      _   _             _____           _                 
| ___ \/ _ \ | |    |  _  |                    | | (_)           /  ___|         | |                
| |_/ / /_\ \| |    | | | |_ __   ___ _ __ __ _| |_ _ _ __   __ _\ `--. _   _ ___| |_ ___ _ __ ___  
|  __/|  _  || |    | | | | '_ \ / _ \ '__/ _` | __| | '_ \ / _` |`--. \ | | / __| __/ _ \ '_ ` _ \ 
| |   | | | || |____\ \_/ / |_) |  __/ | | (_| | |_| | | | | (_| /\__/ / |_| \__ \ ||  __/ | | | | |
\_|   \_| |_/\_____/ \___/| .__/ \___|_|  \__,_|\__|_|_| |_|\__, \____/ \__, |___/\__\___|_| |_| |_|
                 ______   | |                                __/ |       __/ |                      
                |______|  |_|                               |___/       |___/                       
)"<<endl;
	kout<<endl;
}
