#pragma once

#define IOCTL_LOAD_FILTER       CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x01, 0, FILE_WRITE_DATA) //88004
#define IOCTL_UNLOAD_FILTER     CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x02, 0, FILE_WRITE_DATA) //88008
#define IOCTL_INIT_FIND         CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x03, 0, FILE_READ_DATA)  //8400C

//84024 find first
//84028 find next


typedef struct _FILTER_NAME
{
    USHORT Length;
    WCHAR FilterName[1];

} FILTER_NAME, *PFILTER_NAME;
