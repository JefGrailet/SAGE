# Dataset

*By Jean-François Grailet (last updated: January 7, 2020)*

## About

The data provided in this repository consists in separate datasets provided for a variety of 
Autonomous Systems (or ASes) for specific dates. Each dataset was obtained by running a single 
instance of `SAGE` on a single PlanetLab node. In order to renew the measurements in a more 
interesting manner, vantage points were rotated during each campaign. The dates of each 
campaign present in this dataset are provided below.

|  Start date  |  End date  |  # probed ASes  |  Number  |
| :----------: | :--------- | :-------------- | :------- |
| 29/12/2019   | ??/01/2020 | 12              | 1        |

The ASes targetted by each campaign as well as the sets of involved PlanetLab nodes can be found 
in the "CampaignX.md" files at the root of this folder, where X is the number of the campaign as 
shown in the above table.

A few remarks about this public dataset:

* Each AS sub-folder contains a target file suffixed in *.txt*. Such a file provides one IPv4
  prefix per line.

* The IPv4 prefixes were obtained via the BGP toolkit of Hurricane Electric. You can access 
  and use the BGP toolkit at the following address:
  
  http://bgp.he.net

* For the sake of reproducibility, we also provide in a **Scripts/** folder the bash scripts and 
  the typical files we used to schedule and retrieve our measurements.

## Composition of each dataset

For each AS sub-folder, you will find one or several *.txt* files listing the IPv4 prefixes 
retrieved with the BGP toolkit from Hurricane Electric along with sub-folders matching the year 
and date of each measurement. Each unique dataset is matched with a sub-path /yyyy/mm/dd/.

### Typical content of a dataset (December 29, 2019 and onwards)

_Coming soon_

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
