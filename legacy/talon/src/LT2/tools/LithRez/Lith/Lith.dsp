# Microsoft Developer Studio Project File - Name="Lith" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Lith - Win32 Release

# Begin Project
# PROP AllowPerConfigDependencies 0
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Lith - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\built\Release\tools\LithRez\Lith"
# PROP Intermediate_Dir "..\..\..\built\Release\tools\LithRez\Lith"
# PROP Target_Dir ""
# ADD CPP /nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
LIB32=link.exe -lib
# ADD LIB32 /nologo

!ENDIF

# Begin Target

# Name "Lith - Win32 Release"
# Begin Group "Source Files"

# Begin Source File

SOURCE=.\basehash.cpp
# End Source File
# Begin Source File

SOURCE=.\baselist.cpp
# End Source File
# Begin Source File

SOURCE=.\virtlist.cpp
# End Source File
# End Group
# End Target
# End Project
