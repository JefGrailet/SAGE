# SAGE (Subnet AGgrEgation)

*By Jean-François Grailet (last updated: December 5, 2021)*

## Overview

`SAGE` is a topology discovery tool built on top of [`WISE`](https://github.com/JefGrailet/WISE) (a subnet inference tool) which relies on subnet aggregation and partial (Paris) `traceroute` measurements to build a directed graph modeling the hop-level of a target domain. In this graph, vertices model _neighborhoods_, i.e., network locations bordered by a set of subnets located at most one hop away from each other. Such neighborhoods are in practice individual routers or meshes of routers and can be elucidated through alias resolution. Neighborhoods are inferred using subnet-level data first collected with `WISE`, then located w.r.t. each others using partial `traceroute` records towards a subset of interfaces of each inferred subnet. The edges in the graph model how neighborhoods appear to be connected, based on the `traceroute` data, and are mapped (when possible) with surrounding subnets, as subnets typically act as a connection medium between neighborhoods (and, _in fine_, routers). Graphs as built by `SAGE` can be translated into bipartite graphs to deepen the study of the measured networks.

This repository provides all at once the source files (C/C++) of `SAGE`, a dataset and Python scripts to visualize or evaluate said data. The dataset consists of AS (for **A**utonomous **S**ystem) snapshots collected from both the [PlanetLab testbed](https://planet-lab.eu/) and the [EdgeNet cluster](https://edge-net.org/).

## About development

### About the code

Since it is built on top of `WISE`, which initially needed to be compatible with old environments (e.g. 32-bit machines from the [PlanetLab testbed](https://planet-lab.eu/) running with Fedora 8), `SAGE` is currently written in an _old-fashioned_ C++. In other words, it does not take advantage of the features of the language brought by C++11 and onwards. This said, after several campaigns run from the PlanetLab testbed towards all kinds of target networks without getting a suspicious crash, it is safe to assume `SAGE` is unlikely to mismanage memory. On top of that, it has been extensively tested with `valgrind` on a local network.

### Future updates

`SAGE` itself will not get any large update for a while, though minor issues will be corrected from time to time. However, side Python scripts, such as the `INSIGHT` library, will be expanded over time to provide more ways to parse, model and analyze the data collected by `SAGE`.

## Publications

* [Travelling Without Moving: Discovering Neighborhood Adjacencies](http://www.run.montefiore.ulg.ac.be/~grailet/docs/publications/SAGE_TMA_2021.pdf)<br />
  Jean-François Grailet, Benoit Donnet<br />
  [Network Traffic Measurement and Analysis Conference (TMA) 2021](http://tma.ifip.org/2021/), Online, 14 and 15/09/2021

### Related work

The following publications can be consulted by interested readers to learn more about `SAGE`. The sixth section of the first paper describes and assesses the core ideas used to design `SAGE` (i.e., _neighborhoods_ and their _peers_). The second publication is my doctoral thesis and provides, among others, a detailed description of both `WISE` and `SAGE`. People who want to re-use `WISE`/`SAGE` algorithms might want to read chapters 7 (`WISE`) and 11 (`SAGE`).

* [Virtual Insanity: Linear Subnet Discovery](http://www.run.montefiore.ulg.ac.be/~grailet/docs/publications/WISE_TNSM_2020.pdf)<br />
  Jean-François Grailet, Benoit Donnet<br />
  [IEEE Transactions on Network and Service Management](https://www.comsoc.org/publications/journals/ieee-tnsm), Volume 17, Issue 2, pp. 1268-1281 ([cf. IEEE Xplore](https://ieeexplore.ieee.org/document/9016121))

* [Efficient Multi-Level Measurements and Modeling of Computer Networks](http://www.run.montefiore.ulg.ac.be/~grailet/docs/thesis/EfficientMultiLevelMeasurementsAndModelingOfComputerNetworks.pdf)<br />
  Jean-François Grailet<br />
  Doctoral thesis (available on [ORBi](https://orbi.uliege.be/handle/2268/262826))

## Content of this repository

This repository currently consists of the following content:

* **Dataset/** provides complete datasets for various Autonomous Systems (or ASes) we measured with `SAGE` from both the PlanetLab testbed and the EdgeNet cluster.

* **Python/** provides various Python scripts, most notably the `INSIGHT` library, to evaluate or visualize the data collected by `SAGE`.

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
