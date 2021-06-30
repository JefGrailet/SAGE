# Miscellaneous notes about measurements

*By Jean-François Grailet (last updated: June 30, 2021)*

## About

This file provides a variety of remarks about the measurements performed with `SAGE` from the 
EdgeNet cluster. These notes are provided in chronological order.

## Notes

* **April 1, 2021:** measurements of AS8928 have ceased, due to all measurements from the 
  EdgeNet cluster failing to capture an exhaustive topology like snapshots previously captured 
  from the PlanetLab testbed.

* **April 6, 2021 (I):** three new ASes are now part of the measurement schedule: AS1273 (Vodafone 
  Group PLC), AS4637 (Telstra Global), and AS6461 (Zayo Bandwith). Target files have also been 
  refreshed for all ASes. 

* **April 6, 2021 (II):** measurements of AS6453 and AS3257 are now performed with _strict_ alias 
  resolution. I.e., during subnet inference and neighborhood inference (i.e., building the 
  neighborhood-based directed acyclic graph), `SAGE` only accepts alias pairs/lists discovered via 
  the `iffinder` methodology or with IP-ID-based alias resolution methods. This mode can be set by 
  using a specific configuration file; see also `SAGE/src/Tool/Release/config/strictAR.cfn` (last 
  line). This change was performed due to finding, in both ASes, a large number of alias 
  candidates for which IP-ID-based alias resolution methods are well suited. Any snapshot of these 
  ASes captured after April 6, 2021 was therefore obtained while using strict alias resolution in 
  order to maximize the accuracy of the discovered alias pairs/lists.

* **April 15, 2021:** measurements of AS286, AS1273 and AS6461 are now also using the _strict_ 
  alias resolution for the same reason as before (see April 6, 2021).

* **June 18, 2021:** all target files have been refreshed.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
