# Microsoft Developer Studio Project File - Name="LithRez" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LithRez - Win32 Release

# Begin Project
# PROP AllowPerConfigDependencies 0
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LithRez - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\built\Release\tools\LithRez"
# PROP Intermediate_Dir "..\..\built\Release\tools\LithRez"
# PROP Target_Dir ""
# ADD CPP /nologo /ML /W3 /GX /O2 /I "RezMgr" /I "Lith" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
LINK32=link.exe
# ADD LINK32 libc.lib kernel32.lib /nologo /subsystem:console /machine:I386

!ENDIF

# Begin Target

# Name "LithRez - Win32 Release"
# Begin Group "Source Files"

# Begin Source File

SOURCE=.\lithrez.cpp
# End Source File
# Begin Source File

SOURCE=.\rezutils.cpp
# End Source File
# End Group
# End Target
# End Project
