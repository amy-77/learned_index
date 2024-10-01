# learned_index
## Ⅰ. Benchmark Datasets (7 datasets)
We adopt 4 real datasets from SOSD [1]

Specifically:

* **[fb](https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/EATHF7):** A set of user IDs randomly sampled from Facebook [7].
* **[wiki](https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/SVN8PI):** A set of edit timestamp IDs committed to Wikipedia [8].
* **[books](https://www.dropbox.com/s/y2u3nbanbnbmg7n/books_800M_uint64.zst?dl=1):** A dataset of book popularity from Amazon.
* **[osm_cellids](https://www.dropbox.com/s/j1d4ufn4fyb4po2/osm_cellids_800M_uint64.zst?dl=1):** A set of cell IDs from OpenStreetMap [9].

We also generate 3 synthetic datasets by sampling from uniform, normal, and log-normal distributions, following a process similar to [1, 10]. 

All keys are stored as 64-bit unsigned integers (`uint64_t` in C++). Table 1 summarizes the dataset statistics.

**Table 1: Statistics of benchmark datasets.**

| Dataset | Category | Keys | Raw Size | $h_D$ | $\overline{Cov}$ |
|---|---|---|---|---|---|
| fb | Real | 200 M | 1.6 GB | 3.88 | 94 |
| wiki | Real | 200 M | 1.6 GB | 1.77 | 877 |
| books | Real | 800 M | 6.4 GB | 5.39 | 101 |
| osm | Real | 800 M | 6.4 GB | 1.91 | 129 |
| uniform | Synthetic | 400 M | 3.2 GB | Varied | N.A. |
| normal | Synthetic | 400 M | 3.2 GB | Varied | N.A. |
| lognormal | Synthetic | 400 M | 3.2 GB | Varied | N.A. |


**References:**  
[1] Marcus, et al. SOSD: A Benchmark Suite for Similarity Search over Sorted Data. PVLDB, 2020.  
[7] Sandt, et al.  Facebook's Data Infrastructure at Scale. SIGMOD, 2019.  
[8] Wikidata. [https://www.wikidata.org/wiki/Wikidata:Main_Page](https://www.wikidata.org/wiki/Wikidata:Main_Page)  
[9] OpenStreetMap. [https://www.openstreetmap.org/](https://www.openstreetmap.org/)  
[10] Zhang, et al. Making Learned Indexes Practical: A Comprehensive Study on Data Distribution and Model Selection. PVLDB, 2024.  


## II. RUN RMI to generate results:


### 1, RUN RMI to construct models: 
RMI code: https://github.com/learnedsystems/RMI/tree/master  
This is a reference implementation of recursive model indexes (RMIs). A prototype RMI was initially described in [The Case for Learned Index Structures (https://arxiv.org/abs/1712.01208) by Kraska et al. in 2017.  



```C++
cargo run --release -- --optimize (optimizer_out_wiki.json)JSON_PATH wiki_200M_uint64
cargo run wiki_200M_uint64 --param-grid (optimizer_out_wiki.json)JSON_PATH -d YOUR_RMI_SAVE_FOLDER --threads 8 --zero-build-time
```
And then the RMI will generate 9 models in **YOUR_RMI_SAVE_FOLDER**. 
For fb dabatset: it includs 9 RMI output parameter files(fb_200M_uint64_i_L1_PARAMETERS, i=0,...,9), 27 RMI RMI codes files (each model generate 3 code files (named fb_200M_uint64_i_data.h, fb_200M_uint64_i.cpp, fb_200M_uint64_i.h, i=0,...,9). 

### 2，**Query Workloads.**

Our work focuses on *in-memory read-heavy* workloads. Given a key set  K, we generate the query workload by randomly sampling S (by default S=5,000) keys from K. To simulate different access patterns, we sample lookup keys from two distributions:

* **Uniform:**  Every key in K has an equal likelihood of being sampled.
* **Zipfan:** The probability of sampling the i-th key in K is given by p(i)=i<sup>α</sup>/∑<sub>j=1</sub><sup>N</sup>j<sup>α</sup>. For the Zipfan workload, by default, we set the parameter α=1.3 such that over 90% of index accesses occur within the range of (0, 10<sup>3</sup>].

The detailed sampling method for these two query workloads is in the main_**dataset** folder. dataset can be fb, wiki, books, osm, uniform, normal, or lognormal.

To run the benchmarks:
```C++
make -f Makefile_all run_all
```






