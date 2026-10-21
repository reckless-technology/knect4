# connect4

Connect Four for Windows: a Windows Forms game against the computer, written in C# in 2014.

It is one executable for the .NET Framework 4.8, which every Windows 10 and 11 carries, so it runs with nothing
installed: download `connect4-windows` from the latest run of the `build` workflow and start `ConnectFour.exe`.

## Building

With the .NET SDK (any version that builds `net48`; CI uses 10) on Windows:

```
dotnet build ConnectFour.sln -c Release      # bin/Release/net48/ConnectFour.exe
```

The project fetches the .NET Framework's reference assemblies as a package
(`Microsoft.NETFramework.ReferenceAssemblies`), so no Visual Studio targeting pack is needed. In VS Code, the default
build task builds the Debug configuration and the "Connect Four" launch entry builds and starts it under the C#
extension's debugger; both are Windows only.

## CI

`.github/workflows/build.yml` builds the Release configuration on every push to `main`, every pull request and by hand,
on the organisation's self-hosted Windows runners (the repository is private), and keeps `ConnectFour.exe` with its
`.config` as the run's artifact `connect4-windows` for 30 days.

## History

The game began in the `software` repository, under `ConnectFour/`; it was extracted with its history by
git-filter-repo. It targeted the .NET Framework 2.0 there; the project file is now SDK-style for the .NET Framework 4.8,
which the current tools build, with the same code.
