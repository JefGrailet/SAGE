# Scripts to schedule measurements with SAGE on PlanetLab

*By Jean-François Grailet (last updated: June 22, 2020)*

## Important remark

**The PlanetLab testbed will soon be terminated**, as explained 
[in this short article](https://www.systemsapproach.org/blog/its-been-a-fun-ride), and you might 
want to use the [EdgeNet](https://edge-net.org/) testbed to schedule new measurement campaigns 
with SAGE or other tools (EdgeNet being a kind of "spiritual successor" to PlanetLab). Therefore, 
the scripts provided below are, in a sense, deprecated. However, you could still want to use them 
if you had to deploy SAGE on a bunch of remote machines you can access through SSH, because 
there's most probably nothing to change in the provided scripts, except the paths to the various 
folders of your computer and perhaps your log in data. Obviously, you will also have to change the 
host names of the machines listed in the `Example_nodes` file.

## About

The files contained in this folder are the scripts and files we typically use to schedule and run 
a measurement campaign on the PlanetLab testbed. You can re-use some of these scripts _as is_, 
while others will require some light edition in order to provide the right paths towards the 
folder in your computer where you stored your PlanetLab executable of `SAGE` and the folder where 
you want to store the measurements. Note that such folder should already contain a folder for each 
AS you want to measure, each of these folders containing also the *.txt* file providing the target 
prefixes.

Here's the content of this folder in details:

* `Example_targets`: a simple text file that provides the ASes you want to measure. Each line 
  provides an AS.

* `Example_nodes`: another text file which rather provides the set of PlanetLab nodes you want 
  to use for the campaign. Each line provide a single node.

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

## Rotating vantage points (or VPs)

Provided scripts were designed to ease the rotation of vantage points in order to study a same 
network from different VPs and see how the changes affect the collected data. Each of the scripts 
has an additional but optional parameter called _rotation number_, which is basically a value you 
will add to each index before doing a modulo operation in order to get the shift towards another 
VP. By using an increasing rotation number at each measurement, you can accomplish a full rotation 
of the VPs.

For instance, if you want to measure 10 ASes from 10 VPs with rotations, you can run all scripts 
at first without a rotation number (because it's the initial setting), then re-run them for the 
next measurements by providing rotation numbers 1, 2, 3... up to 9 (at 10 or more, you will 
cycle).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
