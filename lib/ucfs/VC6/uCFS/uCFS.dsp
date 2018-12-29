# Microsoft Developer Studio Project File - Name="uCFS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=uCFS - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "uCFS.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "uCFS.mak" CFG="uCFS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "uCFS - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "uCFS - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/sw/ISP_PROJ/ISP_USE_RAW/fpga/ucFS/VC6/uCFS", IYCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uCFS - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "PCVER" /D _WIN32_WINNT=0x0500 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "uCFS - Win32 Release"
# Name "uCFS - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "simulate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simulate\win32_fs_os.cpp
# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\simulate\win32_heap_simulate.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\simulate\win32_kernel_simulate.cpp

!IF  "$(CFG)" == "uCFS - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\include"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simulate\win32_rtos.cpp

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\rtos" /I "..\include"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\uCFS.cpp
# End Source File
# Begin Source File

SOURCE=.\uCFS.rc
# End Source File
# Begin Source File

SOURCE=.\uCFSDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\uCFSView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\uCFS.h
# End Source File
# Begin Source File

SOURCE=.\uCFSDoc.h
# End Source File
# Begin Source File

SOURCE=.\uCFSView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\uCFS.ico
# End Source File
# Begin Source File

SOURCE=.\res\uCFS.rc2
# End Source File
# Begin Source File

SOURCE=.\res\uCFSDoc.ico
# End Source File
# End Group
# Begin Group "ucFS"

# PROP Default_Filter ""
# Begin Group "Config"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Config\ConfigDrive.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\include"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Config\FS_X_Panic.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\include"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "FS"

# PROP Default_Filter ""
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FS\Core\FS__ECC256.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_AddSpaceHex.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_AssignCache.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Attrib.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\rtos" /I "..\..\inc" /I "..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CacheAll.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CacheMan.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CacheRW.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CacheRWQuota.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CLib.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CopyFile.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Core.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_CreateDir.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Dir.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_DirOld.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_ErrorNo2Text.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_ErrorOut.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Format.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_FRead.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_FWrite.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_GetFilePos.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_GetFileSize.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_GetNumFiles.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_GetNumOpenFiles.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Journal.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Log.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_LogBlock.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_LogVolume.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_memcpy.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Misc.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Move.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_OS_Interface.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Partition.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Read.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Rename.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_SetEndOfFile.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_SetFilePos.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Storage.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Time.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Truncate.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Unmount.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Verify.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Volume.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Warn.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\FS_Write.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Core\ucFS_Version.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Driver"

# PROP Default_Filter ""
# Begin Group "SD_MMC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FS\Driver\SD_MMC\SD_MMC_Drive_PC.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc" /I "..\inc" /I "..\include"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\..\inc" /I "..\inc" /I "..\include" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\Driver\SD_MMC\SD_MMC_PC.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc" /I "..\inc" /I "..\include"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\..\inc" /I "..\inc" /I "..\include" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "FSL"

# PROP Default_Filter ""
# Begin Group "FAT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_API.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_CheckDisk.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Dir.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_DirEntry.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_DiskInfo.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Format.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_FormatSD.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_GetFileClusterSectorInfo.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Ioct.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_LFN.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Misc.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Move.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Open.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Read.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Rename.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_SetEndOfFile.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_VolumeLabel.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\FS\FSL\FAT\FAT_Write.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /W4 /I "..\..\inc" /I "..\inc" /I "..\rtos"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "Code"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FS\Code\Gbcode.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# End Group
# Begin Group "ucFS_inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Inc\FAT.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FAT_Intern.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\fs.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_API.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_CLib.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Conf.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_ConfDefaults.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Debug.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Dev.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_DF_X_HW.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Int.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Lbl.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_OS.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Port.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Storage.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\FS_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\Global.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\IDE_X_HW.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\MMC_SD_CardMode_X_HW.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\MMC_X_HW.h
# End Source File
# Begin Source File

SOURCE=..\..\Inc\ucFS_Version.h
# End Source File
# End Group
# Begin Group "inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\ark1960.h
# End Source File
# Begin Source File

SOURCE=..\inc\ark1960_testcase.h
# End Source File
# Begin Source File

SOURCE=..\inc\bits.h
# End Source File
# Begin Source File

SOURCE=..\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\inc\DmpMemory.h
# End Source File
# Begin Source File

SOURCE=..\inc\fevent.h
# End Source File
# Begin Source File

SOURCE=..\inc\fs_card.h
# End Source File
# Begin Source File

SOURCE=..\inc\gbcode.h
# End Source File
# Begin Source File

SOURCE=..\inc\printk.h
# End Source File
# Begin Source File

SOURCE=..\inc\sdmmc.h
# End Source File
# Begin Source File

SOURCE=..\inc\types.h
# End Source File
# Begin Source File

SOURCE=..\inc\xm_h264_cache_command_queue.h
# End Source File
# Begin Source File

SOURCE=..\inc\xm_h264_cache_file.h
# End Source File
# Begin Source File

SOURCE=..\inc\xm_h264_file.h
# End Source File
# Begin Source File

SOURCE=..\inc\xmcore.h
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\include\xmbase.h
# End Source File
# Begin Source File

SOURCE=..\include\xmfile.h
# End Source File
# Begin Source File

SOURCE=..\include\xmkey.h
# End Source File
# Begin Source File

SOURCE=..\include\xmlang.h
# End Source File
# Begin Source File

SOURCE=..\include\xmprintf.h
# End Source File
# Begin Source File

SOURCE=..\include\xmqueue.h
# End Source File
# Begin Source File

SOURCE=..\include\xmtype.h
# End Source File
# Begin Source File

SOURCE=..\include\xmuser.h
# End Source File
# Begin Source File

SOURCE=..\include\xmvideoitem.h
# End Source File
# End Group
# Begin Group "test"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\test\fs_sd_test.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\test\video_item_test.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "rtos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\message_task.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\rtos" /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /X /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\queue.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\target.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\rtos" /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\xm_h264_cache_command_queue.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\xm_h264_cache_file.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\xm_h264_file.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\xm_semaphore.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "middleware"

# PROP Default_Filter ""
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\middleware\common\common_chardef.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\common\common_chartype.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\common\common_rbtr.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\common\common_string.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\common\common_wstring.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\common\xmvideoitem.c

!IF  "$(CFG)" == "uCFS - Win32 Release"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "uCFS - Win32 Debug"

# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos\\"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "gdi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\middleware\gdi\xm_file.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\middleware\gdi\xm_kbd_drv.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\main\os_main.c
# ADD CPP /I "..\inc" /I "..\include" /I "..\..\inc" /I "..\rtos"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
