;NSIS Script For Rename-It!

!define PRODUCT "Rename-It!"	;Define your own software name here
!define VERSION "4iter2"		;Define your own software version here

;--------------------------------
;Include Modern UI

	!include "MUI.nsh"

;--------------------------------
;Configuration

	;Do A CRC Check
	CRCCheck On

	;Compression format
	SetCompressor /SOLID lzma

	;General
	Name "${PRODUCT}"
	OutFile "renameit-${VERSION}-install.exe"

	;The Default Installation Directory
	InstallDir "$PROGRAMFILES\${PRODUCT}"

	;Remember install folder
	InstallDirRegKey HKLM "Software\${PRODUCT}" ""

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING
	!define MUI_FINISHPAGE_RUN "$INSTDIR\RenameIt.exe"

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "gpl.txt"
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	!insertmacro MUI_PAGE_FINISH
	
	!insertmacro MUI_UNPAGE_WELCOME
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Reserve Files
  
  ;These files should be inserted before other files in the data block
  ;Keep these lines before any File command
  ;Only for solid compression (by default, solid compression is enabled for BZIP2 and LZMA)
  
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

;--------------------------------
;Installer Sections

Section "!${PRODUCT}" SecProduct
	SectionIn RO

	; Get Windows Version
	Call GetWindowsVersion
	Pop $R0
	StrCmp $R0 "2000" lblUnicodeInstall
	StrCmp $R0 "XP" lblUnicodeInstall
	StrCmp $R0 ".NET Server" lblUnicodeInstall
		; Non-Unicode Install
		SetOutPath "$INSTDIR"
		File "..\build\Rename-It!\Release\RenameIt.exe"
		GoTo lblEndSpecificOSInstall
	lblUnicodeInstall: 
		; Unicode Install
		SetOutPath "$INSTDIR"
		File "..\build\Rename-It!\Unicode Release\RenameIt.exe"
		GoTo lblEndSpecificOSInstall
	lblEndSpecificOSInstall:

	; Other install files (not depending to OS)
	SetOutPath "$INSTDIR"
	File "id3lib.dll"
	File "pcre3.dll"
	File "History.txt"
	File "gpl.txt"
	File "..\build\Rename-It!\Unicode Release\Rename-It!.chm"
	File "..\build\Tutorials\Tutorial.exe"
	SetOutPath "$INSTDIR\Filters"
	File "Filters\*.rit"

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" "$INSTDIR\Uninst.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayIcon" "$INSTDIR\RenameIt.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "Publisher" "Beroux"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "URLInfoAbout" "http://www.beroux.com/"	;Publisher's link
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "HelpLink" "http://sourceforge.net/projects/renameit/" ;Support Information
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "URLUpdateInfo" "http://www.beroux.com/"	;Product Updates
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoModify" 0x00000001
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoRepair" 0x00000001

	;Set saved filter path
	WriteRegStr HKCU "Software\${PRODUCT}" "Filters" "$INSTDIR\Filters"
	WriteUninstaller "Uninst.exe"
SectionEnd

Section "Shell Incorporation" SecShell
	; Install the new version
	SetOverwrite ifdiff
	SetOutPath "$INSTDIR"
	Call GetWindowsVersion
	Pop $R0
	StrCmp $R0 "" lblNonUnicodeInstall
	StrCmp $R0 "95" lblNonUnicodeInstall
	StrCmp $R0 "98" lblNonUnicodeInstall
	StrCmp $R0 "ME" lblNonUnicodeInstall
		; Unicode Install
		File /oname=SimpleExt.dl_ "..\build\SimpleExt\Unicode Release MinDependency\SimpleExt.dll"
		GoTo lblEndSpecificOSInstall
	lblNonUnicodeInstall: 
		; Non-Unicode Install
		File /oname=SimpleExt.dl_ "..\build\SimpleExt\Release MinDependency\SimpleExt.dll"
		GoTo lblEndSpecificOSInstall
	lblEndSpecificOSInstall:

	; Rename the file to the correct name
	Rename /REBOOTOK "$INSTDIR\SimpleExt.dl_" "$INSTDIR\SimpleExt.dll"

	; Tell where to find Rename-It!
	WriteRegStr HKLM "Software\${PRODUCT}" "ExeDir" "$INSTDIR"

	; Register the DLL
	RegDLL "$INSTDIR\SimpleExt.dll"
SectionEnd

Section "Start Menu Shortcuts" SecShortcuts
	SetOutPath $INSTDIR
	SetShellVarContext all

	;Add Shortcuts
	CreateDirectory "$SMPROGRAMS\${PRODUCT}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\${PRODUCT}.lnk" "$INSTDIR\RenameIt.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\Help.lnk" "$INSTDIR\Rename-It!.chm"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\Tutorial.lnk" "$INSTDIR\Tutorial.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\What's new.lnk" "$INSTDIR\History.txt"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\Uninstall.lnk" "$INSTDIR\Uninst.exe"
SectionEnd

Section "Desktop Icon" SecDesktop
	SetOutPath $INSTDIR

	;Add Shortcuts
	CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\RenameIt.exe"
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecProduct ${LANG_ENGLISH} "Install ${PRODUCT} and other required files."
  LangString DESC_SecShell ${LANG_ENGLISH} "Add a quick access to ${PRODUCT} from the shell context menu."
  LangString DESC_SecShortcuts ${LANG_ENGLISH} "Create shortcuts in the start menu."
  LangString DESC_SecDesktop ${LANG_ENGLISH} "Add a desktop icon for quick access to ${PRODUCT}."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecProduct} $(DESC_SecProduct)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShell} $(DESC_SecShell)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} $(DESC_SecShortcuts)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section Uninstall
	;Remove Context menu support
	UnRegDLL "$INSTDIR\SimpleExt.dll"
	DeleteRegKey HKLM "Software\${PRODUCT}\ExeDir"

	;Delete Files
	Delete /REBOOTOK "$INSTDIR\RenameIt.exe"
	Delete /REBOOTOK "$INSTDIR\id3lib.dll"
	Delete /REBOOTOK "$INSTDIR\pcre3.dll"
	Delete "$INSTDIR\History.txt"
	Delete "$INSTDIR\gpl.txt"
	Delete /REBOOTOK "$INSTDIR\SimpleExt.dll"
	Delete "$INSTDIR\Filters\*.rit"
	Delete "$DESKTOP\${PRODUCT}.lnk"

	;Delete Shortcuts
	Delete "$SMPROGRAMS\${PRODUCT}\*.*"
	RMDir "$SMPROGRAMS\${PRODUCT}"

	;Delete Uninstaller And Unistall Registry Entries
	Delete "$INSTDIR\Uninst.exe"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
	RMDir "$INSTDIR\Filters"
	RMDir "$INSTDIR"

	DeleteRegKey /ifempty HKLM "Software\${PRODUCT}"
SectionEnd

; GetWindowsVersion
;
; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
; Updated by Joost Verburg
;
; Returns on top of stack
;
; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003)
; or
; '' (Unknown Windows Version)
;
; Usage:
;   Call GetWindowsVersion
;   Pop $R0
;   ; at this point $R0 is "NT 4.0" or whatnot

Function GetWindowsVersion
	Push $R0
	Push $R1

	ClearErrors
	ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
	IfErrors 0 lbl_winnt

	; we are not NT
	ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber

	StrCpy $R1 $R0 1
	StrCmp $R1 '4' 0 lbl_error

	StrCpy $R1 $R0 3

	StrCmp $R1 '4.0' lbl_win32_95
	StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98

	lbl_win32_95:
		StrCpy $R0 '95'
		Goto lbl_done

	lbl_win32_98:
		StrCpy $R0 '98'
		Goto lbl_done

	lbl_win32_ME:
		StrCpy $R0 'ME'
		Goto lbl_done

	lbl_winnt:

	StrCpy $R1 $R0 1

	StrCmp $R1 '3' lbl_winnt_x
	StrCmp $R1 '4' lbl_winnt_x

	StrCpy $R1 $R0 3

	StrCmp $R1 '5.0' lbl_winnt_2000
	StrCmp $R1 '5.1' lbl_winnt_XP
	StrCmp $R1 '5.2' lbl_winnt_2003 lbl_error

	lbl_winnt_x:
		StrCpy $R0 "NT $R0" 6
		Goto lbl_done

	lbl_winnt_2000:
		Strcpy $R0 '2000'
		Goto lbl_done

	lbl_winnt_XP:
		Strcpy $R0 'XP'
		Goto lbl_done

	lbl_winnt_2003:
		Strcpy $R0 '2003'
		Goto lbl_done

	lbl_error:
		Strcpy $R0 ''
		lbl_done:

	Pop $R1
	Exch $R0
FunctionEnd