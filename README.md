# SAGE (Subnet AGgrEgation)

*By Jean-François Grailet (last updated: October 12, 2020)*

## Overview

`SAGE` is new topology discovery tool built on top of [`WISE`](https://github.com/JefGrailet/WISE) (a subnet inference tool) which relies on subnet aggregation and partial (Paris) `traceroute` measurements to build a directed graph modeling the target domain. In this graph, vertices model _neighborhoods_, i.e., network locations bordered by a set of subnets located at most one hop away from each other. Such neighborhoods are in practice approximations of routers or meshes of routers and can be elucidated through alias resolution. Neighborhoods are inferred then located w.r.t. each others using partial `traceroute` records towards a subset of interfaces of each inferred subnet. The edges in the graph model how neighborhoods appear to be connected, following the `traceroute` measurements, and are ideally matched with surrounding subnets, as subnets act as a connection medium between neighborhoods (and, _in fine_, routers).

`SAGE` being involved in ongoing research, this repository only provides the source files (written in C/C++) and a first dataset for now. Said dataset provides AS (for **A**utonomous **S**ystem) snapshots collected from both [PlanetLab](https://planet-lab.eu/) and [EdgeNet](https://edge-net.org/).

## About development

### About the code

Since it is built on top of `WISE`, which initially needed to be compatible with old environments (e.g. 32-bit machines from the [PlanetLab testbed](https://planet-lab.eu/) running with Fedora 8), `SAGE` is currently written in an _old-fashioned_ C++. In other words, it doesn't take advantage of the features of the language brought by C++11 and onwards. This said, after several campaigns run from the PlanetLab testbed towards all kinds of target networks without getting a suspicious crash, it is safe to assume `SAGE` is unlikely to mismanage memory. On top of that, it has been extensively tested with `valgrind` on a local network.

### Future updates

`SAGE` itself will probably not get any large update for a while, though minor issues will be corrected from time to time. However, side programs (such as Python scripts) will eventually be released to provide ways to parse, interpret and analyze the data collected by `SAGE`.

## Publications

### Preliminary research

While there is currently no publication mentioning or introducing `SAGE`, one peer-reviewed publication already discusses the core ideas used to build `SAGE` (i.e., _neighborhoods_ and their _peers_), using an upgraded version of `WISE` to collect the data we needed to do so.

* [Virtual Insanity: Linear Subnet Discovery](http://www.run.montefiore.ulg.ac.be/~grailet/docs/publications/WISE_TNSM_2020.pdf)<br />
  Jean-François Grailet, Benoit Donnet<br />
  [IEEE Transactions on Network and Service Management](https://www.comsoc.org/publications/journals/ieee-tnsm), Volume 17, Issue 2, pp. 1268-1281 ([cf. IEEE Xplore](https://ieeexplore.ieee.org/document/9016121))

## Content of this repository

This repository currently consists of the following content:

* **Dataset/** provides complete datasets for various Autonomous Systems (or ASes) we measured with `SAGE` from the PlanetLab testbed.

* **src/** provides all the source files of `SAGE`.

## Beta version

It is worth noting that an early version of `SAGE` (v1.0) has already been designed and implemented back in late 2017. This early version was built using parts of another topology discovery tool involving subnet inference ([`TreeNET`](https://github.com/JefGrailet/treenet)), as `WISE` was developed in late 2018 and deployed in 2019.

This early version of `SAGE` became publicly available online via a GitHub repository created during the first half of 2018. This repository still exists but has been renamed to avoid confusion. You can therefore still review it by following the next link:

https://github.com/JefGrailet/SAGE_beta

## Disclaimer

`SAGE` was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). `SAGE` itself is built on top of `WISE`, a subnet inference tool designed by the same author. For more details on `WISE`, check the [`WISE` public repository](https://github.com/JefGrailet/WISE).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `SAGE`. I am also inclined to answer questions regarding the algorithms used in `SAGE` and to discuss its application in other research projects.
