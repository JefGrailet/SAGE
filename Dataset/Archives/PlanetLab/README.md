# Archives regarding deployment of SAGE on the PlanetLab testbed

*By Jean-François Grailet (last updated: March 31, 2021)*

## Important remark

**The PlanetLab testbed is no longer relevant**, as explained 
[in this short article](https://www.systemsapproach.org/blog/its-been-a-fun-ride), and you might 
want to now use the [EdgeNet](https://edge-net.org/) cluster to schedule new measurement campaigns 
with SAGE or other tools (EdgeNet being a kind of "spiritual successor" to PlanetLab). Therefore, 
the scripts provided below are, in a way, deprecated. However, you could still want to use them 
if you had to deploy SAGE on a bunch of remote machines you can access through SSH, because 
there's most probably little to change in the provided scripts, with the obvious exceptions of the 
paths to the various folders of your computer and perhaps your log in data. Obviously, you will 
also have to change the host names of the machines listed in the `Example_nodes` file (see below).

## Content

This sub-repository provides the following content:

* the scripts we used up to March 2020 to schedule measurements with SAGE from PlanetLab,

* a summary of the campaigns we conducted as `.md` files.

The files contained in this folder are the scripts and files we typically use to schedule and run 
a measurement campaign on the PlanetLab testbed.

## About the scripts

The scripts provided in this folder are rather generic, as the only prerequisite is to have the 
right SSH key to log into PlanetLab nodes through SSH. As a consequence, they could be easily 
re-used to manage a deployment of `SAGE` from a set of machines you can log into via SSH.

Most of the scripts can be re-used _as is_, while others will require some light edition in order 
to provide the right paths towards the folder in your computer where you stored your PlanetLab 
executable of `SAGE` and the folder where you want to store the measurements. Note that such 
folder should already contain a folder for each AS you want to measure, each of these folders 
containing also the *.txt* file providing the target prefixes.

Here's a short review of the scripts and the other relevant files:

* `Example_targets`: a sample text file listing the ASes to measure. Each line provides an AS.

* `Example_nodes`: another sample text file which rather provides the set of PlanetLab nodes to 
  use during a campaign. Each line provide a single node.

* `ASes_check.sh`: a script that contact each remote PlanetLab node to print the content of the 
  *SAGE/* subfolder (automatically created by the preparation script, see below) and check whether 
  some measurement is complete (typically, a measurement is complete when the *.metrics* output 
  file is available). You have to provide the file listing the nodes to run this script.

* `ASes_get.sh`: a script you will use to download the datasets once all measurements have been 
  completed. You need to provide both the file listing target ASes and the file listing the 
  PlanetLab nodes to run this script.

* `ASes_prepare.sh`: a script you will use to prepare the PlanetLab nodes to start new 
  measurements. The script automatically creates a *SAGE/* folder on the remote node, and uploads 
  in it every file needed to start a measurement. You need to provide both the file listing target 
  ASes and the file listing the PlanetLab nodes to run this script.

* `ASes_start.sh`: a script that starts new measurements from the PlanetLab testbed. Just like 
  the two previous scripts, you have to provide both the file listing target ASes and the file 
  listing the PlanetLab nodes to run this script. Note that in some cases, the program might not 
  actually start running due to the specific settings of the node, but a file with the AS number 
  and the date will be created anyway (though it will have a size of 0 octet). In such a 
  situation, you have to log yourself on the node to run the command starting the measurement.

**Remark:** don't forget to have a quick look at the scripts to change the paths towards files 
or folders to match your own filesystem if you intend to deploy `SAGE` by re-using these scripts. 
In particular, you should at least modify `ASes_get.sh` and `ASes_prepare.sh`.

## About the campaigns and vantage point (VP) rotation

The campaign summaries are named `PlanetLabCampaignX.md` where X is an integer (chronological 
order of the campaigns). They simply recap the start and end dates, the set of target ASes as well 
as the set of PlanetLab nodes and their authority. The corresponding data can be found in the AS 
folders at the root of *Dataset/* between the provided dates. The dates and number of each 
PlanetLab campaign with `SAGE` are provided below.

|  Start date  |  End date  |  # probed ASes  |  Number  |
| :----------: | :--------- | :-------------- | :------- |
| 29/12/2019   | 15/01/2020 | 12              | 1        |
| 06/03/2020   | 20/03/2020 | 12              | 2        |

It's worth noting that, due to the low amount of nodes in use, the campaigns used vantage point 
(VP) rotation to make the most of each node while having data for each target AS from distinct 
vantage points. I.e., all ASes were measured once each from a specific node, then they were 
measured again from a different node from which they haven't been measured yet by simply shifting 
the list of PlanetLab nodes a certain amount of times.

This is achieved by our scripts in practice with an optional parameter (available in all scripts) 
called _rotation number_ that is basically a value added to each index in the list of PlanetLab 
nodes before doing a modulo operation in order to get the shift. By using an increasing rotation 
number at each measurement, you can accomplish a full rotation of the VPs.

For instance, if you want to measure 10 ASes from 10 VPs with rotations, you can run all scripts 
at first without a rotation number (because it's the initial setting), then re-run them for the 
next measurements by providing rotation numbers 1, 2, 3... up to 9 (at 10 or more, you will 
cycle).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
