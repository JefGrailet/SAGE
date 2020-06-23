# About SAGE v2.0 (sage)

*By Jean-François Grailet (last updated: June 23, 2020)*

## About the code

Since it is built on top of `WISE`, which initially needed to be compatible with old environments (e.g. Fedora 8, see remarks on deployment below), `SAGE` is written in an _old-fashioned_ C++. In other words, it doesn't take advantage of the changes brought by C++11 and onwards. As already said elsewhere, it is also designed to be a 32-bit application for the same reason. This said, after several campaigns run from the PlanetLab testbed towards all kinds of target networks without getting a suspicious crash, it is safe to assume `SAGE` is unlikely to mismanage memory. On top of that, it has been extensively tested with `valgrind` on a local network.

## Compilation

You will need gcc and g++ on your Linux distribution to compile `SAGE` (`sage`). To compile it, set *Tool/Release/* as your working directory and execute the command:

```sh
make
```

If you need to recompile `SAGE` after some editing, type the following commands:

```sh
make clean
make
```

## Deployement on PlanetLab testbed

**Remark: in the course of 2020, [PlanetLab will be shut down](https://www.systemsapproach.org/blog/its-been-a-fun-ride). This section is therefore now outdated, but left untouched in case one would want to deploy `WISE` in an environment comparable to PlanetLab.**

If you intent to use `SAGE` from the PlanetLab testbed, here is some advice.

* Do not bother with compiling `SAGE` on PlanetLab nodes and rather compile it on your own computer. Then, you can upload the executable file (found in *Release/*) on a PlanetLab node and uses it as soon as you connect to it.

* Of course, your executable should be compiled with an environement similar to that of the PlanetLab nodes. The oldest OS you should find on a PLC (PlanetLab Central) node is usually Fedora 8 (at the time this file was written). A safe (but slow) method to compile `SAGE` for Fedora 8 and onwards is to run Fedora 8 as a virtual machine, put the sources on it, compile `SAGE` and retrieve the executable file. Note that most if not all PLC nodes are 32-bit machines.

* PLE (PlanetLab Europe) nodes uses 64-bit versions of much more recent releases of Fedora (e.g., Fedora 24). To run `SAGE` as compiled for PLC nodes on PLE nodes, if the PLE node cannot run `SAGE` yet, check what libraries it currently has and copy the 32-bit libraries from the PLC nodes on it (usually, these libraries are just missing). Thanks to this trick, you will be able to run `SAGE` as compiled for PLC nodes. Make sure, however, to double-check what 32-bit libraries are already available on the PLE nodes to not overwrite existing libraries.

## Usage

`SAGE` v2.0 will describe in details its options, flags and how you can use it by running the line:

```sh
./sage -h
```

## Configuration files

In order to simplify the parameters of `SAGE` and only allow the editing of the most important parameters in command-line, specific probing parameters are only editable via configuration files. You can find examples of such configuration files in *Release/config/* (with the default configuration of `SAGE`).

## Remarks

* Most machines forbid the user to open sockets to send probes, which prevents `SAGE` from doing anything. To overcome this, run `SAGE` as a super user (for example, with `sudo`).

* Most of the actual code of `SAGE` is found in *src/algo/* (in *Tool/* sub-directory). *src/prober/* and *src/common/* provides libraries to handle (ICMP, UDP, TCP) probes, IPv4 addresses, etc. If you wish to build a completely different application using ICMP/UDP/TCP probing, you can take the full code of ``SAGE`` and just remove the *src/algo/* folder.

* If you intend to remove or add files to the source code, you have to edit the subdir.*mk* files in *Release/src/* accordingly. You should also edit *makefile* and sources.*mk* in *Release/* whenever you create a new (sub-)directory in *src/*.

## Changes history

* **January 7, 2020:** release of `SAGE` v2.0.

## Disclaimer

`SAGE` was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). `SAGE` itself is built on top of `WISE`, a subnet inference tool designed and implemented by the same author. For more details on `WISE`, check its dedicated public repository:

https://github.com/JefGrailet/WISE

It's worth noting `SAGE` is actually in its second version, because an earlier build exists. This version is rather built on top of [`TreeNET`](https://github.com/JefGrailet/TreeNET) rather than on top of `WISE`, but uses roughly the same ideas (in a less refined manner). You can learn about this _beta_ version by consulting the dedicated public repository:

https://github.com/JefGrailet/SAGE_beta

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `SAGE`. I am also inclined to answer questions regarding the algorithms used in `SAGE` and to discuss its application in other research projects.
