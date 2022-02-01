/*****************************************************************************\
*									      *
*   File name:	    smbios_defs.c					      *
*									      *
*   Description:    Standard SMBIOS tables definitions			      *
*									      *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2012-01-05 JFL Initial implementation.				      *
*    2014-01-03 JFL Minor updates to type 17 fields.			      *
*    2016-03-29 JFL Added type 2, 3, 22 fields, and updated type 17 fields.   *
*    2016-07-04 JFL Renamed dmistd.c as smbios_defs.c.                        *
*    2022-02-01 JFL Added table 43 name.		                      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

char *szDmi2TableTypes[] =
  {
  "BIOS Information",					/* Type 0 */
  "System Information",                                 /* Type 1 */
  "Base Board (or Module) Information",                 /* Type 2 */
  "System Enclosure or Chassis",                        /* Type 3 */
  "Processor Information",                              /* Type 4 */
  "Memory Controller Information",                      /* Type 5 */
  "Memory Module Information",                          /* Type 6 */
  "Cache Information",                                  /* Type 7 */
  "Port Connector Information",                         /* Type 8 */
  "System Slots",                                       /* Type 9 */
  "On Board Devices Information",                       /* Type 10 */
  "OEM Strings",                                        /* Type 11 */
  "System Configuration Options",                       /* Type 12 */
  "BIOS Language Information",                          /* Type 13 */
  "Group Associations",                                 /* Type 14 */
  "System Event Log",                                   /* Type 15 */
  "Physical Memory Array",                              /* Type 16 */
  "Memory Device",                                      /* Type 17 */
  "32-bit Memory Error Information",                    /* Type 18 */
  "Memory Array Mapped Address",                        /* Type 19 */
  "Memory Device Mapped Address",                       /* Type 20 */
  "Built-in Pointing Device",                           /* Type 21 */
  "Portable Battery",                                   /* Type 22 */
  "System Reset",                                       /* Type 23 */
  "Hardware Security",                                  /* Type 24 */
  "System Power Controls",                              /* Type 25 */
  "Voltage Probe",                                      /* Type 26 */
  "Cooling Device",                                     /* Type 27 */
  "Temperature Probe",                                  /* Type 28 */
  "Electrical Current Probe",                           /* Type 29 */
  "Out-of-Band Remote Access",                          /* Type 30 */
  "Boot Integrity Services (BIS) Entry Point",          /* Type 31 */
  "System Boot Information",                            /* Type 32 */
  "64-bit Memory Error Information",                    /* Type 33 */
  "Management Device",                                  /* Type 34 */
  "Management Device Component",                        /* Type 35 */
  "Management Device Threshold Data",                   /* Type 36 */
  "Memory Channel",                                     /* Type 37 */
  "IPMI Device Information",                            /* Type 38 */
  "System Power Supply",                                /* Type 39 */
  "Additional Information",                             /* Type 40 */
  "Onboard Devices Extended Information",               /* Type 41 */
  "Management Controller Host Interface",               /* Type 42 */
  "TPM Device",       				        /* Type 43 */
  };
#define NDMI2TABLETYPES (sizeof(szDmi2TableTypes) / sizeof(char *))

/* -------------- Structure type 0 fields (BIOS Information) --------------- */

char *szBiosCharacteristics[] = {
  "Reserved",
  "Reserved",
  "Unknown",
  "BIOS Characteristics Not Supported",
  "ISA is supported",
  "MCA is supported",
  "EISA is supported",
  "PCI is supported",
  "PC Card (PCMCIA) is supported",
  "Plug and Play is supported",
  "APM is supported",
  "BIOS is Upgradeable (Flash)",
  "BIOS shadowing is allowed",
  "VL-VESA is supported",
  "ESCD support is available",
  "Boot from CD is supported",
  "Selectable Boot is supported",
  "BIOS ROM is socketed",
  "Boot From PC Card (PCMCIA) is supported",
  "EDD (Enhanced Disk Drive) Specification is supported",
  "Int 13h - Japanese Floppy for NEC 9800 1.2mb (3.5\", 1k Bytes/Sector, 360 RPM) is supported",
  "Int 13h - Japanese Floppy for Toshiba 1.2mb (3.5\", 360 RPM) is supported",
  "Int 13h - 5.25\" / 360 KB Floppy Services are supported",
  "Int 13h - 5.25\" /1.2MB Floppy Services are supported",
  "Int 13h - 3.5\" / 720 KB Floppy Services are supported",
  "Int 13h - 3.5\" / 2.88 MB Floppy Services are supported",
  "Int 5h, Print Screen Service is supported",
  "Int 9h, 8042 Keyboard services are supported",
  "Int 14h, Serial Services are supported",
  "Int 17h, Printer Services are supported",
  "Int 10h, CGA/Mono Video Services are supported",
  "NEC PC-98",
};
#define NBIOSCHARACTERISTICS (sizeof(szBiosCharacteristics) / sizeof(char *))

char *szBiosFeaturesExt[16] = {
  "ACPI is supported",                                         /* Byte 0 Bit 0 */
  "USB Legacy is supported",                                   /* Byte 0 Bit 1 */
  "AGP is supported",                                          /* Byte 0 Bit 2 */
  "I2O boot is supported",                                     /* Byte 0 Bit 3 */
  "LS-120 SuperDisk boot is supported",                        /* Byte 0 Bit 4 */
  "ATAPI ZIP Drive boot is supported",                         /* Byte 0 Bit 5 */
  "1394 boot is supported",                                    /* Byte 0 Bit 6 */
  "Smart Battery is supported",                                /* Byte 0 Bit 7 */
  "BIOS Boot Specification is supported",                      /* Byte 1 Bit 0 */
  "Function key-initiated Network Service boot is supported",  /* Byte 1 Bit 1 */
  "Targeted Content Distribution Enabled",                     /* Byte 1 Bit 2 */
  "UEFI Specification is supported",                           /* Byte 1 Bit 3 */
  "The SMBIOS table describes a virtual machine",              /* Byte 1 Bit 4 */
  "Reserved",                                                  /* Byte 1 Bit 5 */
  "Reserved",                                                  /* Byte 1 Bit 6 */
  "Reserved",                                                  /* Byte 1 Bit 7 */
};

char *szProcSocketType[] = {
  "Undefined",
  "Other",
  "Unknown",
  "Daughter Board",
  "ZIF Socket",
  "Replaceable Piggy Back",
  "None",
  "LIF Socket",
  "Slot 1",
  "Slot 2",
  "370-pin socket",
  "Slot A",
  "Slot M",
  "Socket 423",
  "Socket A (Socket 462)",
  "Socket 478",
  "Socket 754",
  "Socket 940",
};
#define NPROCSOCKETTYPES (sizeof(szProcSocketType) / sizeof(char *))

/* ------------- Structure type 1 fields (System Information) -------------- */

char *szWakeUpType[] = {
  "Reserved",            /* 00h */
  "Other",               /* 01h */
  "Unknown",             /* 02h */
  "APM Timer",           /* 03h */
  "Modem Ring",          /* 04h */
  "LAN Remote",          /* 05h */
  "Power Switch",        /* 06h */
  "PCI PME#",            /* 07h */
  "AC Power Restored",   /* 08h */
};
#define NWAKEUPTYPES (sizeof(szWakeUpType) / sizeof(char *))

/* ----------- Structure type 2 fields (Motherboard Information) ----------- */

char *szBaseBoardFlags[] =
  {
  "Motherboard",
  "Daughter board required",
  "Removable",
  "Replaceable",
  "Hot swappable"
  };
#define NBASEBOARDFLAGS (sizeof(szBaseBoardFlags) / sizeof(char *))  

char *szBaseBoardType[] =
  {
  "Other, to be added to the list",
  "Unknown",
  "Other",
  "Server Blade",
  "Connectivity Switch",
  "System Management Module",
  "Processor Module",
  "I/O Module",
  "Memory Module",
  "Daughter board",
  "Motherboard (includes processor, memory, and I/O)"
  "Processor/Memory Module",
  "Processor/IO Module",
  "Interconnect board"
  };
#define NBASEBOARDTYPES (sizeof(szBaseBoardType) / sizeof(char *))

/* ------------- Structure type 3 fields (Chassis Information) ------------- */

char *szChassisType[] =
  {
  "Other, to be added to the list",
  "Other",
  "Unknown",
  "Desktop",
  "Low Profile Desktop",
  "Pizza Box",
  "Mini Tower",
  "Tower",
  "Portable",
  "Laptop",
  "Notebook",
  "Hand Held",
  "Docking Station",
  "All in One",
  "Sub Notebook",
  "Space-saving",
  "Lunch Box",
  "Main Server Chassis",
  "Expansion Chassis",
  "SubChassis",
  "Bus Expansion Chassis",
  "Peripheral Chassis",
  "RAID Chassis",
  "Rack Mount Chassis",
  "Sealed-case PC",
  "Multi-system chassis",
  "Compact PCI",
  "Advanced TCA",
  "Blade",
  "Blade Enclosure",
  "Tablet",
  "Convertible",
  "Detachable"
  };
#define NCHASSISTYPES (sizeof(szChassisType) / sizeof(char *))

/* ------------ Structure type 4 fields (Processor Information) ------------ */

/* -------- Structure type 5 fields (Memory Controller Information) -------- */

char *szMemType[] =
  {
  "Other, to be added to the list",
  "Unknown",
  "Standard",
  "Fast Page Mode",
  "EDO",
  "Parity",
  "ECC",
  "SIMM",
  "DIMM",
  "Burst EDO",
  "SDRAM",
  };
#define NMEMTYPES (sizeof(szMemType) / sizeof(char *))

char *szMemSpeed[] = 
  {
  "Other, to be added to the list",
  "Unknown",
  "70ns",
  "60ns",
  "50ns",
  };
#define NMEMSPEEDS (sizeof(szMemSpeed) / sizeof(char *))
    
char *szMemVolt[] = 
  {
  "5V",
  "3.3V",
  "2.9V",
  };
#define NMEMVOLTS (sizeof(szMemVolt) / sizeof(char *))

/* ---------- Structure type 6 fields (Memory Module Information) ---------- */

/* -------------- Structure type 7 fields (Cache Information) -------------- */

/* ---------- Structure type 8 fields (Port Connector Information) --------- */

char *szConnType[] = 
  {
  "Unknown",					/* 00h */
  "Centronics",                                 /* 01h */
  "Mini Centronics",                            /* 02h */
  "Proprietary",                                /* 03h */
  "DB-25 pin male",                             /* 04h */
  "DB-25 pin female",	                        /* 05h */
  "DB-15 pin male",                             /* 06h */
  "DB-15 pin female",                           /* 07h */
  "DB-9 pin male",                              /* 08h */
  "DB-9 pin female",                            /* 09h */
  "RJ-11",                                      /* 0Ah */
  "RJ-45",                                      /* 0Bh */
  "50 Pin MiniSCSI",                            /* 0Ch */
  "Mini-DIN",                                   /* 0Dh */
  "Micro-DIN",                                  /* 0Eh */
  "PS/2",                                       /* 0Fh */
  "Infrared",                                   /* 10h */
  "HP-HIL",                                     /* 11h */
  "Access Bus (USB)",                           /* 12h */
  "SSA SCSI",                                   /* 13h */
  "Circular DIN-8 male",                        /* 14h */
  "Circular DIN-8 female",                      /* 15h */
  "On Board IDE",                               /* 16h */
  "On Board Floppy",                            /* 17h */
  "9 Pin Dual Inline (pin 10 cut)",             /* 18h */
  "25 Pin Dual Inline (pin 26 cut)",            /* 19h */
  "50 Pin Dual Inline",                         /* 1Ah */
  "68 Pin Dual Inline",                         /* 1Bh */
  "On Board Sound Input from CD-ROM",           /* 1Ch */
  "Mini-Centronics Type-14",                    /* 1Dh */
  "Mini-Centronics Type-26",                    /* 1Eh */
  "Mini-jack (headphones)",                     /* 1Fh */
  "BNC",                                        /* 20h */
  "1394",                                       /* 21h */
  "SAS/SATA Plug Receptacle",                   /* 22h */
  };
#define NCONNTYPES (sizeof(szConnType) / sizeof(char *))

char *szPortType[] = 
  {
  "Unknown",					/* 00h */
  "Parallel Port XT/AT Compatible",             /* 01h */
  "Parallel Port PS/2",                         /* 02h */
  "Parallel Port ECP",                          /* 03h */
  "Parallel Port EPP",                          /* 04h */
  "Serial Port XT/AT Compatible",               /* 05h */
  "Serial Port 16450 Compatible",               /* 06h */
  "Serial Port 16550 Compatible",               /* 07h */
  "Serial Port 16550A Compatible",              /* 08h */
  "SCSI Port",                                  /* 09h */
  "MIDI Port",                                  /* 0Ah */
  "Joy Stick Port",                             /* 0Bh */
  "Access Bus Port",                            /* 0Ch */
  "Keyboard Port",                              /* 0Dh */
  "Mouse Port",                                 /* 0Eh */
  "SSA SCSI",                                   /* 0Fh */
  "USB",                                        /* 10h */
  "FireWire (IEEE P1394)",                      /* 11h */
  "PCMCIA Type II",                             /* 12h */
  "PCMCIA Type II",                             /* 13h */
  "PCMCIA Type III",                            /* 14h */
  "Cardbus",                                    /* 15h */
  "Access Bus Port",                            /* 16h */
  "SCSI II",                                    /* 17h */
  "SCSI Wide",                                  /* 18h */
  "PC-98",                                      /* 19h */
  "PC-98-Hireso",                               /* 1Ah */
  "PC-H98",                                     /* 1Bh */
  "Video Port",                                 /* 1Ch */
  "Audio Port",                                 /* 1Dh */
  "Modem Port",                                 /* 1Eh */
  "Network Port",                               /* 1Fh */
  "SATA",                                       /* 20H */
  "SAS",                                        /* 21H */
  };
#define NPORTTYPES (sizeof(szPortType) / sizeof(char *))

/* ----------------- Structure type 9 fields (System Slots) ---------------- */

char *szSlotType[] = {
  "Unknown",
  "ISA",
  "MCA",
  "EISA",
  "PCI",
  "PC CARD (PCMCIA)",
  "VL-VESA",
};
#define NSLOTTYPES (sizeof(szSlotType) / sizeof(char *))

char *szSlotTypeEx[] = {
  "Undefined",
  "Other",
  "Unknown",
  "ISA",
  "MCA",
  "EISA",
  "PCI",
  "PC CARD (PCMCIA)",
  "VL-VESA",
  "Proprietary",
  "Processor Card Slot",
  "Proprietary Memory Card Slot",
  "I/O Riser Card Slot",
  "NuBus",
  "PCI - 66MHz Capable",
  "AGP",
  "AGP 2X",
  "AGP 4X",
  "PCI-X",
  "AGP 8X",
};
#define NSLOTTYPESEX (sizeof(szSlotTypeEx) / sizeof(char *))

char *szSlotTypeExA0[] = {
  "PC-98/C20",                  /* A0h */
  "PC-98/C24",                  /* A1h */
  "PC-98/E",                    /* A2h */
  "PC-98/Local Bus",            /* A3h */
  "PC-98/Card",                 /* A4h */
  "PCI Express",                /* A5h */
  "PCI Express x1",             /* A6h */
  "PCI Express x2",             /* A7h */
  "PCI Express x4",             /* A8h */
  "PCI Express x8",             /* A9h */
  "PCI Express x16",            /* AAh */
  "PCI Express Gen 2",          /* ABh */
  "PCI Express Gen 2 x1",       /* ACh */
  "PCI Express Gen 2 x2",       /* ADh */
  "PCI Express Gen 2 x4",       /* AEh */
  "PCI Express Gen 2 x8",       /* AFh */
  "PCI Express Gen 2 x16",      /* B0h */
};

char *szSlotUse[] =
  {
  "Undefined",
  "Other",
  "Unknown",
  "Available",
  "In use",
  };
#define NSLOTUSES (sizeof(szSlotUse) / sizeof(char *))

/* ---------- Structure type 16 fields (Memory Array Information) ---------- */

char *szMemoryArrayLocations[] = {
  "Undefined",                      /* 00h */
  "Other",                          /* 01h */
  "Unknown",                        /* 02h */
  "System board or motherboard",    /* 03h */
  "ISA add-on card",                /* 04h */
  "EISA add-on card",               /* 05h */
  "PCI add-on card",                /* 06h */
  "MCA add-on card",                /* 07h */
  "PCMCIA add-on card",             /* 08h */
  "Proprietary add-on card",        /* 09h */
  "NuBus",                          /* 0Ah */
};
#define NMEMORYARRAYLOCATIONS (sizeof(szMemoryArrayLocations) / sizeof(char *))

char *szMemoryArrayUses[] = {
  "Undefined",                      /* 00h */
  "Other",                          /* 01h */
  "Unknown",                        /* 02h */
  "System memory",                  /* 03h */
  "Video memory",                   /* 04h */
  "Flash memory",                   /* 05h */
  "Non-volatile RAM",               /* 06h */
  "Cache memory",                   /* 07h */
};
#define NMEMORYARRAYUSES (sizeof(szMemoryArrayUses) / sizeof(char *))

char *szMemoryArrayEccTypes[] = {
  "Undefined",                      /* 00h */
  "Other",                          /* 01h */
  "Unknown",                        /* 02h */
  "None",                           /* 03h */
  "Parity",                         /* 04h */
  "Single-bit ECC",                 /* 05h */
  "Multi-bit ECC",                  /* 06h */
  "CRC",                            /* 07h */
};
#define NMEMORYARRAYECCTYPES (sizeof(szMemoryArrayEccTypes) / sizeof(char *))

/* ---------- Structure type 17 fields (Memory Module Information) --------- */

char *szMemoryDeviceFormFactors[] = {
  "Undefined",           /* 00h */
  "Other",               /* 01h */
  "Unknown",             /* 02h */
  "SIMM",                /* 03h */
  "SIP",                 /* 04h */
  "Chip",                /* 05h */
  "DIP",                 /* 06h */
  "ZIP",                 /* 07h */
  "Proprietary Card",    /* 08h */
  "DIMM",                /* 09h */
  "TSOP",                /* 0Ah */
  "Row of chips",        /* 0Bh */
  "RIMM",                /* 0Ch */
  "SODIMM",              /* 0Dh */
  "SRIMM",               /* 0Eh */
  "FB-DIMM",             /* 0Fh */
};
#define NMEMORYDEVICEFORMFACTORS (sizeof(szMemoryDeviceFormFactors) / sizeof(char *))

char *szMemoryDeviceTypes[] = {
  "Undefined",       /* 00h */
  "Other",	     /* 01h */
  "Unknown",         /* 02h */
  "DRAM",            /* 03h */
  "EDRAM",           /* 04h */
  "VRAM",            /* 05h */
  "SRAM",            /* 06h */
  "RAM",             /* 07h */
  "ROM",             /* 08h */
  "FLASH",           /* 09h */
  "EEPROM",          /* 0Ah */
  "FEPROM",          /* 0Bh */
  "EPROM",           /* 0Ch */
  "CDRAM",           /* 0Dh */
  "3DRAM",           /* 0Eh */
  "SDRAM",           /* 0Fh */
  "SGRAM",           /* 10h */
  "RDRAM",           /* 11h */
  "DDR",             /* 12h */
  "DDR2",            /* 13h */
  "DDR2 FB-DIMM",    /* 14h */
  "Reserved",        /* 15h */
  "Reserved",        /* 16h */
  "Reserved",        /* 17h */
  "DDR3",            /* 18h */
  "FBD2",            /* 19h */
  "DDR4",            /* 1Ah */
  "LPDDR",           /* 1Bh */
  "LPDDR2",          /* 1Ch */
  "LPDDR3",          /* 1Dh */
  "LPDDR4",          /* 1Eh */
};
#define NMEMORYDEVICETYPES (sizeof(szMemoryDeviceTypes) / sizeof(char *))

char *szMemoryTypeDetails[] = {
  "Reserved, set to 0",              /* Bit 0  */
  "Other",                           /* Bit 1  */
  "Unknown",                         /* Bit 2  */
  "Fast-paged",                      /* Bit 3  */
  "Static column",                   /* Bit 4  */
  "Pseudo-static",                   /* Bit 5  */
  "RAMBUS",                          /* Bit 6  */
  "Synchronous",                     /* Bit 7  */
  "CMOS",                            /* Bit 8  */
  "EDO",                             /* Bit 9  */
  "Window DRAM",                     /* Bit 10 */
  "Cache DRAM",                      /* Bit 11 */
  "Non-volatile",                    /* Bit 12 */
  "Registered (Buffered)",           /* Bit 13 */
  "Unbuffered (Unregistered)",       /* Bit 14 */
  "LRDIMM",                          /* Bit 15 */
};

/* -------- Structure type 22 fields (Portable Battery Information) -------- */

char *szBatteryChemistry[] =
  {
  "Other, to be added to the list",
  "Other",
  "Unknown",
  "Lead Acid",
  "Nickel Cadmium",
  "Nickel metal hydride",
  "Lithium-ion",
  "Zinc air",
  "Lithium Polymer"
  };
#define NBATTERYCHEMISTRIES (sizeof(szBatteryChemistry) / sizeof(char *))  

