
#ifndef OSDefine_H
#define OSDefine_H

//#define AME_MAC
#define AME_Linux
//#define AME_Windows

/* Enable AME_eeprom_update Support */
#define Enable_AME_eeprom_update

/* Enable AME Software RAID Support */
#define Soft_Raid_Target 4
//#define Enable_AME_RAID_R0
//#define Enable_AME_RAID_R1

/* Disable MPIO Support */
#ifndef Enable_AME_RAID_R0
    #define Disable_MPIO
#endif

#ifdef AME_MAC
    #define TB_Func_support // Mac 10.9 
#endif

#ifdef AME_Windows
    //#define Win_spinlock
    //#define Support_LogMemory //LogMemory only support windows 
#endif

#define Enable_AME_message
//#define Enable_MPIO_message

#endif //OSDefine_H

