.\gradlew --stop
choco install ant
choco install vswhere
choco install zip
if ($env:APPVEYOR -ne "true") {
  choco install visualstudio2017community
  choco install visualstudio2017-workload-nativedesktop
  choco install windows-sdk-7.1
  choco install cygwin

  $cygwinPath = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Cygwin\setup").rootdir
  $env:Path += ";$($cygwinPath)\bin"
}

$env:WINSDK_DIR = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Windows Kits\Installed Roots").KitsRoot10
$vsRoot = "$(vswhere -legacy -latest -property installationPath)"
$env:VCINSTALLDIR = "$($vsRoot)\VC\Auxiliary\Build"
$msvcToolsVer = Get-Content "$env:VCINSTALLDIR\Microsoft.VCToolsVersion.default.txt"
$msvcRedistVer = Get-Content "$env:VCINSTALLDIR\Microsoft.VCRedistVersion.default.txt"
if ([string]::IsNullOrWhitespace($msvcToolsVer)) {
  # The MSVC tools version file can have txt *or* props extension.
  $msvcToolsVer = Get-Content "$env:VCINSTALLDIR\Microsoft.VCToolsVersion.default.props"
}
$env:MSVC_VER = $msvcToolsVer
$env:MSVC_REDIST_VER = $msvcRedistVer

$files = Get-ChildItem "$($vsRoot)\VC\Redist\MSVC\$($msvcRedistVer)\x86\*.CRT\"
$env:WINDOWS_CRT_VER = $files[0].Name.replace("Microsoft.VC","").replace(".CRT","")

$env:VS150COMNTOOLS = $env:VCINSTALLDIR
$env:VSVARS32FILE = "$env:VCINSTALLDIR\vcvars32.bat"
refreshenv
if ($env:APPVEYOR -eq "true") {
  .\gradlew all test -PCOMPILE_WEBKIT=false -PCONF=DebugNative --stacktrace -x :web:test --info --no-daemon
  if ($lastexitcode -ne 0) {
    exit $lastexitcode
  }
} else {
  .\gradlew all test -PCOMPILE_WEBKIT=false -PCONF=Debug --stacktrace -x :web:test --info
}
