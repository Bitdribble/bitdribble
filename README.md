The bitd-agent
==============
The bitd-agent is a dev/ops integration tool with the ability to run tasks on a schedule, and to create flows of triggered tasks where the output of a task instance can be used as input to one or more other task instances. The output of task instances can be sent to a Graphite or InfluxDB database back end, where it is visualized with Grafana dashboards.

Configuration for the bitd-agent can be set in ``yaml`` or ``xml`` format. The configuration file defines which task instances will be running, on what schedule, and which flows of triggered task instances are created. 

``Tasks`` are implemented in bitd-agent ``modules``. A module can contain one or more ``tasks``. Example modules are: 
- The ``bitd-exec`` module, containing the ``exec`` task, which is able to execute any child process (passing its task instance input as standard input to the child, and processing the child standard output and standard error as task instance output, respectively error).
- The ``bitd-assert`` module, containing the ``assert`` task, which is used to assert that a specific condition should happen.
- The ``bitd-echo`` module, containing the ``echo`` task, which simply echoes its input as output.
- The ``bitd-sink-graphite`` module, containing the ``sink-graphite`` task, which sends output to a Graphite database
- The ``bitd-sink-influxdb`` module, containing the ``sink-influxdb`` task, which sends output to an InfluxDB database.

Project documentation is available at http://bitdribble.com/doc.
