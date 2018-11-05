# The bitd-agent [![Build Status](https://travis-ci.com/Bitdribble/bitdribble.svg?branch=master)](https://travis-ci.com/Bitdribble/bitdribble)

The bitd-agent is a dev/ops integration tool with the ability to schedule and run tasks, and to create flows of triggered tasks where the output of a task instance can be used as input to one or more other task instances. The output of task instances can be sent to a Graphite or InfluxDB database back end, where it is visualized with Grafana dashboards.

Configuration for the bitd-agent can be set in ``yaml`` or ``xml`` format. The configuration file defines which task instances will be running, on what schedule, and which flows of triggered task instances are created. 

``Tasks`` are implemented in bitd-agent ``modules``. A module can contain one or more ``tasks``. Example modules are: 
- The ``bitd-exec`` module, containing the ``exec`` task, which is able to execute any child process (passing its task instance input as standard input to the child, and processing the child standard output and standard error as task instance output, respectively error).
- The ``bitd-assert`` module, containing the ``assert`` task, which is used to assert that a specific condition should happen.
- The ``bitd-echo`` module, containing the ``echo`` task, which simply echoes its input as output.
- The ``bitd-sink-graphite`` module, containing the ``sink-graphite`` task, which sends output to a Graphite database
- The ``bitd-sink-influxdb`` module, containing the ``sink-influxdb`` task, which sends output to an InfluxDB database.


# Documentation
Project documentation is available at http://bitdribble.com/doc and at https://bitdribble.readthedocs.io. 


# Supported platforms
The ``bitd-agent`` runs on Linux, Mac OSX, native Windows and Cygwin Windows. 

# Installation
For detailed compilation instructions on non-Linux platforms, see the project documentation. To compile on Linux, ensure that the ``expat``, ``libyaml``, ``openssl`` and ``libcurl`` development libraries installed. Check out the Git sandbox, and execute ``cmake; make`` then, as root, ``make install``. Version 3 ``cmake`` is required. The ``cmake`` step is best executed out of folder. For example on ``Centos 7``:

```
sudo yum install cmake3 expat-devel libyaml-devel openssl-devel libcurl-devel

cd .../bitdribble
mkdir build && cd build && cmake3 ..
make
cpack3 -G RPM
sudo rpm -ivh bitd-<version>-<platform>.rpm
```

And on ``Ubuntu 18.04``:

```
sudo apt-get install libexpat-dev libyaml-dev libssl-dev libcurl4-openssl-dev

cd .../bitdribble
mkdir build && cd build && cmake ..
make

cpack -G DEB
sudo rpm -ivh bitd-<version>-<platform>.rpm
```

And after setting test instance configuration in ``/etc/bitd-agent.yml`` according to details listed in the project documentation:
```
sudo systemctl enable bitd
sudo systemctl start bitd
```

# License
The code is licensed under the Apache version 2 license.
