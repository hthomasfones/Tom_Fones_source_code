REM Note that missing classes in log file mean tthe hat WMI cannot access them.
REM Most likely this indicates a problem with the driver.
REM See %windir%\system32\wbem\wmiprov.log and nt eventlog for more details.
REM You could also delete the line On Error Resume Next and examine the
REM specific VBScript error


On Error Resume Next

Set fso = CreateObject("Scripting.FileSystemObject")
Set a = fso.CreateTextFile(".log", True)
Set Service = GetObject("winmgmts:{impersonationLevel=impersonate}!root/wmi")
Rem MadBusWmiInfo - Mad Bus driver information
Set enumSet = Service.InstancesOf ("MadBusWmiInfo")
a.WriteLine("MadBusWmiInfo")
for each instance in enumSet
    a.WriteLine("    InstanceName=" & instance.InstanceName)
    a.WriteLine("        instance.ErrorCount=" & instance.ErrorCount)
    a.WriteLine("        instance.CurrDevCount=" & instance.CurrDevCount)
    a.WriteLine("        instance.TotlDevCount=" & instance.TotlDevCount)
next 'instance

Rem MadBusWmiControl - WMI method
Set enumSet = Service.InstancesOf ("MadBusWmiControl")
a.WriteLine("MadBusWmiControl")
for each instance in enumSet
    a.WriteLine("    InstanceName=" & instance.InstanceName)
    a.WriteLine("        instance.ControlValue=" & instance.ControlValue)
next 'instance

a.Close
Wscript.Echo " Test Completed, see .log for details"
