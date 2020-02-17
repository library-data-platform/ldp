LDP Admin Guide
===============

##### Contents  
1\. System requirements  
2\. Installing the software  
3\. Preparing the LDP database  
4\. Configuring the LDP software  
5\. Loading data into the database  
6\. Running the LDP in production  
7\. Anonymization of personal data  
8\. Loading data from files (for testing only)  
9\. Disabling database TLS/SSL (for testing only)  
10\. "Direct extraction" of large data


1\. System requirements
-----------------------

### Software

* Operating systems supported:
  * Linux 4.18.0 or later
  * FreeBSD 12.1 or later
  * macOS 10.15.2 or later (non-production use only)
* Database systems supported:
  * [PostgreSQL](https://www.postgresql.org/) 11.5 or later
  * [Amazon Redshift](https://aws.amazon.com/redshift/) 1.0.12094 or later
* Required to build from source code:
  * C++ compilers supported:
    * [GCC C++ compiler](https://gcc.gnu.org/) 8.3.0 or later
    * [Clang](https://clang.llvm.org/) 8.0.1 or later
  * [CMake](https://cmake.org/) 3.7.2 or later
  * [libpq](https://www.postgresql.org/) 11.5 or later
  * [libcurl](https://curl.haxx.se/) 7.64.0 or later
  * [RapidJSON](https://rapidjson.org/) 1.1.0 or later

### Hardware

The LDP software and database are designed to be performant on low
cost hardware, and in most cases they should run well with the
following minimum requirements:

* Database
  * Memory: 1 GB
  * Storage: 160 GB HDD
* LDP software (data loader)
  * Memory: 100 MB
  * Storage: 160 GB HDD

For large libraries, or if very high performance is desired, the
database CPU and memory can be increased as needed.  Alternatively,
Amazon Redshift can be used instead of PostgreSQL for significantly
higher performance and scalability.  (See below for notes on configuring
Redshift.)  The database storage capacity also can be increased as
needed.


2\. Installing the software
---------------------------

### Releases and branches

**Note: All releases earlier than LDP 1.0 are for testing purposes and
are not intended for production use.  Releases earlier than LDP 1.0
also do not support database migration, so that it may be necessary to
create a new database when upgrading to a new release.**

LDP releases use version numbers in the form, *a*.*b*.*c*, where
*a*.*b* is the release number and *c* indicates a bug fix version.
For example, suppose that LDP 1.3 has been released.  The first
version of the 1.3 release will be 1.3.0.  Any subsequent versions
with the same release number, for example, 1.3.1 or 1.3.2, will
generally contain no new features but only bug fixes.  (This practice
will be less strictly observed in pre-releases, i.e. prior to 1.0, due
to the need for user feedback.)

Stable versions of LDP are available from the [releases
page](https://github.com/folio-org/ldp/releases) and via `-release`
branches described below.

Within the [source code repository](https://github.com/folio-org/ldp)
there are two main branches:

* `master` is the branch that new releases are made from.  It contains
  recently added features that have had some testing.

* `current` is for active development and tends to be very unstable.
  This is where new features are first added.

Beginning with LDP 1.0, a numbered branch will be created for each
release; for example, `1.2-release`, would point to the latest version
of LDP 1.2, such as `1.2.5`.  Prior to LDP 1.0, the `beta-release`
version points to the latest release.

### Before installation

Dependencies required for building the LDP software can be installed via
a package manager on some platforms.

#### Debian Linux

```shell
$ sudo apt install cmake g++ libcurl4-openssl-dev libpq-dev \
      postgresql-server-dev-all rapidjson-dev
```

#### FreeBSD

```shell
$ sudo pkg install cmake postgresql11-client rapidjson
```

#### macOS

Using [Homebrew](https://brew.sh/):

```shell
$ brew install cmake postgresql rapidjson
```

### Building the software

To build the LDP software, download and unpack [a recent
release](https://github.com/folio-org/ldp/releases) and `cd` into the
unpacked directory.  Then:

```shell
$ mkdir build
$ cd build/
$ cmake ..
$ make
```
<!--
make install
-->

The compiled executable file `ldp` should appear in `ldp/build/src/`:

```shell
$ cd src/
$ ./ldp help
```


3\. Preparing the LDP database
------------------------------

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


4\. Configuring the LDP software
--------------------------------

The LDP software also needs a configuration file that looks something
like:

```
{
    "sources": {
        "folio": {
            "okapiURL": "https://folio-release-okapi.aws.indexdata.com",
            "okapiTenant": "diku",
            "okapiUser": "diku_admin",
            "okapiPassword": "(Okapi password here)",
            "extractDir": "/var/lib/ldp/extract/"
        }
    },
    "targets": {
        "ldpdemo": {
            "databaseName": "ldp",
            "databaseType": "postgresql",
            "databaseHost": "ldp.indexdata.com",
            "databasePort": "5432",
            "ldpAdmin": "ldpadmin",
            "ldpAdminPassword": "(ldpadmin password here)"
        }
    }
}
```

<!--
**Upgrading from version 0.3.8 or earlier:**  The parameters,
`databaseUser` and `databasePassword`, have changed.  Please rename
`databaseUser` to `ldpAdmin`, and rename `databasePassword` to
`ldpAdminPassword`.
-->

The provided example file
[config_example.json](https://raw.githubusercontent.com/folio-org/ldp/master/config_example.json)
can be used as a template.

This file defines parameters for connecting to Okapi and to the LDP
database.  Please see the next section for how the parameters are used.

The LDP software looks for the configuration file in a location
specified by the `LDPCONFIG` environment variable, e.g. using the Bash
shell:

```shell
$ export LDPCONFIG=/etc/ldp/ldpconfig.json
```

The location also can be specified using the command line option
`--config`:

```shell
$ ldp load --config /etc/ldp/ldpconfig.json  ( etc. )
```


5\. Loading data into the database
----------------------------------

The LDP loader is intended to be run once per day, at a time of day when
usage is low, in order to refresh the database with new data from Okapi.

To extract data from Okapi and load them into the LDP database:
```shell
$ ldp load --source folio --target ldpdemo -v
```

The `load` command is used to load data.  The data are extracted from a
data "source" (Okapi) and loaded into a "target" (the LDP database).

The `--source` option specifies the name of a section under `sources` in
the LDP configuration file.  This section should provide connection
details for Okapi, as well as a directory (`extractDir`) where temporary
extracted files can be written.

The `--target` option works in the same way to specify a section under
`targets` in the configuration file that provides connection details for
the LDP database where data will be loaded.

The `-v` option enables verbose output.  For even more verbose output,
the `--debug` option can be used to see commands that are sent to the
database when loading data, among other details.  The `--debug` option
can generate extremely large output, and for this reason it is best used
when loading relatively small data sets.

Another option that is available to assist with debugging problems is
`--savetemps` (used together with `--unsafe`), which tells the loader
not to delete temporary files that store extracted data.  These files
are not anonymized and may contain personal data.


6\. Running the LDP in production
---------------------------------

**Note:  LDP releases earlier than 1.0 should not be used in production.
This section is included here to assist system administrators in
planning for production deployment.**

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
loading was not completed.  In such cases the data loader generally
leaves the database unchanged, i.e. the database continues to reflect
the data from the previous successful data load.  Once the problem has
been addressed, simply run the data loader again.

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

### Redshift configuration

For libraries that deploy the LDP database in Redshift, these
configuration settings are recommended as a starting point:

* Node type:  `dc2.large`
* Cluster type:  `Single Node`
* Number of compute nodes:  `1`
* Snapshots:  Automated snapshots enabled
* Maintenance Track:  `Trailing`


7\. Anonymization of personal data
----------------------------------

By default, the LDP data loader tries to "anonymize" personal data
that it extracts from Okapi.  At present this anonymization consists
of removing data extracted from interface `/users` except for the
following root-level fields: `id`, `active`, `type`, `patronGroup`,
`enrollmentDate`, `expirationDate`, `meta`, `proxyFor`, `createdDate`,
`updatedDate`, `metadata`, and `tags`.

<!--
It is planned that in a future LDP release, the user ID field, `id`,
will also be anonymized.
-->

<!--
There is no need to do anything to enable these anonymizations; they are
enabled unless the LDP loader is otherwise configured.

If it should be necessary to disable anonymization, this can be done by
setting the `disable_anonymization` parameter to `true` in the LDP
configuration file, within the section that defines connection details
for Okapi.

**WARNING:  Please note that this software does not provide a way to
anonymize the LDP database after personal data have been loaded into it.
This configuration parameter should be used only with great care.**

Turning on the `disable_anonymization` parameter prevents the LDP loader
from attempting to anonymize extracted data, so that all data provided
by Okapi will be loaded into the LDP database.
-->


8\. Loading data from files (for testing only)
----------------------------------------------

As an alternative to loading data with the `--source` option, source
data can be loaded directly from the file system for testing purposes,
using the `--unsafe` and `--sourcedir` options, e.g.:

```shell
$ ldp load --unsafe --sourcedir ldp-analytics/testdata/ --target ldpdemo
```

The loader expects the data files to have particular names, e.g.
`loans_0.json`; and when `--sourcedir` is used, it also looks for an
optional, accompanying file ending with the suffix, `_test.json`, e.g.
`loans_test.json`.  Data in these "test" files are loaded into the same
table as the files they accompany.  This is used for testing in query
development to combine extracted test data with additional static test
data.


9\. Disabling database TLS/SSL (for testing only)
-------------------------------------------------

For very preliminary testing with a database running on `localhost`, one
may desire to connect to the database without TLS/SSL.  Disabling
TLS/SSL is generally not recommended, because it may expose passwords
and library data transmitted to the database.  However, TLS/SSL can be
disabled using the `--unsafe` and `--nossl` options, e.g.:

```shell
$ ldp load --source folio --target ldpdemo --unsafe --nossl
```


10\. "Direct extraction" of large data
--------------------------------------

At the time of writing (LDP 0.3), most FOLIO modules do not offer a
performant method of extracting a large number of records.  For this
reason, a workaround referred to as "direct extraction" has been
implemented in the LDP software that allows some data to be extracted
directly from a module's internal database, bypassing the module API.
Direct extraction is currently supported for holdings, instances, and
items.  It can be enabled by adding several values to the source
configuration, for example:

```
{
    "sources": {
        "folio": {

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
    },

    ( . . . )

}
```

Note that this requires the client host to be able to connect to the
database, which may be protected by a firewall.


Further reading
---------------

[**Learn about using the LDP database in the User Guide > > >**](USER_GUIDE.md)

