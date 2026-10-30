# Microsoft Developer Studio Project File - Name="ME_LTAMgr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ME_LTAMgr - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ME_LTAMgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ME_LTAMgr.mak" CFG="ME_LTAMgr - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ME_LTAMgr - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# talon: stand-in. Build 69's LTA library (Engine/libs/ltamgr, the CLTANode
# reader the Build 69 ltaModel.cpp and ltaScene.cpp are written against),
# compiled as ModelEdit's objects are, so that they link. The 2001 image has
# none of it: its LTA reader is the older parse-node one (ltalex,
# ltaParseNode, ltaStream: CLTALexAnalizer, CParseNode, ILTAStream, which
# smmempnproc.cpp still names), between loadltadlg and ltaModel and after
# ltaModel in the image (progress/ModelEdit-map.md). This project goes when
# ltaModel and ltaScene are brought back to it.

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ME_LTAMgr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\built\Release\tools\ModelEdit\ltamgr"
# PROP BASE Intermediate_Dir "..\..\built\Release\tools\ModelEdit\ltamgr"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\built\Release\tools\ModelEdit\ltamgr"
# PROP Intermediate_Dir "..\..\built\Release\tools\ModelEdit\ltamgr"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\built\Release\tools\ModelEdit\ltamgr\ltamgr.lib"
# ADD LIB32 /nologo /out:"..\..\built\Release\tools\ModelEdit\ltamgr\ltamgr.lib"

!ENDIF 

# Begin Target

# Name "ME_LTAMgr - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltabitfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltacompressedfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltaconverter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltafile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltafilebuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltahuffmantree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltaloadonlyalloc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltanode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltanodebuilder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltanodeiterator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltanodewriter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltanodreader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltareader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltautil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\ltawriter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Engine\libs\ltamgr\lzsswindow.cpp
# End Source File
# End Group
# End Target
# End Project
