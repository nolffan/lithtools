# Microsoft Developer Studio Project File - Name="LTAMgr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=LTAMgr - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LTAMgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LTAMgr.mak" CFG="LTAMgr - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LTAMgr - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# talon: the LTA reader/writer library (Engine/libs/ltamgr) the 3ds Max and Maya
# talon: plug-ins of AvP2 Tools Update 2 link, written for this project from
# talon: Build 69's ltamgr.vcproj (Release: /MD, /Ob1, sdk\inc); VC6 SP5 (8966).
# talon: LT2\lithshared\stl (STLport 4.0) comes first: the shipped CLTAUtil::ShallowFindAll
# talon: grows its vector through STLport's _M_insert_overflow and __node_alloc.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LTAMgr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\built\Release\libs\ltamgr"
# PROP BASE Intermediate_Dir "..\..\built\Release\libs\ltamgr"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\built\Release\libs\ltamgr"
# PROP Intermediate_Dir "..\..\built\Release\libs\ltamgr"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "..\..\..\LT2\lithshared\stl" /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /GF /Gy /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob1 /I "..\..\..\LT2\lithshared\stl" /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"  /GF /Gy /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\built\Release\libs\ltamgr\ltamgr.lib"
# ADD LIB32 /nologo /out:"..\..\built\Release\libs\ltamgr\ltamgr.lib"

!ENDIF 

# Begin Target

# Name "LTAMgr - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ltabitfile.cpp
# End Source File
# Begin Source File

SOURCE=.\ltacompressedfile.cpp
# End Source File
# Begin Source File

SOURCE=.\ltaconverter.cpp
# End Source File
# Begin Source File

SOURCE=.\ltafile.cpp
# End Source File
# Begin Source File

SOURCE=.\ltafilebuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\ltahuffmantree.cpp
# End Source File
# Begin Source File

SOURCE=.\ltaloadonlyalloc.cpp
# End Source File
# Begin Source File

SOURCE=.\ltanode.cpp
# End Source File
# Begin Source File

SOURCE=.\ltanodebuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\ltanodeiterator.cpp
# End Source File
# Begin Source File

SOURCE=.\ltanodewriter.cpp
# End Source File
# Begin Source File

SOURCE=.\ltanodreader.cpp
# End Source File
# Begin Source File

SOURCE=.\ltareader.cpp
# End Source File
# Begin Source File

SOURCE=.\ltautil.cpp
# End Source File
# Begin Source File

SOURCE=.\ltawriter.cpp
# End Source File
# Begin Source File

SOURCE=.\lzsswindow.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\iltaallocator.h
# End Source File
# Begin Source File

SOURCE=.\ltabitfile.h
# End Source File
# Begin Source File

SOURCE=.\ltacompressedfile.h
# End Source File
# Begin Source File

SOURCE=.\ltaconverter.h
# End Source File
# Begin Source File

SOURCE=.\ltadefaultalloc.h
# End Source File
# Begin Source File

SOURCE=.\ltafile.h
# End Source File
# Begin Source File

SOURCE=.\ltafilebuffer.h
# End Source File
# Begin Source File

SOURCE=.\ltahuffmantree.h
# End Source File
# Begin Source File

SOURCE=.\ltalimits.h
# End Source File
# Begin Source File

SOURCE=.\ltaloadonlyalloc.h
# End Source File
# Begin Source File

SOURCE=.\ltamgr.h
# End Source File
# Begin Source File

SOURCE=.\ltanode.h
# End Source File
# Begin Source File

SOURCE=.\ltanodebuilder.h
# End Source File
# Begin Source File

SOURCE=.\ltanodeiterator.h
# End Source File
# Begin Source File

SOURCE=.\ltanodereader.h
# End Source File
# Begin Source File

SOURCE=.\ltanodewriter.h
# End Source File
# Begin Source File

SOURCE=.\ltaparseutils.h
# End Source File
# Begin Source File

SOURCE=.\ltareader.h
# End Source File
# Begin Source File

SOURCE=.\ltautil.h
# End Source File
# Begin Source File

SOURCE=.\ltawriter.h
# End Source File
# Begin Source File

SOURCE=.\lzsslimits.h
# End Source File
# Begin Source File

SOURCE=.\lzsswindow.h
# End Source File
# End Group
# End Target
# End Project
