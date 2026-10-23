; SPDX-License-Identifier: GPL-3.0-only
Unicode true
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "StrFunc.nsh"
${StrRep}
Name "Orbit Launcher"
Caption "Orbit Launcher ${VERSION}"
OutFile "${OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\OrbitLauncher"
InstallDirRegKey HKCU "Software\OrbitLauncher" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
SetOverwrite on
AllowSkipFiles off
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "Orbit Launcher"
VIAddVersionKey "FileDescription" "Orbit Launcher installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Orbit contributors; upstream notices included"
Var TestMode
Var AutoLaunch
Var UpdatePid
Var DataDir
Var BackupReady
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\OrbitLauncher.exe"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PAYLOAD}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Arabic"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Orbit requires 64-bit Windows."
    Abort
  ${EndIf}
  SetShellVarContext current
  StrCpy $TestMode ""
  StrCpy $AutoLaunch ""
  StrCpy $UpdatePid ""
  StrCpy $DataDir "$APPDATA\OrbitLauncher"
  ${GetParameters} $0
  ClearErrors
  ${GetOptions} $0 "/TESTMODE" $1
  ${IfNot} ${Errors}
    StrCpy $TestMode "1"
  ${EndIf}
  ClearErrors
  ${GetOptions} $0 "/AUTOLAUNCH" $1
  ${IfNot} ${Errors}
    StrCpy $AutoLaunch "1"
  ${EndIf}
  ${GetOptions} $0 "/UPDATEPID=" $UpdatePid
  ${GetOptions} $0 "/DATA64=" $1
  ${If} $1 != ""
    ${StrRep} $1 $1 "-" "+"
    ${StrRep} $1 $1 "_" "/"
    StrLen $2 $1
    IntOp $2 $2 % 4
    ${If} $2 == 2
      StrCpy $1 "$1=="
    ${ElseIf} $2 == 3
      StrCpy $1 "$1="
    ${EndIf}
    System::Call 'crypt32::CryptStringToBinaryW(w r1,i 0,i 1,p 0,*i .r2,p 0,p 0)i.r3'
    ${If} $3 != 0
      System::Alloc $2
      Pop $4
      System::Call 'crypt32::CryptStringToBinaryW(w r1,i 0,i 1,p r4,*i r2,p 0,p 0)i.r3'
      ${If} $3 != 0
        System::Call 'kernel32::MultiByteToWideChar(i 65001,i 8,p r4,i r2,p 0,i 0)i.r5'
        ${If} $5 > 0
          IntOp $6 ${NSIS_MAX_STRLEN} * 2
          System::Alloc $6
          Pop $7
          System::Call 'kernel32::MultiByteToWideChar(i 65001,i 8,p r4,i r2,p r7,i r5)i.r3'
          System::Call '*$7(&w${NSIS_MAX_STRLEN} .r8)'
          StrCpy $DataDir $8
          System::Free $7
        ${EndIf}
      ${EndIf}
      System::Free $4
    ${EndIf}
  ${EndIf}
  ${If} $UpdatePid != ""
    ; Wait for the requesting launcher; never terminate it or Minecraft forcibly.
    System::Call 'kernel32::OpenProcess(i 0x100000,i 0,i $UpdatePid)p.r0'
    ${If} $0 P<> 0
      System::Call 'kernel32::WaitForSingleObject(p r0,i 120000)i.r1'
      System::Call 'kernel32::CloseHandle(p r0)'
      ${If} $1 != 0
        SetErrorLevel 2
        Abort
      ${EndIf}
    ${EndIf}
  ${EndIf}
  InitPluginsDir
FunctionEnd

Function .onVerifyInstDir
  ; Installed application files and user content must never share a portable root.
  IfFileExists "$INSTDIR\accounts.json" reject_path
  IfFileExists "$INSTDIR\portable.txt" reject_path
  IfFileExists "$INSTDIR\instances\*.*" reject_path
  Return
reject_path:
  Abort
FunctionEnd

Function .onInstFailed
  ${If} $BackupReady == "1"
    !include "${GENERATED}\restore.nsh"
  ${EndIf}
FunctionEnd

Section "Orbit Launcher"
  SectionIn RO
  Call .onVerifyInstDir
  !include "${GENERATED}\backup.nsh"
  StrCpy $BackupReady "1"
  !include "${GENERATED}\install.nsh"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  ${If} $TestMode != "1"
    WriteRegStr HKCU "Software\OrbitLauncher" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "DisplayName" "Orbit Launcher"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher" "NoRepair" 1
    CreateDirectory "$SMPROGRAMS\Orbit Launcher"
    CreateShortcut "$SMPROGRAMS\Orbit Launcher\Orbit Launcher.lnk" "$INSTDIR\OrbitLauncher.exe"
    CreateShortcut "$DESKTOP\Orbit Launcher.lnk" "$INSTDIR\OrbitLauncher.exe"
  ${EndIf}
SectionEnd

Function .onInstSuccess
  ${If} $AutoLaunch == "1"
  ${AndIf} $TestMode != "1"
    Exec '"$INSTDIR\OrbitLauncher.exe" --dir "$DataDir"'
  ${EndIf}
FunctionEnd

Function un.onInit
  SetShellVarContext current
  StrCpy $TestMode ""
  ${GetParameters} $0
  ClearErrors
  ${GetOptions} $0 "/TESTMODE" $1
  ${IfNot} ${Errors}
    StrCpy $TestMode "1"
  ${EndIf}
FunctionEnd
Section "Uninstall"
  !include "${GENERATED}\uninstall.nsh"
  Delete "$INSTDIR\Uninstall.exe"
  ${If} $TestMode != "1"
    Delete "$SMPROGRAMS\Orbit Launcher\Orbit Launcher.lnk"
    RMDir "$SMPROGRAMS\Orbit Launcher"
    Delete "$DESKTOP\Orbit Launcher.lnk"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbitLauncher"
    DeleteRegKey HKCU "Software\OrbitLauncher"
  ${EndIf}
  ; No recursive deletion: worlds, accounts and all user content are preserved.
  RMDir "$INSTDIR"
SectionEnd
