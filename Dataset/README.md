# Dataset

*By Jean-François Grailet (last updated: October 12, 2020)*

## About

The data provided in this repository consists in separate sets of files collected for a variety of 
Autonomous Systems (or ASes) at specific dates. Each set of files (or _snapshot_) was obtained by 
running a single instance of `SAGE` from a single vantage point.

A few remarks about this public dataset:

* Each AS sub-folder contains a target file named after the AS and suffixed in *.txt*. Such a file 
  provides all IPv4 prefixes associated to the AS, with one prefix per line.

* An AS sub-folder might also contain similar target file but prefixed with an underscore (_). 
  This is simply because target files for ASes that have been already measured in Spring 2020 have 
  been updated in June 2020. The file without the underscore denotes the current target file, 
  while the target file prefixed by an underscore corresponds to what we used prior to June 2020.

* The IPv4 prefixes were obtained via the BGP toolkit of Hurricane Electric. You can access 
  and use the BGP toolkit at the following address:
  
  http://bgp.he.net

* For the sake of reproducibility, we also provide in a **Scripts/** folder the bash scripts and 
  the typical files we used to schedule and retrieve our measurements.

## About the testbeds and measurement scheduling

The snapshots provided in this repository were collected from two different testbeds: 
[PlanetLab](https://planet-lab.eu/) and [EdgeNet](https://edge-net.org/). Indeed, 
[PlanetLab was terminated during 2020](https://www.systemsapproach.org/blog/its-been-a-fun-ride), 
so the earliest snapshots collected with `SAGE` were still using vantage points from PlanetLab 
(until March 2020 included), but starting from September 2020, all snapshots were collected from 
the EdgeNet cluster.

It is worth noting that, due to the low amount of PlanetLab nodes that were still working for 
the past two years, the measurements run from PlanetLab were organized as campaigns to make 
the most out of the available nodes. During such campaigns, vantage points were rotated between 
each snapshot, therefore ensuring each snapshot for a given AS was collected from a different 
vantage point. This scheduling strategy is actually identical to the strategy used to collect 
data with [`WISE`](https://github.com/JefGrailet/WISE). The dates of each campaign present in 
this dataset are provided below.

|  Start date  |  End date  |  # probed ASes  |  Number  |
| :----------: | :--------- | :-------------- | :------- |
| 29/12/2019   | 15/01/2020 | 12              | 1        |
| 06/03/2020   | 20/03/2020 | 12              | 2        |

The ASes targetted by each campaign as well as the sets of involved vantage points can be found 
in the "PlanetLabCampaignX.md" files at the root of this folder, where X is the number of the 
campaign as shown in the above table.

The snapshots collected from EdgeNet do not follow such a scheme, but our selection of the 
vantage point upon scheduling a measurement typically makes sure the selected EdgeNet node was 
never used before to probe the given AS (or that the amount of snapshots collected from this 
vantage point is the lowest among available nodes).

## Composition of this dataset

For each AS sub-folder, you will find one or several *.txt* files listing the IPv4 prefixes 
retrieved with the BGP toolkit from Hurricane Electric along with sub-folders matching with the 
year and date of each measurement. Each unique set of files (or _snapshot_, see below) is matched 
with a sub-path /yyyy/mm/dd/.

### Typical content

A dataset for a given autonomous system consists of several _snapshots_ which are collections of 
files computed during a single measurement of said AS from a single vantage point (or VP) using a 
single instance of `SAGE`. Each snapshot is isolated by date of measurement and consists of 
exactly 13 files. Each file can be identified by its extension or full name.

**Remark (terminology):** an IP address is considered _lower_ when its 32-bit integer equivalent 
is lower than another address displayed in the same format. In practice, an IP address is lower 
than another when it contains the same first bytes then a lower byte (or when the first byte is 
lower). For example, `1.2.3.5` is lower than both `1.2.3.6` and `2.2.3.5`.

* **.aliases-1 file:** contains all the aliases discovered during the preliminary step of subnet 
  inference (flickering IPs aliasing).
* **.aliases-2 file:** contains all the aliases discovered during the neighborhood graph inference 
  (peer IPs aliasing).
* **.aliases-f file:** contains all the aliases discovered for each neighborhood during the full 
  alias resolution step (contra-pivot/peer IPs aliasing). Such aliases are listed by neighborhood.
* **.fingerprints file:** contains all the fingerprints for each IP involved in alias resolution. 
  Only one fingerprint is given per IP because it's assumed a fingerprint doesn't change over time 
  (contrary to alias resolution hints themselves, in particular IP-IDs).
* **.graph file:** describes the neighborhood graph inferred by `SAGE`. The file can be split in 
  three parts: the first part lists all vertices along with the label of their respective 
  neighborhood, the second part describes the edges and the third and last part lists the 
  miscellaneous routes which implements _remote_ links (i.e. paths between two neighborhoods 
  located more than one hop from each other, for which no intermediate neighborhood could be 
  discovered).
* **.hints file:** contains the alias resolution hints collected for each IP involved in alias 
  resolution. Each line starts with the IP suffixed by the algorithmical step during which the 
  hints were collected, both as an additional piece of information and in order to discriminate 
  sets of hints for a same IP from each other.
* **.ips file:** lists all IPs that were discovered during pre-scanning along their associated
  data (i.e., IP dictionary), with the _lowest_ IPs coming first. Special IPs (e.g. IPs which 
  appeared at different hop counts) are annotated with additional details.
* **.metrics file:** provides an early set of metrics on the graph inferred by `SAGE`, such as 
  the maximum degree of a vertice, as well as a short description of the connected components 
  (plus how many vertices they gather as a ratio).
* **.neighborhoods file:** lists all the neighborhoods that were inferred along with their 
  associated subnets, ordered w.r.t. their (first) label IP address (i.e. _lowest_ IPs first), 
  with the neighborhoods based on incomplete or echoing _trails_ coming first (_lowest_ IPs then 
  lowest anomaly or pre-echoing counts first).
* **.peers file:** contains the results of the partial (Paris) `traceroute` measurements used to 
  perform neighborhood inference.
* **.subnets file:** lists all the subnets that were inferred along with their responsive 
  interfaces, ordered w.r.t. their prefix IP (i.e. _lowest_ prefix IP addresses first).
* **.txt file:** gives the details about how the measurement went, e.g., it gives the detailed 
  amount of probes used by each phase along the time they took for completion. This corresponds, 
  in fact, to the console output of `SAGE`.
* **VP.txt:** gives the host name of the **v**antage **p**oint used to measure the AS on that 
  specific date.

In rare instances, these files might be complemented by one additional **README** file in case 
there was something unusual about the measurement (such as having to restart it due to a temporar 
failure).

## Special remarks

* Up to late September 2020, the `.graph` files might list (in)direct links with a _medium_ (i.e., 
  a subnet which might be the real-life link) connecting a neighborhood `u` to a neighborhood `v` 
  where `v` is bordered by subnets built with the third rule of subnet inference (_echoing_ 
  trails). Such links shouldn't actually have a _medium_ at all since the last route hops towards 
  subnets like those of `v` are _echoing_ and therefore hiding the actual router interface. While 
  parsing a `.graph`, one can therefore ignore the _medium_ associated to any link pointing 
  towards a neighborhood comparable to `v`. As of October 12, 2020, the public code of `SAGE` no 
  longer searches a _medium_ for such links.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
