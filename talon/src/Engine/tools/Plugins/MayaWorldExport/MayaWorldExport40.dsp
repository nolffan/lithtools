# Microsoft Developer Studio Project File - Name="MayaWorldExport40" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MayaWorldExport40 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MayaWorldExport40.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MayaWorldExport40.mak" CFG="MayaWorldExport40 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MayaWorldExport40 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# talon: MayaWorldExport40.mll, the LithTech world exporter and importer for Maya 4.0 (AvP2 Tools Update 2,
# talon: built 2002-03-28), written for this project from Build 69's MayaWorldExport.vcproj,
# talon: "Release Maya40": the Maya 4.0 developer kit (toolchains/maya-4.0-devkit), /MD,
# talon: /Ob1, no TdGuard.lib (Touchdown's, later); VC6 SP5's compiler (8966) and
# talon: linker.
# talon: STLport (LT2\lithshared\stl), as the shipped code has it: its vector has no
# talon: allocator word and size() makes no null test. No LTEulerAngles.cpp: the 2002
# talon: ExportNodeKey converts with MEulerRotation::reorderIt.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MayaWorldExport40 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\built\Release\tools\Plugins\MayaWorldExport40"
# PROP BASE Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MayaWorldExport40"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\built\Release\tools\Plugins\MayaWorldExport40"
# PROP Intermediate_Dir "..\..\..\built\Release\tools\Plugins\MayaWorldExport40"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maya-4.0-devkit\include" /I "..\..\..\..\LT2\lithshared\stl" /I "..\Shared" /I "..\..\..\libs\ltamgr" /I "..\..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "NT_PLUGIN" /D "USES_MAYA4_SDK" /GF /Gy /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "t:\maya-4.0-devkit\include" /I "..\..\..\..\LT2\lithshared\stl" /I "..\Shared" /I "..\..\..\libs\ltamgr" /I "..\..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "NT_PLUGIN" /D "USES_MAYA4_SDK" /GF /Gy /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Foundation.lib OpenMaya.lib OpenMayaAnim.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /out:"..\..\..\built\Release\tools\Plugins\MayaWorldExport\MayaWorldExport40.mll" /libpath:"t:\maya-4.0-devkit\lib"
# ADD LINK32 Foundation.lib OpenMaya.lib OpenMayaAnim.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"libc.lib" /out:"..\..\..\built\Release\tools\Plugins\MayaWorldExport\MayaWorldExport40.mll" /libpath:"t:\maya-4.0-devkit\lib"

!ENDIF 

# Begin Target

# Name "MayaWorldExport40 - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DllMain.cpp
# End Source File
# Begin Source File

SOURCE=.\LTAFile.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldExportCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldExportFileTranslator.cpp
# End Source File
# Begin Source File

SOURCE=..\Shared\WorldImport.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldImportCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldImportFileTranslator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\LTAFile.h
# End Source File
# Begin Source File

SOURCE=.\WorldExportCommand.h
# End Source File
# Begin Source File

SOURCE=.\WorldExportFileTranslator.h
# End Source File
# Begin Source File

SOURCE=.\WorldImportCommand.h
# End Source File
# Begin Source File

SOURCE=.\WorldImportFileTranslator.h
# End Source File
# End Group
# End Target
# End Project
