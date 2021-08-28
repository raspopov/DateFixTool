[![Build status](https://ci.appveyor.com/api/projects/status/hg993uhq5ha7gnht?svg=true)](https://ci.appveyor.com/project/raspopov/datefixtool)

# Date Fix Tool
A tool for correcting the system date on a computer with the old BIOS.

## Preface

This tool was primarily created for an old notebook HP OmniBook xe4100
(BIOS version KC.M1.27) having a strange time problem after 2020 year,
upon every boot Windows XP drops back to 1920 year but preserving time, day and month
(it's not a CMOS battery problem).

Perhaps the "1920" is a slightly less then POSIX `time()`'s "start of time"
i.e. "1 Jan 1970" and many programs simply crash because of invalid negative time.

The main idea was in powering on the computer at safe year and restoring date
back to correct one during Windows boot.

## Algorithm

Date Fix Tool registered and started as a Windows service and check the current year.
If the year equal or greater than 2020 it does nothing and stop itself.

But if year is between 2020 and 2000 the tool adds 20 and set a new date,
for year below 2000 the tool replaces first two digits to 20 and set a new date also.
So "1921" changed to "2021" and "2001" changed to "2021" too.

After setting a new date the utility keep itself running (as service) until
system shutdown and at the last moment changes the year back subtracting 20
from it (i.e. "2021" became "2001").

At next boot this process repeats.

Diagnostic messages what tool actually performs can be viewed in the Application Log by Event Viewer.

## System Requirements

 - Microsoft Windows XP SP3.
 - Microsoft Visual C++ 2017 Redistributables (included).
 - Administrator rights.
 - No SSE2 CPU required.

## Command line options

Commonly this tool runs as a Windows service silently but also supports the next command line options:

 - `DateFixTool.exe install` - install the service and start it
 - `DateFixTool.exe uninstall` - stop the service and remove it

## Development Requirements

 - [Microsoft Visual Studio 2017 Community](https://aka.ms/vs/15/release/vs_Community.exe) with components:
   - Microsoft.VisualStudio.Workload.NativeDesktop
   - Microsoft.VisualStudio.Component.VC.ATLMFC
   - Microsoft.VisualStudio.Component.WinXP
   - Microsoft.VisualStudio.ComponentGroup.NativeDesktop.WinXP
   - Microsoft.VisualStudio.Component.NuGet
 - NuGet packages:
   - Pandoc.Windows.2.1.0
   - Tools.InnoSetup.5.6.1

## License

Copyright (C) 2021 Nikolay Raspopov <<raspopov@cherubicsoft.com>>

https://github.com/raspopov/DateFixTool

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see <http://www.gnu.org/licenses/>.
