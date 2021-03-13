# PolyGlot

From the [Chess Programming Wiki](https://www.chessprogramming.org/PolyGlot):
> PolyGlot, an adapter that allows UCI engines to use interfaces and GUIs supporting the Chess Engine Communication Protocol, developed by Fabien Letouzey and modified by Fonzy Bluemers. Polyglot is open source, licensed under the GPL, and is able to run on various operating systems, which allows to run UCI engines not only under Windows with its native chess GUIs, but also under Linux and Mac OS using XBoard. PolyGlot 1.4 provides a simplistic opening book implementation, referred as bin-opening book format.

This version here is a fork of v2.0.4 by [H.G. Muller](http://hgm.nubati.net/cgi-bin/gitweb.cgi?p=polyglot.git;a=summary). I have added the following additional PolyGlot options:

* NodesLimit. If set, any ```go``` command is replaced by ```go nodes NodesLimit```.
* DepthLimit. If set, any ```go``` command is replaced by ```go nodes DepthLimit```.
* Movetime. If set, any ```go``` command is replaced by ```go nodes movetime Movetime```.
* AverageMovetime and AverageMovetimeWindow. If set, any ```go``` command is replaced by ```go wtime AverageMovetime*AverageMovetimeWindow btime AverageMovetime*AverageMovetimeWindow movestogo AverageMovetimeWindow```. This allows to achieve an average time per move over a sequence of specified length. By default, AverageMovetimeWindow is set to 10.
* HostCalibration, Hosts, and HostsPerformance. Hosts must be a comma-separated list of host names and HostsPerformance a comma separated list of integers. From this we determine the relative HostPerformanceFactor. If HostCalibration=true, then PolyGlot will determine the host name and all move times are multiplied by HostPerformanceFactor. This allows to ensure similar conditions on different machines.

Moreover, I have added the following additional UCI commands:
* test. This loads (using position fen) a difficult test position (6br/1KNp1n1r/2p2p2/P1ppRP2/1kP3pP/3PBB2/PN1P4/8 w - -), which allows a quick engine test without first searching for an appropriate test position.

## Compilation

Under Linux and macOS the compilation is as usual via ```./configure && make```. Under Windows you need to install [Cygwin](https://www.cygwin.com) together with the packages gcc-core and gcc-g++. Compilation can then be done in the Cygwin terminal via ```make -f makefile.gcc```. You have to add ```C:\cygwin64\bin``` (or whatever your Cywgin installation directory is) to your PATH in order to have all the necessary DLLs available.
