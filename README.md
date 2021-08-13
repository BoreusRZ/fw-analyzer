## Table of Contents
* [Description](#description)
* [Flags](#flags)
* [Configuration](#configuration)
* [Building](#building)
* [Dependencies](#dependencies)
* [Known Limititations](#known-limitations)
## Description
The fw-analyzer tools aims to aid in sanitizing large iptables-ruleset configurations
by performing static analysis to identify:
- **dead rules**:
    rules that never match a packet
- **consumers**:
    The consumer analysis can be enabled seperatly.
    It runs after the deadrule analysis for each dead rule identified and tracks where
    packets, that would have matched against the dead rule, matched instead.
    These consumers will be listed sorted by the number of packets matched.
- **dead jumps**:
    Rules that do match packets, but the place they jump to doesn't perform
    any of the following operations ACCEPT, DROP, REJECT, DNAT, SNAT on the packets that matched 
    against the dead jump.
- **subset rules**:
    pairs of rules where the first rule matches only a subset of the possible matches of the
    second rule.
- **mergable rules**:
    pairs of rules which could be combined into one.
### Example
```
*raw
:PREROUTING DROP [0:0]
:other_chain - [0:0]
-A PREROUTING -s 1.2.3.0/32 -j ACCEPT
-A PREROUTING -s 1.2.3.0/32 -j ACCEPT
-A PREROUTING -s 1.2.3.0/24 -j ACCEPT
-A PREROUTING -dports 10 -j other_chain
-A PREROUTING -dports 11 -j other_chain
-A other_chain -dport 11 -j ACCEPT
COMMIT
```
will yield
```
+-----------------+--------+--------------------------------------------------+
|     ANALYSIS    | AMOUNT |                   LINE NUMBERS                   |
+-----------------+--------+--------------------------------------------------+
| dead rules      | 1      | 5                                                |
+-----------------+--------+--------------------------------------------------+
| dead jumps      | 1      | 7                                                |
+-----------------+--------+--------------------------------------------------+
| subset rules    | 2      | 4->5 5->6                                        |
+-----------------+--------+--------------------------------------------------+
| mergeable rules | 3      | 4+5 5+6 7+8                                      |
+-----------------+--------+--------------------------------------------------+
```
## Flags
- [positional argument]
    Speciefies the path to the iptables-ruleset file.
- "--ipset"
    Speciefies the path to the iptables-ipset file.
- "--analyze"
    Specifies which analysis to run or not to run.
    Currently you can only toggle on the consumer analysis.
    "--analyze consumers" 
- "--config"
    Specifies the [configuration file](configuration).
- "--verbose"
    Enables verbose output e.g.
    after each dead rule found it will print
    the line number and the line itself that contains the dead rule.
- "--progress"
    Enables logging of the progress during the deadrule-analysis.
## Configuration
The configuration happens in file that conforms to [yaml format](https://en.wikipedia.org/wiki/YAML).
```yaml
silence:
    unknown_flags:
        # list of flags that will 
        # not be printed in the summary as unknown_flags
    no_jump_target:
        # list of names of chains in which rules without jump target are allowed
    empty_chains:
        # list of names of chains which are allowed to be empty
disable:
    rules:
        # a disabled rule will not be analyzed in the
        # dead_rule, subset_rule and mergeable_rule analysis
        with_jump_target:
            # list of names of chains
            # rules that have this jump target will be disabled
        with_flags:
            # list of flags and an optional argument
            # rules with these flags and arguments will be disabled 
check_chains:
    # list of chain-names that must be present in the ruleset file
```
## Building
### The Analyzer Executable
After cloning the repository execute the following steps:
```bash
mkdir build
cd build
cmake ..
make analyzer
```
### The Source-Code Documentation
This requires you to install doxygen
```bash
sudo pacman -S doxygen
```
After that do the following to build the documentation
```bash
cd docs
doxygen Doxyfile
```
This will create a html directory, which contains an 'index.html' file
which you can open in your browser.
### Dependencies
- [fmtlib](https://github.com/fmtlib/fmt)
- [gtest](https://github.com/google/googletest)

automaticaly downloaded:
- [p-ranav/argparse](https://github.com/p-ranav/argparse)
- [p-ranav/tabulate](https://github.com/p-ranav/tabulate)
- [rapidcheck](https://github.com/emil-e/rapidcheck)
- [rapidyaml](ttps://github.com/biojppm/rapidyaml)
## Known Limitations
The deadrule-analysis is computationaly very expensive and thus currently only supports
the following packet properties
- source ip e.g. "-s 1.2.3.4/32"
- destination ip e.g. "-d 1.2.3.4/32"
- input interfaces (-i)
- output interfaces (-o)
- source ports (-port,-sport,-sports)
- destination ports (-port,-dport,-dports)
- protocols (-p)

All other flags are ignored by default.
If this messes up the semantics of your ruleset,
you can disable rules with certain flags in the [configuration-file](#configuration)
