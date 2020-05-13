LDP Admin Guide
===============

##### Contents  
1\. System requirements  
2\. Installing the software  
3\. Preparing the LDP database  
4\. Configuring the LDP software  
5\. Loading data into the database  
6\. Running the LDP in production  
7\. Loading data from files (for testing only)  
8\. Direct extraction of large data  
9\. Server mode  
10\. Referential analysis (experimental)


1\. System requirements
-----------------------

### Software

* Operating systems supported:
  * Linux 4.18.0 or later
  * macOS 10.15.3 or later (non-production use only)
* Database systems supported:
  * [PostgreSQL](https://www.postgresql.org/) 11 or later
  * [Amazon Redshift](https://aws.amazon.com/redshift/) 1.0.12094 or later
* Other software dependencies:
  * ODBC driver for [PostgreSQL](https://odbc.postgresql.org/) or [Redshift](https://docs.aws.amazon.com/redshift/latest/mgmt/configure-odbc-connection.html#install-odbc-driver-linux)
  * [unixODBC](http://www.unixodbc.org/) 2.3.4 or later
  * [libpq](https://www.postgresql.org/) 11.5 or later
  * [libcurl](https://curl.haxx.se/) 7.64.0 or later
  * [RapidJSON](https://rapidjson.org/) 1.1.0 or later
* Required to build from source code:
  * C++ compilers supported:
    * [GCC C++ compiler](https://gcc.gnu.org/) 8.3.0 or later
    * [Clang](https://clang.llvm.org/) 8.0.1 or later
  * [CMake](https://cmake.org/) 3.16.2 or later

### Hardware

The LDP software and database are designed to be performant on low
cost hardware, and in most cases they should run well with the
following minimum requirements:

* Database:
  * Memory: 1 GB
  * Storage: 160 GB HDD
* LDP software:
  * Memory: 100 MB
  * Storage: 160 GB HDD

For large libraries, or if very high performance is desired, the
database CPU and memory can be increased as needed.  Alternatively,
Amazon Redshift can be used instead of PostgreSQL for significantly
higher performance and scalability.  (See below for notes on
configuring Redshift.)  The database storage capacity also can be
increased as needed.


2\. Installing the software
---------------------------

### Releases and branches

Note: All releases earlier than LDP 1.0 are for testing purposes and
are not intended for production use.  Releases earlier than LDP 1.0
also do not support database migration, so that it may be necessary to
create a new database when upgrading to a new release.

LDP releases use version numbers in the form, _a_._b_._c_, where
_a_._b_ is the release number and _c_ indicates a bug fix version.
For example, suppose that LDP 1.3 has been released.  The first
version of the 1.3 release will be 1.3.0.  Any subsequent versions
with the same release number, for example, 1.3.1 or 1.3.2, will
generally contain no new features but only bug fixes.  (This practice
will be less strictly observed in pre-releases, i.e. prior to 1.0.)

Stable versions of LDP are available from the [releases
page](https://github.com/folio-org/ldp/releases) and via the
`-release` branches described below.

Within the [source code repository](https://github.com/folio-org/ldp)
there are two main branches:

* `master` is the branch that new releases are made from.  It contains
  recently added features that have had some testing.

* `current` is for active development and tends to be very unstable.
  This is where new features are first added.

Beginning with LDP 1.0, a numbered branch will be created for each
release; for example, `1.2-release` would point to the latest version
of LDP 1.2, e.g. `1.2.5`.

The `-release` branches are the most stable versions, and `current` is
the least stable; `master` is somewhere in between in terms of
stability.

### Before installation

Dependencies required for building the LDP software can be installed via
a package manager on some platforms.

#### Debian Linux

```shell
$ sudo apt install cmake g++ libcurl4-openssl-dev libpq-dev \
      postgresql-server-dev-all rapidjson-dev unixodbc unixodbc-dev \
      libsqlite3-dev
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ sudo apt install odbc-postgresql
```

#### RHEL/CentOS Linux

```shell
$ sudo dnf install cmake gcc-c++ libcurl-devel libpq-devel make \
      postgresql-server-devel unixODBC-devel
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ sudo dnf install postgresql-odbc
```

RapidJSON can be [installed from
source](https://rapidjson.org/index.html#autotoc_md5).

#### macOS

Using [Homebrew](https://brew.sh/):

```shell
$ brew install cmake postgresql psqlodbc rapidjson unixodbc
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ brew install psqlodbc
```

### Building the software

To build the LDP software, download and unpack [a recent
release](https://github.com/folio-org/ldp/releases) and `cd` into the
unpacked directory.  Then:

```shell
$ ./all.sh
```

The compiled executable file `ldp` should appear in `ldp/build/src/`:

```shell
$ ./build/src/ldp
```


3\. Preparing the LDP database
------------------------------

### Configuring the database

Before using the LDP software, we need a database to load data into.
Two database users are also required: `ldpadmin`, an administrator
account, and `ldp`, a normal user of the database.  It is also a good
idea to restrict access permissions.

In PostgreSQL, this can be done on the command line, for example:

```shell
$ createuser ldpadmin --username=<admin_user> --pwprompt
$ createuser ldp --username=<admin_user> --pwprompt
$ createdb ldp --username=<admin_user> --owner=ldpadmin
$ psql ldp --username=<admin_user> --single-transaction \
      --command="REVOKE ALL ON SCHEMA public FROM public;" \
      --command="GRANT ALL ON SCHEMA public TO ldpadmin;" \
      --command="GRANT USAGE ON SCHEMA public TO ldp;"
```

Additional command line options may be required to specify the database
host, port, etc.

In Redshift, this can be done in the database once it has been created,
for example:

```sql
CREATE USER ldpadmin PASSWORD '(ldpadmin password here)';
CREATE USER ldp PASSWORD '(ldp password here)';
ALTER DATABASE ldp OWNER TO ldpadmin;
REVOKE ALL ON SCHEMA public FROM public;
GRANT ALL ON SCHEMA public TO ldpadmin;
GRANT USAGE ON SCHEMA public TO ldp;
```

Assuming this preliminary set up has been done, the LDP software will
automatically attempt to initialize the schema in an empty database,
or to upgrade the schema in a database previously initialized with an
earlier version of LDP.

### Configuring ODBC

The LDP software uses unixODBC to connect to the LDP database.  To
configure ODBC, install the ODBC driver for the database system being
used (PostgreSQL or Redshift), and create the files
`$HOME/.odbcinst.ini` and `$HOME/.odbc.ini` as described in this
[guide](http://www.unixodbc.org/odbcinst.html).

The provided example files
[odbcinst.ini](https://raw.githubusercontent.com/folio-org/ldp/master/examples/odbcinst.ini)
and
[odbc.ini](https://raw.githubusercontent.com/folio-org/ldp/master/examples/odbc.ini)
can be used as templates.

__odbcinst.ini__
```
[PostgreSQL]
Description = PostgreSQL
Driver = /usr/lib/x86_64-linux-gnu/odbc/psqlodbcw.so
FileUsage = 1
```

__odbc.ini__
```
[ldpdemo]
Description = ldp
Driver = PostgreSQL
Database = ldp
Servername = ldp.indexdata.com
UserName = ldpadmin
Password = (ldpadmin password here)
Port = 5432
SSLMode = require
```


4\. Configuring the LDP software
--------------------------------

The LDP software reads a configuration file that describes
administrative settings for an LDP database.  The provided example
file
[ldpconfig.json](https://raw.githubusercontent.com/folio-org/ldp/master/examples/ldpconfig.json)
can be used as a template.

__ldpconfig.json__
```
{
    "ldpDatabase": {
        "odbcDataSourceName": "ldpdemo"
    },
    "dataSources": {
        "okapi": {
            "okapiURL": "https://folio-release-okapi.aws.indexdata.com",
            "okapiTenant": "diku",
            "okapiUser": "diku_admin",
            "okapiPassword": "(Okapi password here)",
            "extractDir": "/var/lib/ldp/extract/"
        }
    }
}
```

The path to this file is specified using the command line option
`--config`, e.g.:

```shell
$ ldp update --config /etc/ldp/ldpconfig.json  ( etc. )
```


5\. Loading data into the database
----------------------------------

The LDP loader is intended to be run once per day, at a time of day when
usage is low, in order to refresh the database with new data.

To extract data and load them into the LDP database:

```shell
$ ldp update --config ldpconf.json --source okapi
```

The `update` command is used to load data.  The data are extracted
from a data "source" and loaded into the LDP database.

The `--source` option specifies the name of a section under `sources`
in the LDP configuration file.  This section should provide connection
details for a data source, as well as a directory (`extractDir`) where
temporary extracted files can be written.

LDP logs its activities to the table `ldpsystem.log`.  The `--trace`
option can be used to enable more detailed logging.

Another option that is available to assist with debugging problems is
`--savetemps` (used together with `--unsafe`), which tells the loader
not to delete temporary files that store extracted data.  These files
are not anonymized and may contain personal data.


6\. Running the LDP in production
---------------------------------

Note:  LDP releases earlier than 1.0 should not be used in production.
This section is included here to assist system administrators in
planning for production deployment.

The data loader is intended to be run automatically once per day, using
a job scheduler such as Cron.

### Scheduling data loads and availability of the database

While the data loader runs, the database generally will be available to
users, although query performance may be affected.  However, note that
some of the final stages of the loading process involve schema changes
and could interrupt any long-running queries that are executing at the
same time.  For these reasons, it may be best to run the data loader at
a time when the database will not be used heavily.

### Handling errors in data loading

If the data loading fails, the `ldp` process returns a non-zero exit
status.  It is recommended to check the exit status if the process is
being run automatically, in order to alert the administrator that data
loading was not completed.  Once the problem has been addressed,
simply run the data loader again.

### Managing temporary disk space for data loading

As mentioned earlier, the "sources" listed in the configuration file
include `extractDir` which is a directory used for writing temporary
files extracted from a source.  This directory should have enough disk
space to hold all of the extracted data in JSON format.

The data loader expects that the `extractDir` directory already exists,
and it creates a temporary directory under it beginning with the prefix,
`tmp_ldp_`.  After data loading has been completed, the `ldp` process
will normally try to delete the temporary directory as part of "cleaning
up".  However, it is possible that the clean-up phase may not always
run, for example, if a signal 9 is sent to the process.  This could
result in filling up disk space and cause future data loading to fail.
In a production context it is a good idea to take additional steps to
ensure that any temporary directories under the `extractDir` directory
are removed after each run of the data loader.

### RDS/PostgreSQL configuration

For libraries that deploy the LDP database in PostgreSQL using Amazon
Relational Database Service (RDS), these configuration settings are
suggested as a starting point:

* Instance type:  `db.m5.large`
* Number of instances:  `1`
* Storage:  `General Purpose SSD`
* Snapshots:  Automated snapshots enabled

### Redshift configuration

For libraries that deploy the LDP database in Redshift, these
configuration settings are suggested as a starting point:

* Node type:  `dc2.large`
* Cluster type:  `Single Node`
* Number of compute nodes:  `1`
* Snapshots:  Automated snapshots enabled
* Maintenance Track:  `Trailing`


7\. Loading data from files (for testing only)
----------------------------------------------

As an alternative to loading data with the `--source` option, source
data can be loaded directly from the file system for testing purposes,
using the `--unsafe` and `--sourcedir` options, e.g.:

```shell
$ ldp update --config ldpconf.json --unsafe --sourcedir ldp-analytics/testdata/
```

The loader expects the data files to have particular names, e.g.
`loans_0.json`; and when `--sourcedir` is used, it also looks for an
optional, accompanying file ending with the suffix, `_test.json`, e.g.
`loans_test.json`.  Data in these "test" files are loaded into the same
table as the files they accompany.  This is used for testing in query
development to combine extracted test data with additional static test
data.


8\. Direct extraction of large data
-----------------------------------

At the present time, FOLIO modules do not generally offer a performant
method of extracting a large number of records.  For this reason, a
workaround referred to as "direct extraction" has been implemented in
the LDP software that allows some data to be extracted directly from a
module's internal database, bypassing the module API.  Direct
extraction is currently supported for holdings, instances, and items.
It can be enabled by adding database connection parameters to a data
source configuration, for example:

```
{

    ( . . . )

    "dataSources": {
        "okapi": {

            ( . . . )

	    "directInterfaces": [
	        "/holdings-storage/holdings",
		"/instance-storage/instances",
		"/item-storage/items"
            ],
            "directDatabaseName": "okapi",
            "directDatabaseHost": "database.indexdata.com",
            "directDatabasePort": "5432",
            "directDatabaseUser": "folio_admin",
            "directDatabasePassword": "(database password here)"
        }
    }
}
```

Note that this requires the client host to be able to connect to the
database, which may be protected by a firewall.


9\. Server mode
---------------

LDP 0.7 provides a new "server mode" which does not require Cron for
scheduling data updates.  To test this mode, start `ldp` as a
background process with `nohup`, and use the `server` command in place
of the `update` command, e.g.:

```shell
$ nohup ldp server --config ldpconf.json --source okapi &
```

Data updates can be scheduled by setting `next_full_update` in table
`ldpconfig.general`.  For example:

```sql
UPDATE ldpconfig.general
    SET next_full_update = '2020-05-07 22:00:00Z';
```

Also ensure that `full_update_enabled` is set to `TRUE`.


10\. Referential analysis (experimental)
----------------------------------------

LDP 0.7.3 provides an experimental feature that analyzes potential
foreign key relationships between tables, and writes messages to the
log (`ldpsystem.log`) about presumed referential violations.  The
analysis runs immediately after a full update.

To enable this feature:

```sql
UPDATE ldpconfig.general
    SET log_referential_analysis = TRUE;
```


Further reading
---------------

[__Learn about using the LDP database in the User Guide > > >__](User_Guide.md)

