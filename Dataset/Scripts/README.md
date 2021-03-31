# Scripts to deploy `SAGE` on EdgeNet

*By Jean-François Grailet (last updated: March 31, 2021)*

## About

This sub-repository provides various bash/Python scripts which are used to conveniently kickstart 
measurements with `SAGE` from the [EdgeNet cluster](https://edge-net.org/). The scripts were 
designed to automate vantage point selection, ensuring a target AS is measured from an exhaustive 
set of vantage points, and carefully checks the state of pods after launching them to know when a 
snapshot is available or when a measurement has failed. These scripts are completed with a logging 
mechanism that both helps the user to know how measurements are going and helps Python scripts to 
avoid EdgeNet nodes where measurements previously failed for a short amount of time.

While these scripts are very convenient to use in practice, they require some set up before using 
them, as well as some knowledge of how [Docker](https://www.docker.com/) and 
[Kubernetes](https://kubernetes.io/) work. More details on the scripts themselves and how to set 
up everything are detailed below.

## Data collection methodology

The scripts provided below are designed to work with a _remote repository_, i.e., a server-like 
machine through which files will transit. In particular, the Kubernetes pods will first download 
target/configuration files from the repository to kickstart a measurement with `SAGE` and will 
upload the resulting snapshot to the same repository. The snapshot will remain on the repository 
until the user downloads it (e.g., by running the `collect.sh` script).

As a consequence, prior to using anything from this sub-repository, one must first ensure it has 
access to a machine which can fulfill the role of the _remote repository_. Such a machine should 
be accessible through SSH without a password, i.e., with a RSA key which will be stored on one's 
computer and in a [Kubernetes secret](https://kubernetes.io/docs/concepts/configuration/secret/) 
which will be used by pods to contact the remote repository.

## Before using the scripts

In addition to finding a machine able to fulfill the role of the _remote repository_ and creating 
a Kubernetes secret, one should also perform the subsequent tasks.

* Create an image with [Docker](https://www.docker.com/). A simple way to achieve this consists 
  in copying the `dockerfile` found in the *Docker/* sub-repository in a new directory, in which 
  you will copy the *Release/* and *src/* folders of `SAGE` source code into a sub-directory 
  *SAGE/*, then running the next command:

```sh
sudo docker build -t username/my-image-name .
```

* Check for all *TODO* comments in the bash scripts, as you need to provide in several scripts: 
  the name of your EdgeNet slice, the data to use your _remote repository_ and the folder where 
  you will store the snapshots.

* Carefully review the Python script `BuildYAML.py` in the *Python/* sub-repository to at least 
  change the image name/pod names/secret names, or change the sequence of commands run by pods.

* Make sure the folder where you will place the bash scripts features sub-folders named after the 
  target ASes, each sub-folder containing the target file (i.e., equivalent to *Dataset/* at the 
  root of this GitHub repository). The *Python/* sub-folder of this sub-repository should be 
  copied as well.

## Python scripts

Three Python scripts are provided to ease some tasks of the main bash scripts, such as the 
computation of a new scheduling (i.e., exclusive pairs of target AS, EdgeNet node).

* `BuildYAML.py` reads the file `template.yaml` and completes it so the resulting YAML can be 
  used to create a new pod.

* `Schedule.py` receives various text files produced by a bash script to know the current use 
  of EdgeNet and schedules new measurements as pairs of target AS, EdgeNet node. The main purpose 
  of this script is to consult the logs to ensure a target AS is not measured with a node it was 
  already measured with as long as there are other options.

* `EvaluateCapacity.py` is a simple utility script to evaluate the total amount of attributable 
  IPs encompassed by the prefixes listed in the target files (i.e., the `.txt` files at the root 
  of AS sub-folders).

## Bash scripts

**Remarks:**
-scripts are listed in lexicographical order, not in the order in which they should be used.
-all scripts can be used without argument; tasks such as listing target ASes are automated.
-some scripts provided optional arguments; check them to learn about it.
-`diagnostic.sh` and `evaluate.sh` are designed to verify scheduling/pods while leaving pods untouched.

* `clean.sh`: deletes currently running pods regardless of their state. Pods to delete can be selected via an optional argument.
* `collect.sh`: checks the current pods, collects the available snapshots and deletes pods that have finished their lifecycle (failed or successful).
* `diagnostic.sh`: thoroughly checks the current pods without collecting measurements.
* `evaluate.sh`: computes a scheduling and displays it, but doesn't apply it.
* `pods.sh`: lists all current pods and displays their name, status and node.
* `prepare.sh`: prepares the remote repository, i.e., creates folder hierarchy and uploads target files (+ side configuration/requirements if any).
* `repository.sh`: checks the remote repository. Specific ASes can be selected to not display the full content.
* `schedule.sh`: computes a scheduling and applies it.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
