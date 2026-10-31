# Microsoft Developer Studio Project File - Name="MaxDTXLoader30" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MaxDTXLoader30 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MaxDTXLoader30.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MaxDTXLoader30.mak" CFG="MaxDTXLoader30 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MaxDTXLoader30 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# talon: MaxDTXLoader30.bmi, the LithTech .dtx bitmap loader for 3ds Max 3 (AvP2 Tools
# talon: Update 2, built 2002-03-28), written for this project from Build 69's
# talon: MaxDTXLoader.vcproj, "Release MAX30": maxsdk-3.0's headers and libraries
# talon: (toolchains/maxsdk-3.0), /MD, /Ob1, no TdGuard.lib (Touchdown's, later);
# talon: VC6 SP5's compiler (8966) and linker.
# talon: As shipped (2002), not as Build 69 has it: no MFC block in MaxDTXLoader.h
# talon: (no _AFXDLL, no MFC42.DLL import). Build 69's defines are kept: with
# talon: BDEFS_MFC, bdefs.h brings in StdLith's stdafx.h, so the objects that
# talon: include it name static MFC (nafxcw.lib), whose nolib.obj is the Rich
# talon: header's one 8569 object; operator new is MSVCRT's, delete msvcprt's,
# talon: and pixelformat.obj's template instances land before its initializers,
# talon: as shipped. /YX (the link's layout needs it); no /GF (the literals are
# talon: in .data); no /G6. Lib_StdLith (l_allocator.obj) through Plugins.dsw.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MaxDTXLoader30 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\built\Release\tools\Plugins\MaxDTXLoader30"
# PROP BASE Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MaxDTXLoader30"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\built\Release\tools\Plugins\MaxDTXLoader30"
# PROP Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MaxDTXLoader30"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maxsdk-3.0\include" /I "..\..\shared\engine" /I "..\..\..\..\libs\stdlith" /I "..\..\..\sdk\inc" /I "..\..\preprocessor" /I "..\..\shared\world" /I "..\..\dedit\misc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USES_MAX3_SDK" /D "WINVER=0x500" /D "DIRECTEDITOR_BUILD" /D "STDLITH_MFC" /D "BDEFS_MFC" /D "PRE_FLOAT" /D "PREPROCESSOR_BUILD" /D "NO_PRAGMA_LIBS" /D "LT15_COMPAT" /Gy /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maxsdk-3.0\include" /I "..\..\shared\engine" /I "..\..\..\..\libs\stdlith" /I "..\..\..\sdk\inc" /I "..\..\preprocessor" /I "..\..\shared\world" /I "..\..\dedit\misc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USES_MAX3_SDK" /D "WINVER=0x500" /D "DIRECTEDITOR_BUILD" /D "STDLITH_MFC" /D "BDEFS_MFC" /D "PRE_FLOAT" /D "PREPROCESSOR_BUILD" /D "NO_PRAGMA_LIBS" /D "LT15_COMPAT" /Gy /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib comctl32.lib core.lib bmm.lib maxutil.lib paramblk2.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /def:".\MaxDTXLoader30.def" /out:"..\..\..\built\Release\tools\Plugins\MaxDTXLoader\MaxDTXLoader30.bmi" /libpath:"t:\maxsdk-3.0\lib"
# ADD LINK32 kernel32.lib user32.lib comctl32.lib core.lib bmm.lib maxutil.lib paramblk2.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /def:".\MaxDTXLoader30.def" /out:"..\..\..\built\Release\tools\Plugins\MaxDTXLoader\MaxDTXLoader30.bmi" /libpath:"t:\maxsdk-3.0\lib"

!ENDIF 

# Begin Target

# Name "MaxDTXLoader30 - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DLLMain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\dtxmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\genltstream.cpp
# End Source File
# Begin Source File

SOURCE=.\MaxDTXLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\pixelformat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\engine\streamsim.cpp
# End Source File
# Begin Source File

SOURCE=.\MaxDTXLoader.rc
# End Source File
# Begin Source File

SOURCE=.\MaxDTXLoader30.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\MaxDTXLoader.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# End Target
# End Project
