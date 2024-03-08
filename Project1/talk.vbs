'variables
Dim  oVoice


Set oVoice = CreateObject("SAPI.SpVoice")
Set oVoice.Voice = oVoice.GetVoices.Item(0)
oVoice.Rate = 0
oVoice.Volume = 100
oVoice.Speak "open"

oVoice.Speak "close"


