# Evaluation of graph isomorphism

*By Jean-François Grailet (last updated: March 31, 2021)*

## About

**N.B.:** if you reached this part of the repository via a direct link, please note that this 
sub-folder doesn't provide the data collected by `SAGE` nor its source code. You will rather find 
them respectively in the **Dataset/** and **src/** sub-folders at the root of this repository.

You can use the content of this folder in order to evaluate the isomorphism of graphs, i.e., 
evaluate how similar two graphs **of a same target network** are. The purpose of the provided 
scripts is to check to what extent graphs of a same target network collected from different 
vantage points (and on different dates) are similar.

## Quantifiying similarities between two measurements

To compare graphs, one can first compare the vertices, i.e., the neighborhoods discovered by 
`SAGE` in each snapshot (i.e., the data collected by `SAGE` for a single target network from a 
single vantage point). In the data, neighborhoods are typically identified by one or several 
router interfaces, with several router interfaces meaning the neighborhood was built with the 
help of alias resolution (e.g., if it's a convergence point in load-balanced paths). It is 
therefore possible to check if a neighborhood is present in two snapshots by checking if there 
exists one vertice in each snapshot identified by the same router interface(s).

It's worth reminding that in some cases, `SAGE` cannot build a neighborhood on the basis of 
the last hops towards a set of subnets due to these last hops containing problematic hops such as 
anonymous interfaces. For such cases, `SAGE` relies on _best effort_ strategies to build 
neighborhoods, which are consequently named _best effort_ neighborhoods. It is still possible to 
compare labels of _best effort_ neighborhoods, though there are less likely to always appear in 
the same way depending on the vantage point. This is why it is recommended to quantify the 
ratio of common vertices in two snapshots (i.e., the total of vertices appearing in both 
snapshots w.r.t. to the total of vertices in the first snapshot) with and without _best effort_ 
neighborhoods.

After quantifying the common vertices, it becomes possible to check whether the graphs in two 
snapshots provide comparable edges. I.e., if two snapshots feature neighborhoods/vertices 
identified by the router interfaces `u` and `v`, if the first snapshot contains an edge 
`u -> v`, then this edge should appear as well in the second snapshot.

## Metrics

The scripts and figures of this sub-repository use the following metrics.

* **Redudant non-best effort neighborhoods ratio:** given two graphs (snapshots), this metric 
  expresses how many neighborhoods (vertices), excluding _best effort_ neighborhoods, appear in 
  both graphs, i.e., they are identified by the same router interfaces. This ratio is computed as 
  the total of redundant vertices divided by the total of vertices found in the first graph.

* **Redundant neighborhoods ratio:** this is the same metric as above, but including _best effort_ 
  neighborhoods. This ratio is usually lower than the previous one due to _best effort_ 
  neighborhoods being less likely to appear identically depending on the vantage points from 
  where `SAGE` was run.

* **Redundant edges ratio:** given two graphs (snapshots), this metric expresses the amount of 
  edges that exist in both graphs with respect to the amount of edges that _can_ exist in both 
  graphs, i.e., the vertices connected by an edge of the first graph also exist in the second 
  graph. E.g., for an edge `u -> v` found in the first graph, `u` and `v` must also exist in 
  the second graph to test whether the selected edge is redundant.

* **Intersection of vertices:** given a set of graphs (snapshots) collected for a same target 
  network but from different vantage points (and different dates), this metric expresses the 
  ratio of vertices from the first graph that always reappear in the subsequent graphs. It is 
  meant to evaluate how many neighborhoods from the first measurement always reappear in 
  subsequent measurements despite the change of vantage point.

## Scripts

There are three scripts to review these metrics.

* `CompareGraphs.py`: given a target Autonomous System (or AS) and a list of dates (provided in 
  a text file), compares the graph obtained on the first date with each graph computed on each 
  subsequent date. The computed metrics are both written in the console and plotted in a PDF 
  figure.

* `CompareGraphPair.py`: given a target Autonomous System (or AS) and two dates in dd/mm/yyyy 
  format, compares the two graphs produced on both dates by computing the previously described 
  metrics. In addition, this script also writes a detailed description of the redundant edges in 
  the console.

* `IsomorphismFigures.sh`: if provided with a list of Autonomous Systems (ASes) in text format 
  as well as a list of dates (again in text format), this Shell script will run the Python script 
  `CompareGraphs.py` for each individual AS for the given dates, then move the produced figures 
  to a sub-folder named after the first and last date of the list of dates.

Here is the typical command you could use to generate figures for the ASes listed in 
`Example_ASes` for the dates given in `Example_dates`:

```sh
./IsomorphismFigures.sh Example_ASes Example_dates
```

## Important remarks

* Do not forget to adapt the `datasetPrefix` variable in both Python scripts to match the location 
  in your file system where you placed the data collected by `SAGE`.

* `IsomorphismFigures.sh` will be relevant if and only if the graphs available for the listed ASes 
  were produced on the same dates, just like with the two PlanetLab campaigns of `SAGE` which you 
  will find in the **Dataset/** sub-folder located at the root of this repository. Consider using 
  `CompareGraphs.py` directly (thus, one AS at a time with differing dates) if the graphs which 
  you want to compare for a collection of ASes were not produced on the same dates.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
