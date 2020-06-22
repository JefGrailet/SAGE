# SAGE (Subnet AGgrEgation)

*By Jean-François Grailet (last updated: June 22, 2020)*

## Overview

`SAGE` is new topology discovery tool built on top of `WISE` which relies on subnet aggregation and partial `traceroute` records to build a graph modeling the target domain.

`SAGE` is fully working but just got deployed. As such, only the source files (written in C/C++) and a few datasets are currently available.

## About the code

Since it is built on top of `WISE`, which initially needed to be compatible with old environments (e.g. 32-bit machines from the [PlanetLab testbed](https://planet-lab.eu/) running with Fedora 8), `SAGE` is written in an _old-fashioned_ C++. In other words, it doesn't take advantage of the features of the language brought by C++11 and onwards. This said, after several campaigns run from the PlanetLab testbed towards all kinds of target networks without getting a suspicious crash, it is safe to assume `SAGE` is unlikely to mismanage memory. It has been, on top of that, extensively tested with `valgrind` on a local network.

## Content of this repository

This repository currently consists of the following content:

* **Dataset/** provides complete datasets for various Autonomous Systems (or ASes) we measured with `SAGE` from the PlanetLab testbed.

* **src/** provides all the source files of `SAGE`.

## Legacy

It is worth noting that an early version of `SAGE` (v1.0) has already been designed and implemented back in late 2017. This early version was built using parts of another topology discovery tool involving subnet inference (`TreeNET`), as `WISE` was developed in late 2018 and deployed in 2019.

This early version of `SAGE` became publicly available online via a GitHub repository created during the first half of 2018. This repository still exists but has been renamed to avoid confusion. You can therefore still review it by following the next link:

https://github.com/JefGrailet/SAGE_beta

## Disclaimer

`SAGE` was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). `SAGE` itself is built on top of `WISE`, a subnet inference tool designed by the same author. For more details on `WISE`, check the [`WISE` public repository](https://github.com/JefGrailet/WISE).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `SAGE`. I am also inclined to answer questions regarding the algorithms used in `SAGE` and to discuss its application in other research projects.
