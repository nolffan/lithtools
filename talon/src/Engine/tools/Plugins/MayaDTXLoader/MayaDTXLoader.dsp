# Microsoft Developer Studio Project File - Name="MayaDTXLoader" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MayaDTXLoader - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MayaDTXLoader.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MayaDTXLoader.mak" CFG="MayaDTXLoader - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MayaDTXLoader - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# talon: IMFLithDTX.dll, the Maya image-format plug-in for .dtx textures
# (AvP2 Tools Update 2, built 2002-03-28). Written for this project from
# Build 69's MayaDTXLoader.vcproj: the shipped image imports MFC42.DLL (MFC
# as a shared DLL, where Build 69's project uses none) and links no
# TdGuard.lib (Touchdown's, later); VC6 SP5's compiler (8966) and linker.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MayaDTXLoader - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\built\Release\tools\Plugins\MayaDTXLoader"
# PROP Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MayaDTXLoader"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W2 /GX /O2 /Ob1 /I "..\..\shared\engine" /I "..\..\..\..\libs\stdlith" /I "..\..\..\sdk\inc" /I "..\..\preprocessor" /I "..\..\shared\world" /I "..\..\dedit\misc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PLUGIN_DLL" /D "BDEFS_WINDOWS_H" /D "_WINDLL" /D "_AFXDLL" /GF /Gy /FD /c
# ADD CPP /nologo /MD /W2 /GX /O2 /Ob1 /I "..\..\shared\engine" /I "..\..\..\..\libs\stdlith" /I "..\..\..\sdk\inc" /I "..\..\preprocessor" /I "..\..\shared\world" /I "..\..\dedit\misc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PLUGIN_DLL" /D "BDEFS_WINDOWS_H" /D "_WINDLL" /D "_AFXDLL" /GF /Gy /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\..\built\Release\tools\Plugins\MayaDTXLoader\IMFLithDTX.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\..\built\Release\tools\Plugins\MayaDTXLoader\IMFLithDTX.dll"

!ENDIF 

# Begin Target

# Name "MayaDTXLoader - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\shared\engine\dtxmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\genltstream.cpp
# End Source File
# Begin Source File

SOURCE=.\MayaDTXLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\pixelformat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\streamsim.cpp
# End Source File
# Begin Source File

SOURCE=.\IMF.lib
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\image.h
# End Source File
# End Group
# End Target
# End Project
