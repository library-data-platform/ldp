LDP User Guide
==============

##### Contents  
1\. [Data model](#1-data-model)  
2\. [JSON queries](#2-json-queries)  
3\. [Relational attributes vs. JSON](#3-relational-attributes-vs-json)  
4\. [Local tables](#4-local-tables)  
5\. [Creating reports](#5-creating-reports)  
6\. [Historical data](#6-historical-data)  
7\. [Database views](#7-database-views)  
8\. [JSON arrays](#8-json-arrays)  
9\. [Community](#9-community)


1\. Data model
--------------

The data model used for most LDP tables is a hybrid of relational and
JSON schemas.  Each table contains JSON data in a relational attribute
called `data`, and a subset of the JSON fields is also stored in
individual relational attributes.  It is typically queried via SQL:

```sql
SELECT * FROM circulation_loans LIMIT 1;
```

```
id            | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
action        | checkedin
due_date      | 2020-02-17 08:43:15+00
item_id       | 459afaba-5b39-468d-9072-eb1685e0ddf4
item_status   | Available
loan_date     | 2020-02-03 08:43:15+00
renewal_count | 0
return_date   | 2020-03-01 11:35:12+00
user_id       | ab579dc3-219b-4f5b-8068-ab1c7a55c402
data          | {                                                          
              |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
              |     "action": "checkedin",                                 
              |     "dueDate": "2020-02-17T08:43:15.000+0000",             
              |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
              |     "itemStatus": "Available",                             
              |     "loanDate": "2020-02-03T08:43:15Z",                    
              |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
              |     "renewalCount": 0,                                     
              |     "returnDate": "2020-03-01T11:35:12Z",                  
              |     "status": {                                            
              |         "name": "Closed"                                   
              |     },                                                     
              |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
              | }
```

The relational attributes are provided to simplify writing queries,
and the JSON fields offer access to the complete source data.

The LDP software creates these tables, having extracted the data from
source databases.  It then updates the data from those sources once
per day, so that the LDP database reflects the state of the source
data as of sometime within the past 24 hours or so.

(Note that uppercase is not required in SQL, but it is a common
convention used in published SQL queries and in documentation.)


2\. JSON queries
----------------

To access the JSON fields, the function `jsonb_extract_path_text()` (or
`json_extract_path_text()` for LDP 1.x.) can be used to retrieve data
from a path of nested fields, for example:

```sql
SELECT data FROM circulation_loans LIMIT 1;
```

```
                         data                          
-------------------------------------------------------
 {                                                     
     "id": "87598cd2-4989-4191-aaf0-c8fbb5552a06",     
     "action": "checkedout",                           
     "itemId": "575d5eec-ad28-472f-ba79-09f8cf5539a3", 
     "status": {                                       
         "name": "Open"                                
     },                                                
     "userId": "79dffc9d-be22-462e-b46f-78ca88babe91", 
     "dueDate": "2020-01-15T00:00:00Z",                
     "loanDate": "2020-01-01T00:00:00Z"                
 }                                                     
```

```sql
SELECT jsonb_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM circulation_loans
    GROUP BY status;
```

```
 status | count
--------+--------
 Closed | 504407
 Open   |   1294
```

In this example, `jsonb_extract_path_text(data, 'status', 'name')`
refers to the `name` field nested within the `status` field.

### JSON compatibility across LDP versions 1.x and 2.x

If there is a need to maintain compatibility across LDP 1.x and 2.x, the
`#>`and `#>>` operators may be used, for example:
```
t #>> '{f1,f2,f3}'
```
instead of:
```
jsonb_extract_path_text(t, f1, f2, f3)
```

Otherwise, the longer forms such as `jsonb_extract_path_text()` are
preferred, because they are significantly more readable.


3\. Relational attributes vs. JSON
----------------------------------

An example of a query written entirely using JSON data:

```sql
SELECT jsonb_extract_path_text(u.data, 'id') AS user_id,
       jsonb_extract_path_text(g.data, 'group') AS "group"
    FROM user_users AS u
        LEFT JOIN user_groups AS g
            ON jsonb_extract_path_text(u.data, 'patronGroup') = jsonb_extract_path_text(g.data, 'id')
    LIMIT 5;
```

This can be written in a simpler form by using the relational
attributes rather than JSON fields:

```sql
SELECT u.id AS user_id,
       g.group
    FROM user_users AS u
        LEFT JOIN user_groups AS g ON u.patron_group = g.id
    LIMIT 5;
```

```
               user_id                |    group
--------------------------------------+---------------
 429510c8-0b50-4ec5-b4ca-462278c0677b | staff
 af91674a-6468-4d5f-b037-28ebbf02f051 | undergraduate
 0736f1a1-6a19-4f13-ab3b-50a4915dcc4a | undergraduate
 37274158-5e38-4852-93c7-359bfdd76892 | undergraduate
 2c2864b1-58cb-4752-ae8e-3ce7bc9a3be4 | faculty
```


4\. Local tables
----------------

The `local` schema is created by LDP as a shared, common area in the
database where users can create or import their own data sets,
including storing the results of queries, e.g.:

```sql
CREATE TABLE local.loan_status AS
SELECT jsonb_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM circulation_loans
    GROUP BY status;
```


5\. Creating reports
--------------------

An effective way to create a report is to package it as a database
function.  A database function can define a query and associated
parameters.  Users can then call the function, specifying a value for
each parameter.

For example, suppose that the following query counts the number of
loans for each circulated item within a range of dates.

```sql
SELECT item_id,
       count(*) AS loan_count
    FROM circulation_loans
    WHERE '2023-01-01' <= loan_date AND loan_date < '2024-01-01'
    GROUP BY item_id;
```

We can create a function to generalize this query.  Instead of
including the dates directly within the query, we will define them as
parameters: `start_date` and `end_date`.

```sql
CREATE FUNCTION local.lisa_count_loans(
    start_date date DEFAULT '2000-01-01',
    end_date date DEFAULT '2050-01-01')
RETURNS TABLE(
    item_id text,
    loan_count integer)
AS $$
SELECT item_id,
       count(*) AS loan_count
    FROM circulation_loans
    WHERE start_date <= loan_date AND loan_date < end_date
    GROUP BY item_id
$$
LANGUAGE SQL
STABLE
PARALLEL SAFE;
```

Now the function can be called with different arguments to generate
reports:

```sql
SELECT * FROM local.lisa_count_loans(start_date => '2022-01-01', end_date => '2023-01-01');

SELECT * FROM local.lisa_count_loans(start_date => '2023-01-01');
```

By default, all LDP users share the same user account, and all users
will be able to call this function.  However if LDP has been
configured with individual user accounts, the user that created the
function would have to grant other users privileges before they could
call it, for example:

```sql
GRANT EXECUTE ON FUNCTION local.lisa_count_loans TO celia, rosalind;
```

Defining shared functions can be used together with a web-based
database tool such as CloudBeaver to make reports available to a wider
group of users.


6\. Historical data
-------------------

### History tables

As mentioned earlier, the database contains a snapshot of the source
data as of the last update.  For all tables except `srs_error`,
`srs_marc`, and `srs_records`, LDP also retains data that have been
updated in the past, including data that may no longer exist in the
source.  These "historical data" are stored in a schema called
`history`.  Each table normally has a corresponding history table,
e.g.  the history table for `circulation_loans` is
`history.circulation_loans`.

This historical data capability can be used for gaining insights about
the library by analyzing trends over time.

History tables contain these attributes:

* `id` is the record ID.
* `data` is the source data, usually a JSON object.
* `updated` is the date and time when the data were updated.

For example:

```sql
SELECT * FROM history.circulation_loans LIMIT 1;
```

```
id        | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
data      | {                                                          
          |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
          |     "action": "checkedin",                                 
          |     "dueDate": "2020-02-17T08:43:15.000+0000",             
          |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
          |     "itemStatus": "Available",                             
          |     "loanDate": "2020-02-03T08:43:15Z",                    
          |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
          |     "renewalCount": 0,                                     
          |     "returnDate": "2020-03-01T11:35:12Z",                  
          |     "status": {                                            
          |         "name": "Closed"                                   
          |     },                                                     
          |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
          | }
updated   | 2020-03-02 03:46:49.362606+00
```

Unlike the main tables in which `id` is unique, the history tables can
accumulate many records with the same value for `id`.  Note also that
if a value in the source changes more than once during the interval
between two LDP updates, the history will only reflect the last of
those changes.

### Querying historical data

These are some basic examples that show data evolving over time.

For a high level view of all updated records, for example, in
`circulation_loans`:

```sql
SELECT updated,
       count(*)
    FROM history.circulation_loans
    GROUP BY updated
    ORDER BY updated;
```

To view how a single record changes over time:

```sql
SELECT *
    FROM history.circulation_loans
    WHERE id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
    ORDER BY updated;
```

The `SELECT` clause of this query can be modified to examine only
specific fields, e.g.:

```sql
SELECT jsonb_extract_path_text(data, 'action'),
       jsonb_extract_path_text(data, 'returnDate'),
       updated
    FROM history.circulation_loans
    WHERE id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
    ORDER BY updated;
```

### Data cleaning

Since the source data schemas may evolve over time, the `data`
attribute in history tables does not necessarily have a single schema
that is consistent over an entire table.  As a result, querying
history tables may require "data cleaning."  A suggested first step
would be to select a subset of data within a time window, pulling out
JSON fields of interest into relational attributes, and storing this
result in a local table, e.g.:

```sql
CREATE TABLE local.loan_status_history AS
SELECT id,
       jsonb_extract_path_text(data, 'itemStatus') AS item_status,
       updated
    FROM history.circulation_loans
    WHERE '2020-01-01' <= updated AND updated < '2021-01-01';
```

This will make it easier to examine the data to check for inconsistent
or missing values, update them, etc.  Note that in SQL, `''` and
`NULL` may look the same in the output of a `SELECT`, but they are
distinct values.


7\. Database views
------------------

The schema of source data can change over time, and LDP reflects
these changes when it updates data.  For this reason, LDP does not
support the use of database views.  LDP updates may fail to run if
the database contains views.  Instead of creating a view, use `CREATE
TABLE ... AS SELECT ...` to store a result set, as in the local schema
example above.


8\. JSON arrays
---------------

LDP does not yet support extracting arrays from JSON data.  However,
there is a workaround for PostgreSQL users.

A lateral join can be used with the function `jsonb_array_elements()`
(or `json_array_elements()` for LDP 1.x) to convert the elements of a
JSON array to a set of rows, one row per array element.

For example, if the array elements are simple `text` strings:

```sql
CREATE TABLE local.example_array_simple AS
SELECT id AS instance_id,
       instance_format_ids.data #>> '{}' AS instance_format_id,
       instance_format_ids.ordinality
    FROM inventory_instances
        CROSS JOIN LATERAL jsonb_array_elements(jsonb_extract_path(data, 'instanceFormatIds')) WITH ORDINALITY
            AS instance_format_ids (data);
```

If the array elements are JSON objects:

```sql
CREATE TABLE local.example_array_objects AS
SELECT id AS holdings_id,
       jsonb_extract_path_text(notes.data, 'holdingsNoteTypeId') AS holdings_note_type_id,
        jsonb_extract_path_text(notes.data, 'note') AS note,
        (jsonb_extract_path_text(notes.data, 'staffOnly'))::boolean AS staff_only,
        notes.ordinality
    FROM inventory_holdings
        CROSS JOIN LATERAL jsonb_array_elements(jsonb_extract_path(data, 'notes')) WITH ORDINALITY
            AS notes (data);
```


Further reading
---------------

[__Administrator Guide__](Admin_Guide.md)

