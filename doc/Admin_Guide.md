LDP Administrator Guide
=======================

##### Contents  
1\. Overview  
2\. System requirements  
3\. Installation  
4\. Database configuration  
5\. Server configuration  
6\. Direct extraction


1\. Overview
------------

There are two components in LDP deployment: an LDP server and a
database server.  Together these constitute an _LDP instance_.

The LDP server updates data in the database from data sources such as
FOLIO modules, and users connect directly to the database to perform
reporting and analytics.

LDP is not multitenant in the general sense, and normally one LDP
instance is deployed per library.  However, shared data from multiple
libraries of a consortium can be stored in a single LDP instance, and
in that case we refer to each of the libraries as a tenant.  _This
consortial feature is not fully implemented but is planned for the
near future._

This administrator guide covers installation and configuration of a
single LDP instance.


2\. System requirements
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
  * [SQLite](https://sqlite.org/) 3.22.0 or later
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
higher performance and scalability.  The database storage capacity
also can be increased as needed.


3\. Installation
----------------

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
      postgresql-server-devel sqlite-devel unixODBC-devel
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
$ cd build/src/
$ ./ldp
```


4\. Database configuration
--------------------------

### Creating a database

Before using the LDP software, we have to create a database that will
store the data.  This can be a local or cloud-based PostgreSQL
database or a cloud-based Redshift database.

#### PostgreSQL hosted in RDS

For libraries that deploy LDP with cloud-based PostgreSQL using Amazon
Relational Database Service (RDS), these configuration settings are
suggested as a starting point:

* Instance type:  `db.m5.large`
* Number of instances:  `1`
* Storage:  `General Purpose SSD`
* Snapshots:  Automated snapshots enabled

#### Redshift

For libraries that deploy LDP with Redshift, these configuration
settings are suggested as a starting point:

* Node type:  `dc2.large`
* Cluster type:  `Single Node`
* Number of compute nodes:  `1`
* Snapshots:  Automated snapshots enabled
* Maintenance Track:  `Trailing`

### Configuring the database

Three database users are required:

* `ldpadmin` is the LDP administrator which owns all database objects
  created by the LDP software.  This account should be used sparingly
and carefully.

* `ldpconfig` is a special user account for changing configuration
  settings in the `ldpconfig` schema.  It is intended to enable
designated users to make changes safely to the server operation such
as scheduling when data updates occur.  This user name can be modified
using the `ldpconfigUser` configuration setting in `ldpconf.json`.

* `ldp` is a general user of the LDP database.  This user name can be
  modified using the `ldpUser` configuration setting in
`ldpconf.json`.

In addition to creating these users, it is a good idea to restrict
access permissions.  In PostgreSQL, this can be done on the command
line, for example:

```shell
$ createuser ldpadmin --username=<admin_user> --pwprompt
$ createuser ldpconfig --username=<admin_user> --pwprompt
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
CREATE USER ldpconfig PASSWORD '(ldpconfig password here)';
CREATE USER ldp PASSWORD '(ldp password here)';
ALTER DATABASE ldp OWNER TO ldpadmin;
REVOKE ALL ON SCHEMA public FROM public;
GRANT ALL ON SCHEMA public TO ldpadmin;
GRANT USAGE ON SCHEMA public TO ldp;
```

Assuming this preliminary set up has been completed, the LDP software
will automatically initialize the schema in an empty database, or
upgrade the schema in a database previously initialized with an
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


5\. Server configuration
------------------------

### Creating a data directory

LDP uses a "data directory" where cached and temporary data, as
well as server configuration files, are stored.  In these examples, we
will suppose that the data directory is `/var/lib/ldp` and that the
server will be run as an `ldp` user:

```shell
$ sudo mkdir -p /var/lib/ldp
$ sudo chown ldp /var/lib/ldp
```

### Configuration file

Server configuration settings are stored in a file in the data
directory called `ldpconf.json`.  In our example it would be
`/var/lib/ldp/ldpconf.json`.  The provided example file
[ldpconf.json](https://raw.githubusercontent.com/folio-org/ldp/master/examples/ldpconf.json)
can be used as a template.

__ldpconf.json__
```
{
    "ldpDatabase": {
        "odbcDatabase": "ldpdemo"
    },
    "enableSources": ["okapi"],
    "sources": {
        "okapi": {
            "okapiURL": "https://folio-release-okapi.aws.indexdata.com",
            "okapiTenant": "diku",
            "okapiUser": "diku_admin",
            "okapiPassword": "(Okapi password here)"
        }
    }
}
```

The following configuration settings are supported:

* `ldpDatabase` (required) is a group of database-related settings.

  * `odbcDatabase` (required) is the ODBC "data source name" of the
    LDP database.

  * `ldpconfigUser` (optional) is the database user that is defined by
    default as `ldpconfig`.

  * `ldpUser` (optional) is the database user that is defined by
    default as `ldp`.

* `enableSources` (required) is an array of sources that are enabled
  for the LDP to extract data from.  The source names refer to a
subset of those defined under `sources` (see below).  Only one source
should be provided in the case of non-consortial deployments.

* `sources` (required) is a collection of sources that LDP can extract
  data from.  Only one source should be provided in the case of
non-consortial deployments.  A source is defined by a source name and
an associated object containing several settings:

  * `okapiURL` (required) is the URL for the Okapi instance to extract
    data from.

  * `okapiTenant` (required) is the Okapi tenant.

  * `okapiUser` (required) is the Okapi user name.

  * `okapiPassword` (required) is the password for the specified Okapi
    user name.

### Starting the server

To start the LDP server:

```shell
$ nohup ldp server -D /var/lib/ldp &>> logfile &
```

The server logs details of its activities in the table
`ldpsystem.log`.  For more detailed logging, the `--trace` option can
be used:

```shell
$ nohup ldp server -D /var/lib/ldp --trace &>> logfile &
```

Once per day, the server runs a _full update_ which performs all
supported data updates.  Full updates can be scheduled at a preferred
time of day using the table `ldpconfig.general`.  See the
[Configuration Guide](Config_Guide.md) for more information.


6\. Direct extraction
---------------------

LDP extracts most data via module APIs; but in some cases it is
necessary to extract directly from a module's internal database, such
as when the data are too large for the API to process.  In LDP this is
referred to as _direct extraction_ and is currently supported for the
following tables:

* `inventory_holdings`
* `inventory_instances`
* `inventory_items`

Direct extraction can be enabled by adding the list of tables and
database connection parameters to a source configuration, as in this
example:

```
{

    ( . . . )

    "sources": {
        "okapi": {

            ( . . . )

	    "directTables": [
	        "inventory_holdings",
		"inventory_instances",
		"inventory_items"
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

Note that direct extraction requires network access to the database,
which may be protected by a firewall.


<!--
7\. Updating data from files (testing only)
-------------------------------------------

For testing purposes, source data can be loaded directly from the file
system using the `update` command with options `--unsafe` and
`--sourcedir`, e.g.:

```shell
$ ldp update -D /usr/local/ldp/data --unsafe --sourcedir ldp-analytics/testdata/
```

The `update` command cannot be used while the server is running, and
it will wait until the server is stopped before continuing.

The source data are expected to have particular names, e.g.
`loans_0.json`.  In addition, an optional, accompanying file can be
included having the suffix, `_test.json`, e.g. `loans_test.json`.
Data in these "test" files are loaded into the same table as the files
they accompany.  This is used for testing in query development to
combine extracted test data with additional static test data.
-->


<!--
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
-->


Further reading
---------------

[__Learn about configuring LDP in the Configuration Guide > > >__](Config_Guide.md)

[__Learn about using the LDP database in the User Guide > > >__](User_Guide.md)

