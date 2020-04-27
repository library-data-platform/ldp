Database Tools Compatibility
============================

This document is a compilation of notes on the compatibility of LDP
with database tools for querying/reporting, visualization, and data
analysis.


##### Contents  
1\. DBeaver Community Edition  
2\. Tableau Desktop  
3\. Microsoft Access


1\. DBeaver Community Edition
-----------------------------

### Summary

[DBeaver](https://dbeaver.io/) is used extensively by LDP query
writers and is believed to be compatible with LDP.

### DBeaver 6.3.0 timezone problems with Redshift

DBeaver, or possibly the JDBC driver it uses, does not set the user's
timezone correctly in Redshift.  This can be confirmed by issuing the
query:

```sql
SHOW timezone;
```

To set the timezone, e.g. to Eastern Time:

```sql
SET timezone='US/Eastern';
```

To view a list of valid timezone names that can be used with the `SET
timezone` command:

```sql
SELECT pg_timezone_names();
```

Setting the timezone using `SET timezone` only affects the current
session.  To run this command every time a new session begins, use
`Edit Connection` in DBeaver to modify the connection parameters.
Under `Connection settings > Initialization > Connection > Bootstrap
queries`, select `Configure`.  Then select `Add` and enter the `SET
timezone = . . . ;` command with your timezone.


2\. Tableau Desktop
-------------------

### Summary

[Tableau Desktop](https://www.tableau.com/products/desktop) is
believed to be compatible with LDP.


3\. Microsoft Access
--------------------

### Summary

We are currently investigating compatibility of PostgreSQL-based LDP
with [Microsoft Access](http://office.microsoft.com/access) 2019.  LDP
with Redshift does not appear to support Access at the present time.

<!--
LDP with PostgreSQL is believed to be compatible with [Microsoft
Access](http://office.microsoft.com/access) 2019.  LDP with Redshift
does not appear to support Access at the present time.
-->

### Background and additional issues

Access is a database system for Windows that includes a user interface
to the database.  In addition to providing an interface to its own
internal database, it can also connect to an external database and
allow interaction with it via the same user interface.  In Access this
concept is called "linked tables."  The user interface presents a
local table that is linked, i.e. automatically synchronized, with an
external table.

LDP users should be aware that in many cases Access has very specific
requirements for linking external tables, which however may not be
well documented.  Tables are required to have an integer key which
Access can use as a primary key, and they should not have attributes
with non-standard or non-traditional data types.  Tables that users
would like to modify via Access are required to contain a column
having a specific timestamp data type, which in PostgreSQL is
`TIMESTAMP(0)`.
 
