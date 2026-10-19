Unicode True
!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "WinMessages.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
Name "Papel Vivo 2.0.1"
!ifndef OUTPUT_FILE
 !define OUTPUT_FILE "../../out/PapelVivo-2.0.1-Setup.exe"
!endif
!ifndef PAYLOAD_ROOT
 !define PAYLOAD_ROOT "../../stage"
!endif
OutFile "${OUTPUT_FILE}"
RequestExecutionLevel user
InstallDir "$LOCALAPPDATA\Programs\PapelVivo"
InstallDirRegKey HKCU "Software\kz\PapelVivo" "InstallDir"
!ifdef QA_FAST
SetCompressor zlib
SetCompress off
!else
SetCompressor /SOLID lzma
SetCompressorDictSize 64

!endif
CRCCheck on
SetFont "Segoe UI" 10
ShowInstDetails show
ShowUninstDetails show
Var NewDir
Var LogPath
Var LogHandle
Var Tick
Var OldDir
Var PreviousDir
Var Committed
Var ScanHandle
Var FoundDir
Var ScanPath
Var RemovalError
Var NewKey
Var OldKey
Var InstallerMutex
Var DeleteUserData
Var DeleteDataCheckbox
Var LanguageChoice


BrandingText "Papel Vivo 2.0.1  |  kz"
VIProductVersion "2.0.1.0"
VIAddVersionKey "ProductName" "Papel Vivo"
VIAddVersionKey "CompanyName" "kz"
VIAddVersionKey "FileDescription" "Papel Vivo Setup / Instalador"
VIAddVersionKey "FileVersion" "2.0.1"
VIAddVersionKey "LegalCopyright" "kz — direitos dos componentes de terceiros preservados"
!define MUI_ICON "../assets/app.ico"
!define MUI_UNICON "../assets/app.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_BGCOLOR "FFF0F6"
!define MUI_TEXTCOLOR "4B1938"
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_CUSTOMFUNCTION_ABORT InstallAbort
!define MUI_LANGDLL_REGISTRY_ROOT HKCU
!define MUI_LANGDLL_REGISTRY_KEY "Software\kz\PapelVivo"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallerLanguage"
!define MUI_LANGDLL_ALWAYSSHOW
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_WELCOMEPAGE_TITLE "$(WelcomeTitle)"
!define MUI_WELCOMEPAGE_TEXT "$(WelcomeBody)"
Page custom SetupLanguagePage SetupLanguageLeave
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "$(LicenseData)"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE CheckDirectory
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_SHOW ProgressStyle
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TITLE "$(FinishTitle)"
!define MUI_FINISHPAGE_TEXT "$(FinishBody)"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "$(LaunchText)"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!define MUI_FINISHPAGE_SHOWREADME "$LogPath"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(LogText)"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
UninstPage custom un.DataPage un.DataLeave
!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.ProgressStyle
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "English"
LicenseLangString LicenseData ${LANG_PORTUGUESEBR} "../LICENCA-PAPEL-VIVO.txt"
LicenseLangString LicenseData ${LANG_ENGLISH} "LICENSE-en.txt"
!include "strings.nsh"
!include "payload.nsh"

Function Log
 Exch $0
 ${If} $LogHandle != ""
  FileWrite $LogHandle "$0$\r$\n"
 ${EndIf}
 ; Detailed records go to the local log, not the public setup screen.
 Pop $0
FunctionEnd
Function .onInit
 SetRegView 64
 SetShellVarContext current
 StrCpy $LANGUAGE ${LANG_PORTUGUESEBR}
 ReadRegDWORD $1 HKCU "Software\kz\PapelVivo" "InstallerLanguage"
 ${If} $1 == ${LANG_ENGLISH}
  StrCpy $LANGUAGE ${LANG_ENGLISH}
 ${EndIf}
 System::Call 'kernel32::CreateMutexW(p 0, i 0, w "Local\PapelVivo.Player.Setup") p .r0 ?e'
 Pop $1
 StrCpy $InstallerMutex $0
 ${If} $1 == 183
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(AlreadyRunning)"
  Abort
 ${EndIf}
 ${GetParameters} $0
 ClearErrors
 ${GetOptions} "$0" "/LANG=" $1
 ${IfNot} ${Errors}
  ${If} $1 == "en"
   StrCpy $LANGUAGE ${LANG_ENGLISH}
  ${ElseIf} $1 == "pt-BR"
   StrCpy $LANGUAGE ${LANG_PORTUGUESEBR}
  ${Else}
   MessageBox MB_OK|MB_ICONSTOP "Only pt-BR and en are supported. / Somente pt-BR e en são aceitos."
   Abort
  ${EndIf}
 ${EndIf}
 ${IfNot} ${RunningX64}
  MessageBox MB_OK|MB_ICONSTOP "$(Need64)"
  Abort
 ${EndIf}
 ${IfNot} ${AtLeastWin10}
  MessageBox MB_OK|MB_ICONSTOP "$(Need10)"
  Abort
 ${EndIf}
 StrCpy $Committed 0
 StrCpy $NewDir ""
 ReadRegStr $OldDir HKCU "Software\kz\PapelVivo" "CurrentDir"
 ReadRegStr $OldKey HKCU "Software\kz\PapelVivo" "UninstallKey"
 ReadRegStr $PreviousDir HKCU "Software\kz\PapelVivo" "InstallDir"
 System::Call 'kernel32::GetTickCount() i .r0'
 StrCpy $Tick $0
 StrCpy $NewKey "PapelVivo-kz-2.0.1-$Tick"
 StrCpy $LogPath "$TEMP\PapelVivo-Setup-$Tick.log"
 ClearErrors
 FileOpen $LogHandle "$LogPath" w
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONSTOP "$(CannotLog)"
  Abort
 Push "Papel Vivo 2.0.1 | kz | language=$LANGUAGE"
 Call Log
 Push "$(Started)"
 Call Log
FunctionEnd
Function CloseRunningApp
 FindWindow $0 "PapelVivo.Player.Control"
 ${If} $0 != 0
  MessageBox MB_YESNO|MB_ICONQUESTION "$(CloseApp)" IDYES close_it
  Abort
 close_it:
  SendMessage $0 ${WM_COMMAND} 110 0 /TIMEOUT=3000
  StrCpy $1 0
 wait_app:
  Sleep 100
  FindWindow $0 "PapelVivo.Player.Control"
  ${If} $0 != 0
   IntOp $1 $1 + 1
   IntCmp $1 100 app_busy wait_app app_busy
  ${EndIf}
  Goto app_closed
 app_busy:
  MessageBox MB_OK|MB_ICONSTOP "$(AppBusy)"
  Abort
 app_closed:
 ${EndIf}
FunctionEnd
Function CheckDirectory
 ClearErrors
 Push "Requested directory: $INSTDIR"
 Call Log
 System::Call 'kernel32::GetFullPathNameW(w "$INSTDIR", i 1024, w .r2, p 0) i .r3'
 ${If} $3 == 0
  Goto dir_fail
 ${EndIf}
 StrCpy $INSTDIR $2
 Push "Directory validation: path=$INSTDIR; Windows=$WINDIR"
 Call Log
 ${GetRoot} "$INSTDIR" $0
 StrCpy $1 "$0\"
 Push "Root validation: root=$0; path=$INSTDIR"
 Call Log
 ${If} $INSTDIR == $0
 ${OrIf} $INSTDIR == $1
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(UnsafeFolder)"
  Abort
 ${EndIf}
 StrLen $0 $INSTDIR
 ${If} $0 > 150
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(LongFolder)"
  Abort
 ${EndIf}
 StrLen $0 $WINDIR
 StrCpy $1 $INSTDIR $0
 ${If} $1 == $WINDIR
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(UnsafeFolder)"
  Abort
 ${EndIf}
 ReadINIStr $4 "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "Product"
 ${If} $4 == "kz.PapelVivo.Player"
  Goto own_folder
 ${EndIf}
 IfFileExists "$INSTDIR\*.*" 0 own_folder
 FindFirst $0 $1 "$INSTDIR\*.*"
 check_nonempty:
  ${If} $1 == ""
   FindClose $0
   Goto own_folder
  ${EndIf}
  ${If} $1 == "."
  ${OrIf} $1 == ".."
   FindNext $0 $1
   Goto check_nonempty
  ${EndIf}
 FindClose $0
 MessageBox MB_YESNO|MB_ICONQUESTION "$(FolderOccupied)" IDYES create_child
 Abort
 create_child:
 StrCpy $INSTDIR "$INSTDIR\PapelVivo"
 ReadINIStr $4 "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "Product"
 ${If} $4 == "kz.PapelVivo.Player"
  Goto own_folder
 ${EndIf}
 IfFileExists "$INSTDIR\*.*" 0 own_folder
 MessageBox MB_OK|MB_ICONEXCLAMATION "$(ChooseEmpty)"
 Abort
 own_folder:
 ClearErrors
 CreateDirectory "$INSTDIR"
 IfErrors dir_fail
 GetTempFileName $0 "$INSTDIR"
 IfErrors dir_fail
 ClearErrors
 FileOpen $1 "$0" w
 IfErrors dir_fail
 ClearErrors
 FileWrite $1 "Papel Vivo write test"
 IfErrors write_test_failed
 FileClose $1
 Delete "$0"
 Push "$(Destination) $INSTDIR"
 Call Log
 Return
 write_test_failed:
 FileClose $1
 Delete "$0"
 dir_fail:
 Push "$(FolderWriteError) $INSTDIR"
 Call Log
 MessageBox MB_OK|MB_ICONSTOP "$(FolderWriteError)$\r$\n$INSTDIR$\r$\n$\r$\n$(LogAt) $LogPath"
 Abort
FunctionEnd
Function Rollback
 ${If} $NewDir != ""
 ${AndIf} $Committed != 1
  Push "$(RollbackText) $NewDir"
  Call Log
  !insertmacro DeleteOwned "$NewDir"
  Delete "$NewDir\.papel-vivo-version"
  Delete "$NewDir\Desinstalar.exe"
  RMDir "$NewDir"
  RMDir "$INSTDIR\App"
  ${If} $PreviousDir == $INSTDIR
   WriteINIStr "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "CurrentDir" "$OldDir"
  ${Else}
   Delete "$INSTDIR\PapelVivo-install.ini"
  ${EndIf}
  RMDir "$INSTDIR"
 ${EndIf}
FunctionEnd
Function InstallAbort
 SetErrorLevel 2
 Push "$(CancelledText)"
 Call Log
 Call Rollback
 FileClose $LogHandle
 StrCpy $LogHandle ""
FunctionEnd
Function .onInstFailed
 SetErrorLevel 1
 Call Rollback
 Push "$(FailedText)"
 Call Log
 FileClose $LogHandle
 StrCpy $LogHandle ""
 MessageBox MB_YESNO|MB_ICONSTOP "$(FailedText)$\r$\n$(LogAt) $LogPath$\r$\n$\r$\n$(OpenLogQuestion)" IDNO no_log
 Exec '"$WINDIR\notepad.exe" "$LogPath"'
 no_log:
FunctionEnd
Function .onInstSuccess
 FileClose $LogHandle
 StrCpy $LogHandle ""
 CreateDirectory "$LOCALAPPDATA\PapelVivo\Player\Logs"
 CopyFiles /SILENT "$LogPath" "$LOCALAPPDATA\PapelVivo\Player\Logs\"
FunctionEnd
Function LaunchApp
 Exec '"$NewDir\PapelVivo.exe"'
 IfErrors 0 done_launch
 MessageBox MB_OK|MB_ICONEXCLAMATION "$(LaunchError)$\r$\n$NewDir"
 done_launch:
FunctionEnd

Section "$(MainSection)" MainSection
 SectionIn RO
 StrCpy $RemovalError 0
 Call CloseRunningApp
 Call CheckDirectory
 StrCpy $NewDir "$INSTDIR\App\2.0.1-$Tick"
 IfFileExists "$NewDir\*.*" 0 fresh_destination
 StrCpy $NewDir ""
 Abort
 fresh_destination:
 ClearErrors
 CreateDirectory "$NewDir"
 IfErrors install_fail
 Push "$(WaitText)"
 Call Log
 SetDetailsPrint none
 !insertmacro ExtractPayload
 SetDetailsPrint both
 DetailPrint "$(WaitText)"
 ClearErrors
 WriteUninstaller "$NewDir\Desinstalar.exe"
 IfErrors install_fail
 WriteINIStr "$NewDir\.papel-vivo-version" "PapelVivo" "Product" "kz.PapelVivo.Player"
 WriteINIStr "$NewDir\.papel-vivo-version" "PapelVivo" "Version" "2.0.1"
 IfErrors install_fail
 WriteINIStr "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "Product" "kz.PapelVivo.Player"
 WriteINIStr "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "CurrentDir" "$NewDir"
 IfErrors install_fail
 ; Only our own registry keys and preferences; no system DLL, service or key store.
 ClearErrors
 WriteRegStr HKCU "Software\kz\PapelVivo" "InstallDir" "$INSTDIR"
 WriteRegStr HKCU "Software\kz\PapelVivo" "CurrentDir" "$NewDir"
 WriteRegStr HKCU "Software\kz\PapelVivo" "UninstallKey" "$NewKey"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "DisplayName" "Papel Vivo 2.0.1"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "DisplayVersion" "2.0.1"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "Publisher" "kz"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "UninstallString" '$\"$NewDir\Desinstalar.exe$\"'
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "DisplayIcon" "$NewDir\PapelVivo.exe,0"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "InstallLocation" "$INSTDIR"
 WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "NoModify" 1
 WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey" "NoRepair" 1
 IfErrors registry_fail
 ClearErrors
 CreateDirectory "$LOCALAPPDATA\PapelVivo\Player"
 ${If} $LANGUAGE == ${LANG_ENGLISH}
  WriteINIStr "$LOCALAPPDATA\PapelVivo\Player\config.ini" "Preferencias" "Idioma" "en"
 ${Else}
  WriteINIStr "$LOCALAPPDATA\PapelVivo\Player\config.ini" "Preferencias" "Idioma" "pt-BR"
 ${EndIf}
 IfErrors 0 preferences_ok
 Push "$(PreferenceWarning)"
 Call Log
 MessageBox MB_OK|MB_ICONEXCLAMATION "$(PreferenceWarning)"
 preferences_ok:
 ClearErrors
 CreateDirectory "$SMPROGRAMS\Papel Vivo"
 CreateShortCut "$SMPROGRAMS\Papel Vivo\Papel Vivo.lnk" "$NewDir\PapelVivo.exe" "" "$NewDir\PapelVivo.exe" 0
 CreateShortCut "$SMPROGRAMS\Papel Vivo\Desinstalar.lnk" "$NewDir\Desinstalar.exe" "" "$NewDir\PapelVivo.exe" 0
 IfErrors 0 start_shortcuts_ok
 Push "$(ShortcutError)"
 Call Log
 MessageBox MB_OK|MB_ICONEXCLAMATION "$(ShortcutError)"
 start_shortcuts_ok:
 StrCpy $Committed 1
 ${If} $OldKey != ""
  StrCpy $0 $OldKey 13
  ${If} $0 == "PapelVivo-kz-"
   DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$OldKey"
  ${EndIf}
 ${EndIf}
 Push "$(SuccessText) $NewDir"
 Call Log
 Goto install_done
 registry_fail:
 ${If} $PreviousDir != ""
  WriteRegStr HKCU "Software\kz\PapelVivo" "InstallDir" "$PreviousDir"
  WriteRegStr HKCU "Software\kz\PapelVivo" "CurrentDir" "$OldDir"
  WriteRegStr HKCU "Software\kz\PapelVivo" "UninstallKey" "$OldKey"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey"
 ${Else}
  DeleteRegKey HKCU "Software\kz\PapelVivo"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey"
 ${EndIf}
 install_fail:
 SetErrorLevel 1
 Push "$(ExtractError) $NewDir"
 Call Log
 SetErrors
 Abort
 install_done:
SectionEnd
Section /o "$(DesktopSection)" DesktopSection
 ClearErrors
 CreateShortCut "$DESKTOP\Papel Vivo.lnk" "$NewDir\PapelVivo.exe" "" "$NewDir\PapelVivo.exe" 0
 IfErrors 0 desktop_done
 Push "$(ShortcutError)"
 Call Log
 MessageBox MB_OK|MB_ICONEXCLAMATION "$(ShortcutError)"
 desktop_done:
SectionEnd
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
 !insertmacro MUI_DESCRIPTION_TEXT ${MainSection} "$(MainDescription)"
 !insertmacro MUI_DESCRIPTION_TEXT ${DesktopSection} "$(DesktopDescription)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onInit
 StrCpy $DeleteUserData 0
 System::Call 'kernel32::CreateMutexW(p 0, i 0, w "Local\PapelVivo.Player.Setup") p .r0 ?e'
 Pop $1
 StrCpy $InstallerMutex $0
 ${If} $1 == 183
  MessageBox MB_OK|MB_ICONEXCLAMATION "Papel Vivo setup is already running. / O instalador já está aberto."
  Abort
 ${EndIf}
 SetRegView 64
 SetShellVarContext current
 ReadRegStr $INSTDIR HKCU "Software\kz\PapelVivo" "InstallDir"
 ${If} $INSTDIR == ""
  ${GetParent} "$EXEDIR" $0
  ${GetParent} "$0" $INSTDIR
 ${EndIf}
 ReadINIStr $0 "$INSTDIR\PapelVivo-install.ini" "PapelVivo" "Product"
 ${If} $0 != "kz.PapelVivo.Player"
  MessageBox MB_OK|MB_ICONSTOP "Papel Vivo: install marker missing. No files were removed. / Registro da instalação ausente. Nada foi removido."
  Abort
 ${EndIf}
 ReadRegStr $NewKey HKCU "Software\kz\PapelVivo" "UninstallKey"
 ReadINIStr $0 "$LOCALAPPDATA\PapelVivo\Player\config.ini" "Preferencias" "Idioma"
 ${If} $0 == "en"
  StrCpy $LANGUAGE ${LANG_ENGLISH}
 ${Else}
  StrCpy $LANGUAGE ${LANG_PORTUGUESEBR}
 ${EndIf}
FunctionEnd
Section "Uninstall"
 StrCpy $RemovalError 0
 FindWindow $0 "PapelVivo.Player.Control"
 ${If} $0 != 0
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(UnCloseApp)"
  Abort
 ${EndIf}
 FindFirst $ScanHandle $FoundDir "$INSTDIR\App\2.0.*"
 un_next:
 ${If} $FoundDir == ""
  Goto un_finished
 ${EndIf}
 StrCpy $ScanPath "$INSTDIR\App\$FoundDir"
 ReadINIStr $0 "$ScanPath\.papel-vivo-version" "PapelVivo" "Product"
 ${If} $0 == "kz.PapelVivo.Player"
  DetailPrint "$(RemovingText) $ScanPath"
  !insertmacro DeleteOwned "$ScanPath"
  ; Previous release source archive: exact owned filename, only after product marker verification.
  Delete "$ScanPath\Sources\PapelVivo-2.0.0-Fontes.zip"
  RMDir "$ScanPath\Sources"
  ${If} $RemovalError == 0
  Delete "$ScanPath\.papel-vivo-version"
  Delete "$ScanPath\Desinstalar.exe"
  RMDir "$ScanPath"
  ${EndIf}
 ${EndIf}
 FindNext $ScanHandle $FoundDir
 Goto un_next
 un_finished:
 FindClose $ScanHandle
 ${If} $RemovalError != 0
  MessageBox MB_OK|MB_ICONSTOP "$(RemoveFailed)"
  SetErrorLevel 1
  Abort
 ${EndIf}
 RMDir "$INSTDIR\App"
 Delete "$SMPROGRAMS\Papel Vivo\Papel Vivo.lnk"
 Delete "$SMPROGRAMS\Papel Vivo\Desinstalar.lnk"
 RMDir "$SMPROGRAMS\Papel Vivo"
 Delete "$DESKTOP\Papel Vivo.lnk"
 Delete "$INSTDIR\PapelVivo-install.ini"
 RMDir "$INSTDIR"
 StrCpy $0 $NewKey 13
 ${If} $0 == "PapelVivo-kz-"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$NewKey"
 ${EndIf}
 DeleteRegKey HKCU "Software\kz\PapelVivo"
 ${If} $DeleteUserData == ${BST_CHECKED}
  RMDir /r "$LOCALAPPDATA\PapelVivo\Player"
 ${EndIf}
SectionEnd

Function ProgressStyle
 GetDlgItem $0 $HWNDPARENT 1004
 System::Call 'user32::GetWindowLongW(p r0, i -16) i .r1'
 IntOp $1 $1 | 1
 System::Call 'user32::SetWindowLongW(p r0, i -16, i r1)'
 SendMessage $0 ${PBM_SETBARCOLOR} 0 0x009A2BC2
FunctionEnd
Function un.ProgressStyle
 GetDlgItem $0 $HWNDPARENT 1004
 SendMessage $0 ${PBM_SETBARCOLOR} 0 0x009A2BC2
FunctionEnd
Function un.DataPage
 !insertmacro MUI_HEADER_TEXT "$(UserDataTitle)" "$(UserDataBody)"
 nsDialogs::Create 1018
 Pop $0
 SetCtlColors $0 4B1938 FFF0F6
 ${NSD_CreateLabel} 10u 12u 280u 32u "$(DeleteDataQuestion)"
 Pop $1
 SetCtlColors $1 4B1938 FFF0F6
 ${NSD_CreateCheckbox} 10u 56u 280u 36u "$(DeleteDataOption)"
 Pop $DeleteDataCheckbox
 ${NSD_Uncheck} $DeleteDataCheckbox
 SetCtlColors $DeleteDataCheckbox 4B1938 FFF0F6
 ${NSD_CreateLabel} 10u 108u 280u 56u "$(DeleteDataHelp)"
 Pop $1
 SetCtlColors $1 4B1938 FFF0F6
 nsDialogs::Show
FunctionEnd
Function un.DataLeave
 ${NSD_GetState} $DeleteDataCheckbox $DeleteUserData
FunctionEnd

Function SetupLanguagePage
 !insertmacro MUI_HEADER_TEXT "Papel Vivo 2.0.1" "Português / English"
 nsDialogs::Create 1018
 Pop $0
 SetCtlColors $0 4B1938 FFF0F6
 ${NSD_CreateLabel} 0 12u 100% 30u "Escolha o idioma da instalação. / Choose the setup language."
 Pop $0
 SetCtlColors $0 4B1938 FFF0F6
 ${NSD_CreateDropList} 0 50u 90% 80u ""
 Pop $LanguageChoice
 ${NSD_CB_AddString} $LanguageChoice "Português (Brasil)"
 ${NSD_CB_AddString} $LanguageChoice "English"
 ${If} $LANGUAGE == ${LANG_ENGLISH}
  SendMessage $LanguageChoice ${CB_SETCURSEL} 1 0
 ${Else}
  SendMessage $LanguageChoice ${CB_SETCURSEL} 0 0
 ${EndIf}
 nsDialogs::Show
FunctionEnd
Function SetupLanguageLeave
 ${NSD_GetText} $LanguageChoice $0
 ${If} $0 == "English"
  StrCpy $LANGUAGE ${LANG_ENGLISH}
 ${Else}
  StrCpy $LANGUAGE ${LANG_PORTUGUESEBR}
 ${EndIf}
 WriteRegDWORD HKCU "Software\kz\PapelVivo" "InstallerLanguage" $LANGUAGE
FunctionEnd
