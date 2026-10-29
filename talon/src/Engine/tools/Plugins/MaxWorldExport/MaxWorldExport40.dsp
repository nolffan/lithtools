# Microsoft Developer Studio Project File - Name="MaxWorldExport40" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MaxWorldExport40 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MaxWorldExport40.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MaxWorldExport40.mak" CFG="MaxWorldExport40 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MaxWorldExport40 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# talon: MaxWorldExport40.dle, the LithTech world exporter for 3ds Max 4 (AvP2 Tools
# talon: Update 2, built 2002-03-28), written for this project from Build 69's
# talon: MaxWorldExport.vcproj, "Release MAX40": maxsdk-4.0's headers and libraries
# talon: (toolchains/maxsdk-4.0), /MD, /Ob1, no TdGuard.lib (Touchdown's, later);
# talon: VC6 SP5's compiler (8966) and linker.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MaxWorldExport40 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\built\Release\tools\Plugins\MaxWorldExport40"
# PROP BASE Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MaxWorldExport40"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\built\Release\tools\Plugins\MaxWorldExport40"
# PROP Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MaxWorldExport40"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maxsdk-4.0\include" /I "..\..\..\sdk\inc" /I "..\..\..\libs\ltamgr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USES_MAX4_SDK" /GF /Gy /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maxsdk-4.0\include" /I "..\..\..\sdk\inc" /I "..\..\..\libs\ltamgr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USES_MAX4_SDK" /GF /Gy /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib maxutil.lib core.lib geom.lib bmm.lib mesh.lib paramblk2.lib mnmath.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /def:".\MaxWorldExport40.def" /out:"..\..\..\built\Release\tools\Plugins\MaxWorldExport\MaxWorldExport40.dle" /libpath:"t:\maxsdk-4.0\lib"
# ADD LINK32 comctl32.lib maxutil.lib core.lib geom.lib bmm.lib mesh.lib paramblk2.lib mnmath.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /def:".\MaxWorldExport40.def" /out:"..\..\..\built\Release\tools\Plugins\MaxWorldExport\MaxWorldExport40.dle" /libpath:"t:\maxsdk-4.0\lib"

!ENDIF 

# Begin Target

# Name "MaxWorldExport40 - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DLLMain.cpp
# End Source File
# Begin Source File

SOURCE=.\Exporter.cpp
# End Source File
# Begin Source File

SOURCE=.\LTAFile.cpp
# End Source File
# Begin Source File

SOURCE=.\MaxWorldExport.cpp
# End Source File
# Begin Source File

SOURCE=.\MaxWorldExport.rc
# End Source File
# Begin Source File

SOURCE=.\MaxWorldExport40.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\LTAFile.h
# End Source File
# Begin Source File

SOURCE=.\MaxWorldExport.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# End Target
# End Project
