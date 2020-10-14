# Evaluation of graph isomorphism

*By Jean-François Grailet (last updated: October 14, 2020)*

## About

**N.B.:** if you reached this part of the repository via a direct link, please note that this 
sub-folder doesn't provide the data collected by `SAGE`. You will rather find it in the 
*Dataset/* sub-folder located at the root of this repository.

You can use the content of this folder in order to evaluate the isomorphism of graphs, i.e., 
evaluate how similar two graphs **of a same target network** are. The purpose of the provided 
scripts is to verify whether graphs of a same target network collected from different vantage 
points (and on different dates) share similarities or not. We are in particular interested by 
three metrics.

* **Redundant edges ratio:** this metric expresses the amount of edges that exist in two given 
  graphs with respect to the amount of edges that _can_ exist in both graphs, i.e., the vertices 
  connected by an edge of the first graph also exist in the second graph. A high ratio of 
  redundant edges mean that `SAGE` discovered the same links for all vertices that appears in 
  both graphs despite the change of vantage point. A low ratio means that `SAGE` fails to 
  recognize the same links despite that the target network is the same.

* **Common vertices ratio:** this metric expresses how many vertices present in the first graph 
  also appears in the second graph, with respect to the total amount of vertices that exist in 
  the first graph. A high ratio means that `SAGE` could discover the same vertices despite the 
  change of vantage point.

* **Intersection of vertices:** when reviewing several graphs together (comparing the first 
  graph of the bunch with all subsequent graphs), this metric expresses the ratio of vertices 
  from the first graph that always reappear in the subsequent graphs. When compared with the 
  common vertices ratio for each graph, this metric hints the amount of unique vertices that 
  are discovered with each graph. A small intersection (expressed by a low ratio) with high 
  common vertices ratios suggests that `SAGE` discovers consistently the same target network 
  despite the change of vantage point, but gets a slightly different picture for each vantage 
  point.

There are three scripts to review these metrics.

* _CompareGraphs.py_: given a target Autonomous System (or AS) and a list of dates (provided in 
  a text file), compares the graph obtained on the first date with each subsequent graph. The 
  computed metrics are written in the console and plotted in a figure at the end.

* _CompareGraphPair.py_: given a target Autonomous System (or AS) and two dates in dd/mm/yyyy 
  format, compares the two graphs produced on both dates to obtain the ratio of redundant edges 
  and the ratio of common vertices. In addition, this script also writes a detailed description 
  of the redundant edges in the console.

* _IsomorphismFigures.sh_: if provided with a list of Autonomous Systems (ASes) in text format 
  as well as a list of dates (again in text format), this Shell script will run the Python script 
  _CompareGraphs.py_ for each individual AS for the given dates, then move the produced figures 
  to a sub-folder named after the first and last date of the list of dates.

Here is the typical command you could use to generate figures for the ASes listed in 
_Example\_ASes_ for the dates given in _Example\_dates_:

```sh
./IsomorphismFigures.sh Example_ASes Example_dates
```

## Important remarks

* Do not forget to adapt the `datasetPrefix` variable in both Python scripts to match the location 
  in your file system where you placed the data collected by `SAGE`.

* _IsomorphismFigures.sh_ will be relevant if and only if the graphs available for the listed ASes 
  were produced on the same dates, just like for the two PlanetLab campaigns of `SAGE` which you 
  will find in the *Dataset/* sub-folder located at the root of this repository. Consider using 
  _CompareGraphs.py_ directly (thus, one AS at a time with differing dates) if the graphs which 
  you want to compare for a collection of ASes were not produced on the same dates.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
