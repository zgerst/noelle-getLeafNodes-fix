# <u>N</u>OELLE <u>O</u>ffers <u>E</u>mpowering <u>LL</u>VM <u>E</u>xtensions


## Table of Contents
- [Description](#description)
- [Version](#version)
- [Prerequisites](#prerequisites)
- [Build NOELLE](#build-noelle)
- [Testing](#testing)
- [Structure](#structure)
- [License](#license)


## Description
NOELLE provides abstractions to help build advanced code analyses and transformations.

NOELLE is in active development so more tools, tests, and abstractions will be added.

We release NOELLE's source code in the hope of benefiting others. 
You are kindly asked to acknowledge usage of the tool by citing the following paper:
```
@misc{matni2021noelle,
      title={{NOELLE} {O}ffers {E}mpowering {LL}VM {E}xtensions},
      author={Angelo Matni and Enrico Armenio Deiana and Yian Su and Lukas Gross and Souradip Ghosh and Sotiris Apostolakis and Ziyang Xu and Zujun Tan and Ishita Chaturvedi and David I. August and Simone Campanoni},
      year={2021},
      eprint={2102.05081},
      archivePrefix={arXiv},
      primaryClass={cs.PL}
}
```

The only documentation available for NOELLE is:
- the paper: https://arxiv.org/abs/2102.05081
- the comments within the code
- the slides we use in the "Advanced Topics in Compilers" class: www.cs.northwestern.edu/~simonec/ATC.html
  (Projects students do in this class are built upon NOELLE)


## Version
The latest stable version is 9.2.1 (tag = `v9.2.1`).

### Version Numbering Scheme
The version number is in the form of \[v _Major.Minor.Revision_ \]
- **Major**: Each major version matches a specific LLVM version (e.g., version 9 matches LLVM 9, version 11 matches LLVM 11)
- **Minor**: Starts from 0, each minor version represents either one or more API replacements/removals that might impact the users OR a forced update every six months (the minimum minor update frequency)
- **Revision**: Starts from 0; each revision version may include bug fixes or incremental improvements

#### Update Frequency
- **Major**: Matches the LLVM releases on a best-effort basis
- **Minor**: At least once per six months, at most once per month (1/month ~ 2/year)
- **Revision**: At least once per month, at most twice per week (2/week ~ 1/month)


## Prerequisites
LLVM 9.0.0

### Northwestern
Next is the information for those that have access to the Zythos cluster at Northwestern.

To enable the correct LLVM, run the following command from any node of the Zythos cluster:
```
source /project/extra/llvm/9.0.0/enable
```

The guide about the Zythos cluster can be downloaded here: 
www.cs.northwestern.edu/~simonec/files/Research/manuals/Zythos_guide.pdf

## Build NOELLE
To build and install: run `make` from the repository root directory.

Run `make clean` from the root directory to clean the repository.

Run `make uninstall` from the root directory to uninstall the NOELLE installation.


## Testing
To run all tests in parallel using Condor, invoke the following commands:
```
make clean ; 
cd tests ;
make condor ;
```
To monitor how tests are doing: `cd tests ; make condor_watch`

To find out if all tests passed: `cd tests ; make condor_check`

To test NOELLE using condor to run all tests in parallel, go to "tests" and run "make condor".


## Structure
The directory `src` includes sources of the noelle framework.
Within this directory, `src/core` includes the abstractions provided by NOELLE.
Also, `src/tools` includes code transformations that rely on the NOELLE's abstractions to modify the code.

The directory `external` includes libraries that are external to noelle that are used by noelle.
Some of these libraries are patched and/or extended for noelle.

The directory `tests` includes unit tests, integration tests, and performance tests.
Furthermore, this directory includes the scripts to run all these tests in parallel via condor.

The directory `examples` includes examples of LLVM passes (and their tests) that rely on the noelle framework.

Finally, the directory `doc` includes the documentation of noelle.


### Examples of using NOELLE
LLVM passes in the directory `examples/passes` shows use cases of NOELLE.

If you have any trouble using this framework feel free to reach out to us for help (contact simone.campanoni@northwestern.edu).


### Contributions
We welcome contributions from the community to improve this framework and evolve it to cater for more users.


## License
NOELLE is licensed under the [MIT License](./LICENSE.md).
