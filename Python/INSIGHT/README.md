# INSIGHT - Investigate Networks from Subnet Inference to GrapH Transformations

*By Jean-François Grailet (last updated: October 23, 2020)*

## About

**N.B.:** if you reached this part of the repository via a direct link, please note that this 
sub-folder doesn't provide the data collected by `SAGE` nor its source code. You will rather find 
them respectively in the **Dataset/** and **src/** sub-folders at the root of this repository.

`INSIGHT` (which stands for _**I**nvestigate **N**etworks from **S**ubnet **I**nference to 
**G**rap**H** **T**ransformations_) is a small Python library devised for parsing and transforming 
the data collected by `SAGE` in order to model and analyze the measured networks. It relies on the 
[NetworkX Python library](https://networkx.org/) in order to build graphs (directed or bipartite), 
extract their metrics and render them visually. In addition, `INSIGHT` comes with a module for 
plotting the metrics it could compute thanks to NetworkX.

Running `INSIGHT` on a network snapshot collected by `SAGE` (i.e., the entire set of files 
produced by `SAGE` in a single run while probing a single target network) will help users to 
visualize it and study the main characteristics of the corresponding network.

## Content

All the source files (in Python) of `INSIGHT` are located in this sub-folder. The **Results/** 
sub-folder provides pre-computed figures and plots for each snapshot found in the dataset. The 
file hierarchy of this sub-folder matches the target network and the date of the snapshot (e.g., 
for figures computed with the snapshot of AS1241 collected on 12/09/2020, go to 
`Results/AS1241/2020/09/12/`).

## Usage

You can run `INSIGHT` on a single snapshot with the following command:

```sh
python INSIGHT.py [Mode] [Snapshot prefix] [Output files prefix]
```

where:
* **Mode** is the type of graph you will use. Current modes are `raw` (for raw directed graph) and 
  `bip` (for simple bipartite graph). Append this mode with `+details` to get additional 
  information in the terminal.
* **Snapshot prefix** is the complete path (absolute or relative) to the files of a network 
  snapshot where all output files only differ by extension. E.g., if you want to use a snapshot 
  from this repository, e.g. AS1241 on 12/09/2020, the snapshot prefix will be 
  `/path/to/repository/Dataset/AS1241/2020/09/12/AS1241_12-09`.
* **Output files prefix** is the prefix for the output files that will be generated. Typically, 
  for `bip` mode, `INSIGHT` creates 4 PDF files (3 plots and one render of the graph).

Alternatively, you can also run the `process.sh` Shell script to run `INSIGHT` on multiple 
snapshots, as long as you modify the `datasetPath` variable at the start of the script to match 
your file system. Then, you can use this kind of command:

```sh
./process.sh [AS number]
```

where **AS number** is the name of an AS (**A**utonomous **S**ystem) present in the dataset (e.g. 
AS13789). This will run `INSIGHT` on all snapshots for this AS (except snapshots that have been 
already processed) and move the output files to a `Results/AS number/yyyy/mm/dd/` sub-folder 
created by the script. Note that this script also comes with two options you can check by typing 
`./process.sh` without arguments in your terminal.

## Changes history

* **October 23, 2020:** first public version of `INSIGHT`. Handles directed graphs (raw 
  translations of graphs built by `SAGE`) and simple bipartite graphs (neighborhood - subnet).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
