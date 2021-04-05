LDP Configuration Guide
=======================

##### Contents  
1\. [Foreign keys](#2-foreign-keys)  
[Reference](#reference)


1\. Foreign keys
----------------

### Detecting foreign keys

LDP can infer foreign key relationships in data.  To enable this
feature:

```sql
UPDATE
    dbconfig.general
SET
    detect_foreign_keys = TRUE;
```

This will cause the foreign key analysis to run directly after every
full update.  The results, in the form of suggested foreign key
constraints, are placed in the table
`dbsystem.suggested_foreign_keys`.  The data in this table can be
used to create foreign key constraints, as described in the next
section.

### Foreign key constraints

After a full update LDP can optionally create foreign key constraints,
which are defined in the table `dbconfig.foreign_keys`.  This table
has exactly the same schema as table
`dbsystem.suggested_foreign_keys` described in the previous section,
which allows the detected foreign keys to be easily used as a starting
point for configuring constraints:

```sql
INSERT INTO dbconfig.foreign_keys
SELECT
    *
FROM
    dbsystem.suggested_foreign_keys;
```

These constraints have no effect unless they are activated by setting
`enable_foreign_key_warnings` or `force_foreign_key_constraints` (or
both) in the table `dbconfig.general`:

The `enable_foreign_key_warnings` does not create any constraints, but
it writes referential integrity warnings to `dbsystem.log` as though
the constraints were to be created.

The `force_foreign_key_constraints` creates foreign key constraints.
In order to do this, it deletes rows having foreign keys that are not
present in referenced tables.

Both of these configuration values take effect after every full
update.


Reference
---------

### Table: dbconfig.general

* `detect_foreign_keys` (BOOLEAN) enables detection of foreign key
  relationships between tables in the `public` schema.  If enabled,
  the analysis runs after every full update and the table
  `dbsystem.suggested_foreign_keys` contains the results; otherwise
  the table is cleared.  The table schema is (`enable_constraint`,
  `referencing_table`, `referencing_column`, `referenced_table`,
  `referenced_column`).  Each listed pair of (`referencing table`,
  `referencing column`) appears in one or more rows that contain
  "candidate" pairs of (`referenced table`, `referenced column`).  If
  the referencing table and column have only one candidate, then the
  attribute `enable_constraint` is set to `TRUE`; otherwise it is
  `FALSE`.

* `enable_foreign_key_warnings` (BOOLEAN) enables logging of
  referential integrity warnings to `dbsystem.log` that would be
  generated if the database were to enforce the foreign key
  constraints specified in `dbconfig.foreign_keys` where
  `enable_constraint` is set to `TRUE`.

* `enable_full_updates` (BOOLEAN) turns on daily full updates when set
  to `TRUE`.  No full updates are performed if it is set to `FALSE`.

* `force_foreign_key_constraints` (BOOLEAN) enables creation of
  foreign key constraints specified in the table
  `dbconfig.foreign_keys` where `enable_constraint` is set to `TRUE`.
  This process deletes any rows having the specified foreign keys that
  are not present in referenced tables.  The deleted rows are the same
  ones logged as referential integrity warnings if
  `enable_foreign_key_warnings` has been set.

* `next_full_update` (TIMESTAMP WITH TIME ZONE) is the date and time
  of the next full update.  Once the full update begins, this value is
  automatically incremented to the next day at the same time.


Further reading
---------------

[__Administrator Guide__](Admin_Guide.md)

[__User Guide__](User_Guide.md)

