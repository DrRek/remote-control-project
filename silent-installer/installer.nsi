Name "Silent"
RequestExecutionLevel user

SilentInstall silent

OutFile "prova.exe"
InstallDir $DESKTOP\prova

Section
	SetOutPath $INSTDIR
	File main.exe
	;sc create secondtest binpath= C:\Users\Luca\Desktop\prova\main.exe start= auto type= own DisplayName= "Seconda prova"
	nsExec::Exec '"C:\Windows\System32\sc.exe" create fourthtest  start=auto binPath="$INSTDIR\main.exe" DisplayName= "Quarta prova" type= own'
SectionEnd